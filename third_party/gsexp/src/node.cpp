#include "gsexp/sexp.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace gsexp {
namespace {

constexpr std::uint16_t child_count_overflow_marker = 0xffffu;

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

bool node_text_equals(const ParseStorage& storage, const NodeData& node, std::string_view text) {
    if (node.text_size != text.size())
        return false;
    if (text.empty())
        return true;

    if (node.text_storage == TextStorage::Decoded) {
        if (node.text_offset > storage.decoded_text.size())
            return false;
        std::size_t available = storage.decoded_text.size() - node.text_offset;
        if (node.text_size > available)
            return false;
        const char* data = storage.decoded_text.data() + node.text_offset;
        return std::memcmp(data, text.data(), text.size()) == 0;
    }

    if (node.text_offset > storage.source.size())
        return false;
    std::size_t available = storage.source.size() - node.text_offset;
    if (node.text_size > available)
        return false;
    const char* data = storage.source.data() + node.text_offset;
    return std::memcmp(data, text.data(), text.size()) == 0;
}

std::size_t full_child_count(const ParseStorage& storage,
                             const NodeData& node,
                             std::uint32_t node_index) {
    if (node.child_count != child_count_overflow_marker)
        return node.child_count;

    for (const ChildCountOverflow& item : storage.child_count_overflows) {
        if (item.node == node_index)
            return item.count;
    }

    return child_count_overflow_marker;
}

} // namespace

std::size_t ParseResult::root_count() const {
    return roots.size();
}

Node ParseResult::root(std::size_t index) const {
    if (!storage || index >= roots.size())
        return {};
    return Node(storage.get(), roots[index]);
}

StorageStats ParseResult::storage_stats() const {
    StorageStats stats;
    stats.root_count = roots.size();
    if (!storage)
        return stats;

    stats.source_bytes = storage->source.size();
    stats.node_data_bytes = sizeof(NodeData);
    stats.node_count = storage->nodes.size();
    stats.node_capacity = storage->nodes.capacity();
    stats.node_bytes = storage->nodes.capacity() * sizeof(NodeData);
    if (stats.source_bytes > 0)
        stats.node_bytes_per_source_byte =
            static_cast<double>(stats.node_bytes) / static_cast<double>(stats.source_bytes);
    stats.child_count_overflow_count = storage->child_count_overflows.size();
    stats.child_count_overflow_capacity = storage->child_count_overflows.capacity();
    stats.child_count_overflow_bytes =
        stats.child_count_overflow_capacity * sizeof(ChildCountOverflow);
    stats.decoded_string_count = storage->decoded_string_count;
    stats.decoded_string_bytes = storage->decoded_text.size();

    stats.child_index_count = storage->child_indexes.size();
    stats.child_index_capacity = storage->child_indexes.capacity();
    stats.child_index_cache_bytes = stats.child_index_capacity * sizeof(ChildIndexCache);
    stats.child_index_lookup_capacity = storage->child_index_lookup.capacity();
    stats.child_index_lookup_bytes = stats.child_index_lookup_capacity * sizeof(std::uint16_t);
    stats.child_index_entry_count = storage->child_index_entries.size();
    stats.child_index_entry_capacity = storage->child_index_entries.capacity();
    stats.child_index_entry_bytes = stats.child_index_entry_capacity * sizeof(KeyIndexEntry);
    stats.float_cache_capacity = storage->float_cache.capacity();
    stats.float_cache_bytes = stats.float_cache_capacity * sizeof(float);

    stats.approximate_bytes = storage->source.capacity() +
                              storage->nodes.capacity() * sizeof(NodeData) +
                              stats.child_count_overflow_bytes +
                              storage->decoded_text.capacity() +
                              stats.child_index_cache_bytes +
                              stats.child_index_lookup_capacity * sizeof(std::uint16_t) +
                              stats.child_index_entry_bytes +
                              stats.float_cache_bytes;
    return stats;
}

Node::Node(const ParseStorage* node_storage, std::uint32_t node_index)
    : storage(node_storage), index(node_index) {}

bool Node::valid() const {
    return data() != nullptr;
}

bool Node::empty() const {
    return !valid();
}

ValueType Node::type() const {
    const NodeData* node = data();
    if (!node)
        return ValueType::Atom;
    return node->type;
}

std::string_view Node::text() const {
    const NodeData* node = data();
    if (!node)
        return {};
    return node_text(*storage, *node);
}

std::size_t Node::child_count() const {
    const NodeData* node = data();
    if (!node)
        return 0;
    return full_child_count(*storage, *node, index);
}

Node Node::first_child() const {
    const NodeData* node = data();
    if (!node)
        return {};
    return Node(storage, node->first_child);
}

Node Node::next_sibling() const {
    const NodeData* node = data();
    if (!node)
        return {};
    return Node(storage, node->next_sibling);
}

Node Node::child_at(std::size_t child_index) const {
    Node child = first_child();
    for (std::size_t offset = 0; offset < child_index && child.valid(); ++offset)
        child = child.next_sibling();
    return child;
}

Node Node::head() const {
    return first_child();
}

Node Node::second() const {
    Node first = first_child();
    if (!first.valid())
        return {};
    return first.next_sibling();
}

ChildRange Node::children() const {
    const NodeData* node = data();
    if (!node)
        return {};
    return ChildRange(storage, node->first_child);
}

bool Node::is_list() const {
    return valid() && type() == ValueType::List;
}

bool Node::is_atom(std::string_view atom) const {
    const NodeData* node = data();
    return node && node->type == ValueType::Atom && node_text_equals(*storage, *node, atom);
}

bool Node::is_string() const {
    return valid() && type() == ValueType::String;
}

const NodeData* Node::data() const {
    if (!storage || index == invalid_node || index >= storage->nodes.size())
        return nullptr;
    return &storage->nodes[index];
}

ChildIterator::ChildIterator(const ParseStorage* iterator_storage, std::uint32_t node_index)
    : storage(iterator_storage), index(node_index) {}

Node ChildIterator::operator*() const {
    return Node(storage, index);
}

ChildIterator& ChildIterator::operator++() {
    if (!storage || index == invalid_node || index >= storage->nodes.size()) {
        index = invalid_node;
        return *this;
    }

    index = storage->nodes[index].next_sibling;
    return *this;
}

bool ChildIterator::operator==(const ChildIterator& other) const {
    return index == other.index && storage == other.storage;
}

bool ChildIterator::operator!=(const ChildIterator& other) const {
    return !(*this == other);
}

ChildRange::ChildRange(const ParseStorage* range_storage, std::uint32_t first_index)
    : storage(range_storage), first(first_index) {}

ChildIterator ChildRange::begin() const {
    return ChildIterator(storage, first);
}

ChildIterator ChildRange::end() const {
    return ChildIterator(storage, invalid_node);
}

bool is_atom(Node node, std::string_view atom) {
    return node.is_atom(atom);
}

bool is_symbol(Node node, std::string_view symbol) {
    return is_atom(node, symbol);
}

} // namespace gsexp
