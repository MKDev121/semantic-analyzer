#include "LexicalValidation.hpp"

#include <cstddef>

bool validateLexicalStructure(const std::string& source, ErrorReporter& reporter) {
    std::size_t index = 0;
    int line = 1;
    int column = 1;
    const auto peek = [&](std::size_t offset = 0) {
        return index + offset < source.size() ? source[index + offset] : '\0';
    };
    const auto advance = [&]() {
        if (index >= source.size()) return;
        if (source[index++] == '\n') { ++line; column = 1; }
        else { ++column; }
    };

    while (index < source.size()) {
        if (peek() == '/' && peek(1) == '/') {
            while (index < source.size() && peek() != '\n') advance();
            continue;
        }
        if (peek() == '/' && peek(1) == '*') {
            const int startLine = line;
            const int startColumn = column;
            advance();
            advance();
            bool closed = false;
            while (index < source.size()) {
                if (peek() == '*' && peek(1) == '/') {
                    advance();
                    advance();
                    closed = true;
                    break;
                }
                advance();
            }
            if (!closed) {
                reporter.reportError(startLine, startColumn, "Unterminated block comment");
            }
            continue;
        }
        if (peek() == '\'') {
            const int startLine = line;
            const int startColumn = column;
            advance();
            bool validEscape = true;
            bool validCharacter = true;
            bool hasCharacter = false;
            if (index < source.size() && peek() != '\'' && peek() != '\n' && peek() != '\r') {
                hasCharacter = true;
                if (peek() == '\\') {
                    advance();
                    if (index < source.size() && peek() != '\n' && peek() != '\r') {
                        validEscape = std::string("nrt0\\'").find(peek()) != std::string::npos;
                        advance();
                    } else {
                        validEscape = false;
                    }
                } else {
                    validCharacter = static_cast<unsigned char>(peek()) < 128 && peek() != '\0';
                    advance();
                }
            }
            const bool singleCharacter = hasCharacter && peek() == '\'';
            while (index < source.size() && peek() != '\'' && peek() != '\n' && peek() != '\r') advance();
            if (index >= source.size() || peek() != '\'') {
                reporter.reportError(startLine, startColumn, "Unterminated character literal");
            } else {
                advance();
                if (!singleCharacter) {
                    reporter.reportError(startLine, startColumn, "Character literal must contain exactly one character");
                } else if (!validEscape) {
                    reporter.reportError(startLine, startColumn, "Unsupported escape sequence in character literal");
                } else if (!validCharacter) {
                    reporter.reportError(startLine, startColumn, "Character literals must contain an ASCII character");
                }
            }
            continue;
        }
        // Keep unsupported increment/decrement spellings from being mistaken
        // for two unary operators by the deliberately unchanged 25% lexer.
        if ((peek() == '+' && peek(1) == '+') || (peek() == '-' && peek(1) == '-')) {
            const std::string operation(2, peek());
            reporter.reportError(line, column, "Unsupported operator '" + operation + "'");
            advance();
            advance();
            continue;
        }
        if (static_cast<unsigned char>(peek()) >= 128 || peek() == '\0') {
            reporter.reportError(line, column, "Unsupported non-ASCII or NUL source character");
            // One diagnostic for a consecutive group of unsupported bytes.
            do { advance(); }
            while (index < source.size() && (static_cast<unsigned char>(peek()) >= 128 || peek() == '\0'));
            continue;
        }
        advance();
    }
    return !reporter.hasErrors();
}
