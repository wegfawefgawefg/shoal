#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gsexp {

enum class TokenType : std::uint8_t {
    LParen,
    RParen,
    Atom,
    String,
};

enum class ValueType : std::uint8_t {
    List,
    String,
    Atom,
};

enum class TextStorage : std::uint8_t {
    Source,
    Decoded,
};

enum class DiagnosticSeverity {
    Warning,
    Error,
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string message;
    int line = 1;
    int column = 1;
};

struct Token {
    TokenType type = TokenType::Atom;
    std::string text;
    int line = 1;
    int column = 1;
};

constexpr std::uint32_t invalid_node = 0xffffffffu;
constexpr std::uint16_t invalid_child_index_cache = 0xffffu;

struct NodeData {
    std::uint32_t text_offset = 0;
    std::uint32_t text_size = 0;
    std::uint32_t first_child = invalid_node;
    std::uint32_t next_sibling = invalid_node;
    std::uint16_t child_count = 0;
    ValueType type = ValueType::List;
    TextStorage text_storage = TextStorage::Source;
};

struct KeyIndexEntry {
    std::uint32_t key_offset = 0;
    std::uint32_t key_size = 0;
    std::uint32_t child = invalid_node;
};

struct ChildIndexCache {
    std::uint32_t first_entry = 0;
    std::uint32_t entry_count = 0;
};

struct ChildCountOverflow {
    std::uint32_t node = invalid_node;
    std::uint32_t count = 0;
};

struct ParseStorage {
    std::string source;
    std::vector<char> decoded_text;
    std::size_t decoded_string_count = 0;
    std::vector<NodeData> nodes;
    std::vector<ChildCountOverflow> child_count_overflows;
    mutable std::vector<ChildIndexCache> child_indexes;
    mutable std::vector<KeyIndexEntry> child_index_entries;
    mutable std::vector<std::uint16_t> child_index_lookup;
    mutable std::vector<float> float_cache;
};

struct StorageStats {
    std::size_t source_bytes = 0;
    std::size_t node_data_bytes = 0;
    std::size_t node_count = 0;
    std::size_t node_capacity = 0;
    std::size_t node_bytes = 0;
    double node_bytes_per_source_byte = 0.0;
    std::size_t child_count_overflow_count = 0;
    std::size_t child_count_overflow_capacity = 0;
    std::size_t child_count_overflow_bytes = 0;
    std::size_t decoded_string_count = 0;
    std::size_t decoded_string_bytes = 0;
    std::size_t child_index_count = 0;
    std::size_t child_index_capacity = 0;
    std::size_t child_index_cache_bytes = 0;
    std::size_t child_index_lookup_capacity = 0;
    std::size_t child_index_lookup_bytes = 0;
    std::size_t child_index_entry_count = 0;
    std::size_t child_index_entry_capacity = 0;
    std::size_t child_index_entry_bytes = 0;
    std::size_t float_cache_capacity = 0;
    std::size_t float_cache_bytes = 0;
    std::size_t root_count = 0;
    std::size_t approximate_bytes = 0;
};

class Node;
class FormView;
class ChildIterator;
class ChildRange;

struct ParseResult {
    bool ok = false;
    std::shared_ptr<ParseStorage> storage;
    std::vector<std::uint32_t> roots;
    std::vector<Diagnostic> diagnostics;

    std::size_t root_count() const;
    Node root(std::size_t index) const;
    StorageStats storage_stats() const;
};

class Node {
  public:
    Node() = default;
    Node(const ParseStorage* storage, std::uint32_t index);

    bool valid() const;
    bool empty() const;
    ValueType type() const;
    std::string_view text() const;
    std::size_t child_count() const;
    Node first_child() const;
    Node next_sibling() const;
    Node child_at(std::size_t child_index) const;
    Node head() const;
    Node second() const;
    ChildRange children() const;
    bool is_list() const;
    bool is_atom(std::string_view atom) const;
    bool is_string() const;

  private:
    friend class ChildIterator;
    friend class ChildRange;
    friend class FormView;

    const NodeData* data() const;

    const ParseStorage* storage = nullptr;
    std::uint32_t index = invalid_node;
};

class ChildIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Node;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = Node;

    ChildIterator() = default;
    ChildIterator(const ParseStorage* storage, std::uint32_t index);

    Node operator*() const;
    ChildIterator& operator++();
    bool operator==(const ChildIterator& other) const;
    bool operator!=(const ChildIterator& other) const;

  private:
    const ParseStorage* storage = nullptr;
    std::uint32_t index = invalid_node;
};

class ChildRange {
  public:
    ChildRange() = default;
    ChildRange(const ParseStorage* storage, std::uint32_t first);

    ChildIterator begin() const;
    ChildIterator end() const;

  private:
    const ParseStorage* storage = nullptr;
    std::uint32_t first = invalid_node;
};

class FormView {
  public:
    FormView() = default;
    explicit FormView(Node node);

    bool valid() const;
    Node node() const;
    Node head() const;
    Node arg(std::size_t index) const;
    Node find(std::string_view head) const;
    Node find_arg(std::string_view head, std::size_t index) const;

    std::optional<int> get_int(std::string_view head) const;
    std::optional<float> get_float(std::string_view head) const;
    std::optional<std::string> get_string(std::string_view head) const;
    std::optional<std::string_view> get_string_view(std::string_view head) const;

  private:
    Node form;
};

bool looks_like_integer(std::string_view text);
bool looks_like_float(std::string_view text);

ParseResult parse(std::string_view text);
ParseResult parse_owned(std::string text);
std::vector<Token> tokenize(std::string_view text, std::vector<Diagnostic>* diagnostics = nullptr);

bool is_atom(Node node, std::string_view atom);
bool is_symbol(Node node, std::string_view symbol);

std::string quote_string(std::string_view text);

} // namespace gsexp
