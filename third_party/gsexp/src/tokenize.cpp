#include "gsexp/sexp.hpp"

#include <utility>

namespace gsexp {
namespace {

bool is_space(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\f';
}

bool is_delimiter(char c) {
    return is_space(c) || c == '(' || c == ')';
}

void advance_position(char c, int& line, int& column) {
    if (c == '\n') {
        ++line;
        column = 1;
        return;
    }

    ++column;
}

void add_diagnostic(std::vector<Diagnostic>* diagnostics,
                    DiagnosticSeverity severity,
                    std::string message,
                    int line,
                    int column) {
    if (!diagnostics)
        return;

    diagnostics->push_back(Diagnostic{severity, std::move(message), line, column});
}

} // namespace

std::vector<Token> tokenize(std::string_view text, std::vector<Diagnostic>* diagnostics) {
    std::vector<Token> tokens;
    std::size_t index = 0;
    int line = 1;
    int column = 1;

    while (index < text.size()) {
        char c = text[index];

        if (is_space(c)) {
            advance_position(c, line, column);
            ++index;
            continue;
        }

        if (c == ';' || c == '#') {
            while (index < text.size() && text[index] != '\n') {
                advance_position(text[index], line, column);
                ++index;
            }
            continue;
        }

        int token_line = line;
        int token_column = column;

        if (c == '(') {
            tokens.push_back(Token{TokenType::LParen, "(", token_line, token_column});
            advance_position(c, line, column);
            ++index;
            continue;
        }

        if (c == ')') {
            tokens.push_back(Token{TokenType::RParen, ")", token_line, token_column});
            advance_position(c, line, column);
            ++index;
            continue;
        }

        if (c == '"') {
            advance_position(c, line, column);
            ++index;

            std::string buffer;
            bool closed = false;
            while (index < text.size()) {
                char ch = text[index];
                advance_position(ch, line, column);
                ++index;

                if (ch == '\\' && index < text.size()) {
                    char esc = text[index];
                    advance_position(esc, line, column);
                    ++index;
                    switch (esc) {
                        case 'n': buffer.push_back('\n'); break;
                        case 'r': buffer.push_back('\r'); break;
                        case 't': buffer.push_back('\t'); break;
                        case '\\': buffer.push_back('\\'); break;
                        case '"': buffer.push_back('"'); break;
                        default: buffer.push_back(esc); break;
                    }
                } else if (ch == '"') {
                    closed = true;
                    break;
                } else {
                    buffer.push_back(ch);
                }
            }

            if (!closed) {
                add_diagnostic(diagnostics,
                               DiagnosticSeverity::Error,
                               "unterminated string",
                               token_line,
                               token_column);
                return tokens;
            }

            tokens.push_back(Token{TokenType::String, buffer, token_line, token_column});
            continue;
        }

        std::size_t start = index;
        while (index < text.size()) {
            char ch = text[index];
            if (is_delimiter(ch))
                break;

            advance_position(ch, line, column);
            ++index;
        }

        tokens.push_back(Token{
            TokenType::Atom,
            std::string(text.substr(start, index - start)),
            token_line,
            token_column,
        });
    }

    return tokens;
}

std::string quote_string(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 2);
    out.push_back('"');

    for (char c : text) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }

    out.push_back('"');
    return out;
}

} // namespace gsexp
