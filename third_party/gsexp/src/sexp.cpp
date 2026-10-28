#include "gsexp/sexp.hpp"

#if defined(__SSE2__) &&                                                                      \
    (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86))
#include <emmintrin.h>
#define GSEXP_PARSE_SSE2 1
#else
#define GSEXP_PARSE_SSE2 0
#endif

#include <utility>

namespace gsexp {
namespace {

constexpr std::uint16_t child_count_overflow_marker = 0xffffu;
constexpr std::size_t reserve_sample_bytes = 4 * 1024;
constexpr std::size_t reserve_min_nodes = 64;
constexpr std::size_t reserve_slack_nodes = 16;
constexpr double reserve_growth_slack = 1.02;

bool is_space(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\f';
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_delimiter(char c) {
    return is_space(c) || c == '(' || c == ')';
}

bool is_string_special(char c) {
    return c == '"' || c == '\\' || c == '\n' || c == '\r';
}

int first_set_bit(std::uint32_t mask) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(mask);
#else
    int bit = 0;
    while ((mask & 1u) == 0u) {
        mask >>= 1u;
        ++bit;
    }
    return bit;
#endif
}

#if GSEXP_PARSE_SSE2
__m128i match_byte(__m128i bytes, char c) {
    return _mm_cmpeq_epi8(bytes, _mm_set1_epi8(c));
}
#endif

std::size_t find_atom_end(std::string_view text, std::size_t start) {
    std::size_t index = start;

#if GSEXP_PARSE_SSE2
    while (index + 16 <= text.size()) {
        __m128i bytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(text.data() + index));
        __m128i matches = match_byte(bytes, '(');
        matches = _mm_or_si128(matches, match_byte(bytes, ')'));
        matches = _mm_or_si128(matches, match_byte(bytes, ' '));
        matches = _mm_or_si128(matches, match_byte(bytes, '\n'));
        matches = _mm_or_si128(matches, match_byte(bytes, '\r'));
        matches = _mm_or_si128(matches, match_byte(bytes, '\t'));
        matches = _mm_or_si128(matches, match_byte(bytes, '\v'));
        matches = _mm_or_si128(matches, match_byte(bytes, '\f'));

        std::uint32_t mask = static_cast<std::uint32_t>(_mm_movemask_epi8(matches));
        if (mask != 0)
            return index + static_cast<std::size_t>(first_set_bit(mask));
        index += 16;
    }
#endif

    while (index < text.size() && !is_delimiter(text[index]))
        ++index;
    return index;
}

std::size_t find_string_special(std::string_view text, std::size_t start) {
    std::size_t index = start;

#if GSEXP_PARSE_SSE2
    while (index + 16 <= text.size()) {
        __m128i bytes = _mm_loadu_si128(reinterpret_cast<const __m128i*>(text.data() + index));
        __m128i matches = match_byte(bytes, '"');
        matches = _mm_or_si128(matches, match_byte(bytes, '\\'));
        matches = _mm_or_si128(matches, match_byte(bytes, '\n'));
        matches = _mm_or_si128(matches, match_byte(bytes, '\r'));

        std::uint32_t mask = static_cast<std::uint32_t>(_mm_movemask_epi8(matches));
        if (mask != 0)
            return index + static_cast<std::size_t>(first_set_bit(mask));
        index += 16;
    }
#endif

    while (index < text.size() && !is_string_special(text[index]))
        ++index;
    return index;
}

struct SourcePosition {
    int line = 1;
    int column = 1;
};

SourcePosition position_for_offset(std::string_view source, std::size_t offset) {
    SourcePosition position;
    std::size_t end = offset;
    if (end > source.size())
        end = source.size();

    for (std::size_t index = 0; index < end; ++index) {
        if (source[index] == '\n') {
            ++position.line;
            position.column = 1;
        } else {
            ++position.column;
        }
    }

    return position;
}

std::size_t estimate_node_reserve(std::string_view source) {
    if (source.empty())
        return 0;

    std::size_t legacy_reserve = source.size() / 4;
    if (source.size() < 64 * 1024)
        return legacy_reserve;

    std::size_t sample_size = source.size();
    if (sample_size > reserve_sample_bytes)
        sample_size = reserve_sample_bytes;

    std::size_t tokens = 0;
    std::size_t index = 0;
    while (index < sample_size) {
        char c = source[index];
        if (is_space(c) || c == ')') {
            ++index;
            continue;
        }

        if (c == ';' || c == '#') {
            while (index < sample_size && source[index] != '\n')
                ++index;
            continue;
        }

        if (c == '(') {
            ++tokens;
            ++index;
            continue;
        }

        if (c == '"') {
            ++tokens;
            ++index;
            while (index < sample_size) {
                char ch = source[index++];
                if (ch == '\\' && index < sample_size) {
                    ++index;
                    continue;
                }
                if (ch == '"')
                    break;
                if (ch == '\n' || ch == '\r')
                    break;
            }
            continue;
        }

        ++tokens;
        while (index < sample_size && !is_delimiter(source[index]))
            ++index;
    }

    if (tokens == 0)
        return reserve_min_nodes;

    double scaled = static_cast<double>(tokens) * static_cast<double>(source.size()) /
                    static_cast<double>(sample_size);
    std::size_t estimate = static_cast<std::size_t>(scaled * reserve_growth_slack) + reserve_slack_nodes;
    if (estimate < reserve_min_nodes)
        estimate = reserve_min_nodes;

    if (estimate * 2 > legacy_reserve)
        return legacy_reserve;

    return estimate;
}

class Parser {
  public:
    explicit Parser(std::string source) : storage(std::make_shared<ParseStorage>()) {
        storage->source = std::move(source);
        std::size_t node_reserve = estimate_node_reserve(storage->source);
        storage->nodes.reserve(node_reserve);
        text = storage->source;
    }

    ParseResult parse() {
        ParseResult result;
        result.storage = storage;

        while (true) {
            skip_space_and_comments();
            if (index >= text.size())
                break;

            std::uint32_t value = invalid_node;
            if (!parse_value(invalid_node, value, result.diagnostics))
                return result;
            result.roots.push_back(value);
        }

        result.ok = true;
        return result;
    }

  private:
    std::shared_ptr<ParseStorage> storage;
    std::string_view text;
    std::size_t index = 0;

    static std::uint32_t list_tail_child(const NodeData& list) {
        return list.text_offset;
    }

    static void set_list_tail_child(NodeData& list, std::uint32_t tail) {
        list.text_offset = tail;
    }

    std::uint32_t add_node(ValueType type,
                           TextStorage text_storage,
                           std::size_t text_offset,
                           std::size_t text_size,
                           std::uint32_t parent) {
        std::uint32_t node_index = static_cast<std::uint32_t>(storage->nodes.size());
        NodeData node;
        node.type = type;
        node.text_storage = text_storage;
        node.text_offset = static_cast<std::uint32_t>(text_offset);
        node.text_size = static_cast<std::uint32_t>(text_size);
        // Lists have no public text, so this field stores the parse-time child tail.
        if (type == ValueType::List)
            set_list_tail_child(node, invalid_node);
        storage->nodes.push_back(node);

        if (parent != invalid_node) {
            NodeData& parent_node = storage->nodes[parent];
            if (parent_node.first_child == invalid_node) {
                parent_node.first_child = node_index;
            } else {
                storage->nodes[list_tail_child(parent_node)].next_sibling = node_index;
            }
            set_list_tail_child(parent_node, node_index);
            increment_child_count(parent);
        }

        return node_index;
    }

    void increment_child_count(std::uint32_t parent) {
        NodeData& parent_node = storage->nodes[parent];
        if (parent_node.child_count < child_count_overflow_marker) {
            ++parent_node.child_count;
            return;
        }

        for (ChildCountOverflow& item : storage->child_count_overflows) {
            if (item.node == parent) {
                ++item.count;
                return;
            }
        }

        storage->child_count_overflows.push_back(
            ChildCountOverflow{parent, static_cast<std::uint32_t>(child_count_overflow_marker) + 1u});
    }

    void add_error(std::vector<Diagnostic>& diagnostics,
                   std::string message,
                   std::size_t error_offset) {
        SourcePosition position = position_for_offset(text, error_offset);
        diagnostics.push_back(
            Diagnostic{DiagnosticSeverity::Error, std::move(message), position.line, position.column});
    }

    void skip_space_and_comments() {
        while (index < text.size()) {
            switch (text[index]) {
                case ' ':
                case '\n':
                case '\r':
                case '\t':
                case '\v':
                case '\f':
                    ++index;
                    continue;
                case ';':
                case '#':
                    while (index < text.size() && text[index] != '\n')
                        ++index;
                    continue;
                default:
                    return;
            }
        }
    }

    bool parse_value(std::uint32_t parent,
                     std::uint32_t& out,
                     std::vector<Diagnostic>& diagnostics) {
        if (index >= text.size()) {
            add_error(diagnostics, "expected value", index);
            return false;
        }

        char c = text[index];
        if (c == '(')
            return parse_list(parent, out, diagnostics);
        if (c == ')') {
            add_error(diagnostics, "unexpected ')'", index);
            return false;
        }
        if (c == '"')
            return parse_string(parent, out, diagnostics);

        parse_atom(parent, out);
        return true;
    }

    bool parse_list(std::uint32_t parent,
                    std::uint32_t& out,
                    std::vector<Diagnostic>& diagnostics) {
        std::size_t start_offset = index;
        ++index;

        out = add_node(ValueType::List, TextStorage::Source, 0, 0, parent);

        while (true) {
            skip_space_and_comments();
            if (index >= text.size()) {
                add_error(diagnostics, "missing closing ')'", start_offset);
                return false;
            }
            if (text[index] == ')') {
                ++index;
                return true;
            }

            std::uint32_t child = invalid_node;
            if (!parse_value(out, child, diagnostics))
                return false;
        }
    }

    bool parse_string(std::uint32_t parent,
                      std::uint32_t& out,
                      std::vector<Diagnostic>& diagnostics) {
        std::size_t start_offset = index;
        ++index;

        std::size_t content_start = index;
        std::size_t scan = find_string_special(text, index);
        if (scan < text.size() && text[scan] == '"') {
            out = add_node(ValueType::String,
                           TextStorage::Source,
                           content_start,
                           scan - content_start,
                           parent);
            index = scan + 1;
            return true;
        }

        std::size_t decoded_start = prepare_decoded_string();
        if (scan > index) {
            storage->decoded_text.insert(storage->decoded_text.end(),
                                         text.data() + index,
                                         text.data() + scan);
            index = scan;
        }

        while (index < text.size()) {
            std::size_t special = find_string_special(text, index);
            if (special > index) {
                storage->decoded_text.insert(storage->decoded_text.end(),
                                             text.data() + index,
                                             text.data() + special);
                index = special;
            }
            if (index >= text.size())
                break;

            char ch = text[index];
            ++index;
            if (ch == '\\' && index < text.size()) {
                char esc = text[index];
                ++index;
                switch (esc) {
                    case 'n': storage->decoded_text.push_back('\n'); break;
                    case 'r': storage->decoded_text.push_back('\r'); break;
                    case 't': storage->decoded_text.push_back('\t'); break;
                    case '\\': storage->decoded_text.push_back('\\'); break;
                    case '"': storage->decoded_text.push_back('"'); break;
                    default: storage->decoded_text.push_back(esc); break;
                }
            } else if (ch == '"') {
                ++storage->decoded_string_count;
                out = add_node(ValueType::String,
                               TextStorage::Decoded,
                               decoded_start,
                               storage->decoded_text.size() - decoded_start,
                               parent);
                return true;
            } else {
                storage->decoded_text.push_back(ch);
            }
        }

        storage->decoded_text.resize(decoded_start);
        add_error(diagnostics, "unterminated string", start_offset);
        return false;
    }

    std::size_t prepare_decoded_string() {
        if (storage->decoded_text.capacity() == 0) {
            std::size_t reserve_size = storage->source.size() - (storage->source.size() / 4);
            if (reserve_size < 32)
                reserve_size = 32;
            storage->decoded_text.reserve(reserve_size);
        }

        return storage->decoded_text.size();
    }

    void parse_atom(std::uint32_t parent, std::uint32_t& out) {
        std::size_t start = index;
        index = find_atom_end(text, index);

        out = add_node(ValueType::Atom, TextStorage::Source, start, index - start, parent);
    }
};

} // namespace

bool looks_like_integer(std::string_view text) {
    if (text.empty())
        return false;

    std::size_t index = 0;
    if (text[index] == '+' || text[index] == '-') {
        if (text.size() == 1)
            return false;
        ++index;
    }

    bool digit = false;
    for (; index < text.size(); ++index) {
        if (!is_digit(text[index]))
            return false;
        digit = true;
    }

    return digit;
}

bool looks_like_float(std::string_view text) {
    if (text.empty())
        return false;

    bool dot = false;
    bool exp = false;
    bool digit = false;
    std::size_t index = 0;

    if (text[index] == '+' || text[index] == '-') {
        if (text.size() == 1)
            return false;
        ++index;
    }

    for (; index < text.size(); ++index) {
        char c = text[index];
        if (is_digit(c)) {
            digit = true;
            continue;
        }
        if (c == '.' && !dot && !exp) {
            dot = true;
            continue;
        }
        if ((c == 'e' || c == 'E') && !exp && digit) {
            exp = true;
            digit = false;
            if (index + 1 < text.size() && (text[index + 1] == '+' || text[index + 1] == '-'))
                ++index;
            continue;
        }
        return false;
    }

    return digit && (dot || exp);
}

ParseResult parse(std::string_view text) {
    Parser parser{std::string(text)};
    return parser.parse();
}

ParseResult parse_owned(std::string text) {
    Parser parser(std::move(text));
    return parser.parse();
}

} // namespace gsexp
