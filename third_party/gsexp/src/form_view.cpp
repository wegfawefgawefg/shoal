#include "gsexp/sexp.hpp"

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <version>

namespace gsexp {
namespace {

constexpr std::uint32_t indexed_child_threshold = 16;
constexpr std::uint32_t linear_index_lookup_limit = 16;
constexpr float invalid_cached_float = std::numeric_limits<float>::infinity();

std::string_view node_text(const ParseStorage& storage, const NodeData& node) {
    if (node.text_size == 0)
        return {};

    if (node.text_storage == TextStorage::Decoded) {
        if (node.text_offset > storage.decoded_text.size())
            return {};
        std::size_t available = storage.decoded_text.size() - node.text_offset;
        std::size_t size = node.text_size;
        if (size > available)
            size = available;
        return std::string_view(storage.decoded_text.data() + node.text_offset, size);
    }

    if (node.text_offset > storage.source.size())
        return {};
    std::size_t available = storage.source.size() - node.text_offset;
    std::size_t size = node.text_size;
    if (size > available)
        size = available;
    return std::string_view(storage.source.data() + node.text_offset, size);
}

bool source_atom_text_equals(const ParseStorage& storage, const NodeData& node, std::string_view text) {
    if (node.type != ValueType::Atom || node.text_size != text.size())
        return false;
    if (text.empty())
        return true;
    if (node.text_offset > storage.source.size())
        return false;
    std::size_t available = storage.source.size() - node.text_offset;
    if (node.text_size > available)
        return false;
    return std::memcmp(storage.source.data() + node.text_offset, text.data(), text.size()) == 0;
}

std::string_view index_key_text(const ParseStorage& storage, const KeyIndexEntry& entry) {
    if (entry.key_offset > storage.source.size())
        return {};
    std::size_t available = storage.source.size() - entry.key_offset;
    std::size_t size = entry.key_size;
    if (size > available)
        size = available;
    return std::string_view(storage.source.data() + entry.key_offset, size);
}

int compare_index_key(const ParseStorage& storage, const KeyIndexEntry& left, std::string_view right) {
    std::string_view left_text = index_key_text(storage, left);
    std::size_t compare_size = left_text.size();
    if (right.size() < compare_size)
        compare_size = right.size();

    int compared = 0;
    if (compare_size > 0)
        compared = std::memcmp(left_text.data(), right.data(), compare_size);
    if (compared != 0)
        return compared;
    if (left_text.size() < right.size())
        return -1;
    if (left_text.size() > right.size())
        return 1;
    return 0;
}

bool index_key_less(const ParseStorage& storage, const KeyIndexEntry& left, const KeyIndexEntry& right) {
    return compare_index_key(storage, left, index_key_text(storage, right)) < 0;
}

std::vector<KeyIndexEntry> build_child_index(const ParseStorage& storage, const NodeData& list) {
    std::vector<KeyIndexEntry> entries;
    if (list.child_count < indexed_child_threshold)
        return entries;

    entries.reserve(list.child_count - 1);

    bool first = true;
    std::uint32_t child_index = list.first_child;
    while (child_index != invalid_node && child_index < storage.nodes.size()) {
        const NodeData& child = storage.nodes[child_index];
        if (first) {
            first = false;
            child_index = child.next_sibling;
            continue;
        }
        if (child.type == ValueType::List && child.first_child != invalid_node) {
            const NodeData& head = storage.nodes[child.first_child];
            if (head.type == ValueType::Atom)
                entries.push_back(KeyIndexEntry{head.text_offset, head.text_size, child_index});
        }
        child_index = child.next_sibling;
    }

    std::stable_sort(entries.begin(), entries.end(), [&storage](const KeyIndexEntry& a, const KeyIndexEntry& b) {
        return index_key_less(storage, a, b);
    });
    return entries;
}

std::uint32_t find_child_index_direct(const ParseStorage& storage,
                                      const NodeData& list,
                                      std::string_view searched_head) {
    bool first = true;
    std::uint32_t child_index = list.first_child;
    while (child_index != invalid_node && child_index < storage.nodes.size()) {
        const NodeData& child = storage.nodes[child_index];
        if (first) {
            first = false;
            child_index = child.next_sibling;
            continue;
        }

        if (child.type != ValueType::List || child.first_child == invalid_node) {
            child_index = child.next_sibling;
            continue;
        }

        const NodeData& head = storage.nodes[child.first_child];
        if (source_atom_text_equals(storage, head, searched_head))
            return child_index;

        child_index = child.next_sibling;
    }

    return invalid_node;
}

std::uint32_t find_child_index(const ParseStorage& storage,
                               std::uint32_t list_index,
                               const NodeData& list,
                               std::string_view searched_head) {
    if (list.child_count < indexed_child_threshold)
        return find_child_index_direct(storage, list, searched_head);

    if (storage.child_index_lookup.empty())
        storage.child_index_lookup.assign(storage.nodes.size(), invalid_child_index_cache);

    std::uint16_t cache_index = storage.child_index_lookup[list_index];
    if (cache_index == invalid_child_index_cache) {
        std::vector<KeyIndexEntry> entries = build_child_index(storage, list);
        if (storage.child_indexes.size() >= invalid_child_index_cache)
            return find_child_index_direct(storage, list, searched_head);

        ChildIndexCache cache;
        cache.first_entry = static_cast<std::uint32_t>(storage.child_index_entries.size());
        cache.entry_count = static_cast<std::uint32_t>(entries.size());
        if (storage.child_index_entries.empty())
            storage.child_index_entries.reserve(storage.nodes.size() / 3);
        storage.child_index_entries.insert(storage.child_index_entries.end(), entries.begin(), entries.end());

        cache_index = static_cast<std::uint16_t>(storage.child_indexes.size());
        storage.child_indexes.push_back(std::move(cache));
        storage.child_index_lookup[list_index] = cache_index;
    }

    const ChildIndexCache& cache = storage.child_indexes[static_cast<std::size_t>(cache_index)];
    auto begin = storage.child_index_entries.begin() + cache.first_entry;
    auto end = begin + cache.entry_count;
    if (cache.entry_count <= linear_index_lookup_limit) {
        for (auto entry = begin; entry != end; ++entry) {
            if (compare_index_key(storage, *entry, searched_head) == 0)
                return entry->child;
        }
        return invalid_node;
    }

    auto entry = std::lower_bound(begin,
                                  end,
                                  searched_head,
                                  [&storage](const KeyIndexEntry& item, std::string_view key) {
                                      return compare_index_key(storage, item, key) < 0;
                                  });
    if (entry != end && compare_index_key(storage, *entry, searched_head) == 0)
        return entry->child;

    return invalid_node;
}

const NodeData* find_arg_data(const ParseStorage& storage,
                              std::uint32_t list_index,
                              const NodeData& list,
                              std::string_view searched_head,
                              std::size_t arg_index) {
    if (list.type != ValueType::List)
        return nullptr;

    std::uint32_t child_index = find_child_index(storage, list_index, list, searched_head);
    if (child_index == invalid_node || child_index >= storage.nodes.size())
        return nullptr;

    const NodeData& child = storage.nodes[child_index];
    if (child.type != ValueType::List || child.first_child == invalid_node)
        return nullptr;

    std::uint32_t value_index = storage.nodes[child.first_child].next_sibling;
    for (std::size_t offset = 0; offset < arg_index && value_index != invalid_node; ++offset) {
        if (value_index >= storage.nodes.size())
            return nullptr;
        value_index = storage.nodes[value_index].next_sibling;
    }

    if (value_index == invalid_node || value_index >= storage.nodes.size())
        return nullptr;
    return &storage.nodes[value_index];
}

std::uint32_t find_arg_index(const ParseStorage& storage,
                             std::uint32_t list_index,
                             const NodeData& list,
                             std::string_view searched_head,
                             std::size_t arg_index) {
    if (list.type != ValueType::List)
        return invalid_node;

    std::uint32_t child_index = find_child_index(storage, list_index, list, searched_head);
    if (child_index == invalid_node || child_index >= storage.nodes.size())
        return invalid_node;

    const NodeData& child = storage.nodes[child_index];
    if (child.type != ValueType::List || child.first_child == invalid_node)
        return invalid_node;

    std::uint32_t value_index = storage.nodes[child.first_child].next_sibling;
    for (std::size_t offset = 0; offset < arg_index && value_index != invalid_node; ++offset) {
        if (value_index >= storage.nodes.size())
            return invalid_node;
        value_index = storage.nodes[value_index].next_sibling;
    }

    if (value_index == invalid_node || value_index >= storage.nodes.size())
        return invalid_node;
    return value_index;
}

std::optional<int> parse_int(std::string_view text) {
    if (text.empty())
        return std::nullopt;

    bool negative = false;
    if (text.front() == '+' || text.front() == '-') {
        negative = text.front() == '-';
        text.remove_prefix(1);
        if (text.empty())
            return std::nullopt;
    }

    std::uint64_t value = 0;
    std::uint64_t limit = negative ? static_cast<std::uint64_t>(std::numeric_limits<int>::max()) + 1u
                                   : static_cast<std::uint64_t>(std::numeric_limits<int>::max());
    for (char ch : text) {
        if (ch < '0' || ch > '9')
            return std::nullopt;
        std::uint64_t digit = static_cast<std::uint64_t>(ch - '0');
        if (value > (limit - digit) / 10u)
            return std::nullopt;
        value = value * 10u + digit;
    }

    if (negative && value == limit)
        return std::numeric_limits<int>::min();
    int parsed = static_cast<int>(value);
    return negative ? -parsed : parsed;
}

std::optional<float> parse_float(std::string_view text) {
    if (!looks_like_float(text) && !looks_like_integer(text))
        return std::nullopt;
    if (!text.empty() && text.front() == '+')
        text.remove_prefix(1);

    float parsed = 0.0f;
#if defined(__cpp_lib_to_chars)
    const char* begin = text.data();
    const char* end = begin + text.size();
    auto result = std::from_chars(begin, end, parsed);
    if (result.ec == std::errc{} && result.ptr == end)
        return parsed;
#else
    // Apple's libc++ has no floating-point from_chars; strtof reads the same
    // text in the "C" locale, which SDL and gsexp never change.
    const std::string copy(text);
    char* stop = nullptr;
    errno = 0;
    parsed = std::strtof(copy.c_str(), &stop);
    if (errno != ERANGE && stop == copy.c_str() + copy.size())
        return parsed;
#endif
    return std::nullopt;
}

std::optional<float> cached_float(const ParseStorage& storage, std::uint32_t node_index) {
    if (node_index >= storage.nodes.size())
        return std::nullopt;

    const NodeData& node = storage.nodes[node_index];
    if (node.type != ValueType::Atom)
        return std::nullopt;

    if (storage.float_cache.empty())
        storage.float_cache.resize(storage.nodes.size(), std::numeric_limits<float>::quiet_NaN());

    float cached = storage.float_cache[node_index];
    if (std::isinf(cached))
        return std::nullopt;
    if (!std::isnan(cached))
        return cached;

    std::optional<float> parsed = parse_float(node_text(storage, node));
    storage.float_cache[node_index] = parsed ? *parsed : invalid_cached_float;
    return parsed;
}

} // namespace

FormView::FormView(Node node) : form(node) {}

bool FormView::valid() const {
    return form.is_list();
}

Node FormView::node() const {
    return form;
}

Node FormView::head() const {
    return form.head();
}

Node FormView::arg(std::size_t index) const {
    return form.child_at(index + 1);
}

Node FormView::find(std::string_view searched_head) const {
    const ParseStorage* storage = form.storage;
    const NodeData* form_data = form.data();
    if (!storage || !form_data || form_data->type != ValueType::List)
        return {};

    std::uint32_t child_index = find_child_index(*storage, form.index, *form_data, searched_head);
    return Node(storage, child_index);
}

Node FormView::find_arg(std::string_view searched_head, std::size_t index) const {
    const ParseStorage* storage = form.storage;
    const NodeData* form_data = form.data();
    if (!storage || !form_data)
        return {};

    std::uint32_t value_index = find_arg_index(*storage, form.index, *form_data, searched_head, index);
    return Node(storage, value_index);
}

std::optional<int> FormView::get_int(std::string_view searched_head) const {
    const ParseStorage* storage = form.storage;
    const NodeData* form_data = form.data();
    if (!storage || !form_data)
        return std::nullopt;

    const NodeData* value = find_arg_data(*storage, form.index, *form_data, searched_head, 0);
    if (!value || value->type != ValueType::Atom)
        return std::nullopt;
    return parse_int(node_text(*storage, *value));
}

std::optional<float> FormView::get_float(std::string_view searched_head) const {
    const ParseStorage* storage = form.storage;
    const NodeData* form_data = form.data();
    if (!storage || !form_data)
        return std::nullopt;

    std::uint32_t value_index = find_arg_index(*storage, form.index, *form_data, searched_head, 0);
    return cached_float(*storage, value_index);
}

std::optional<std::string> FormView::get_string(std::string_view searched_head) const {
    const ParseStorage* storage = form.storage;
    const NodeData* form_data = form.data();
    if (!storage || !form_data)
        return std::nullopt;

    const NodeData* value = find_arg_data(*storage, form.index, *form_data, searched_head, 0);
    if (!value || (value->type != ValueType::Atom && value->type != ValueType::String))
        return std::nullopt;
    return std::string(node_text(*storage, *value));
}

std::optional<std::string_view> FormView::get_string_view(std::string_view searched_head) const {
    const ParseStorage* storage = form.storage;
    const NodeData* form_data = form.data();
    if (!storage || !form_data)
        return std::nullopt;

    const NodeData* value = find_arg_data(*storage, form.index, *form_data, searched_head, 0);
    if (!value || (value->type != ValueType::Atom && value->type != ValueType::String))
        return std::nullopt;
    return node_text(*storage, *value);
}

} // namespace gsexp
