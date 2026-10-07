#include "Lexer.hpp"

#include <cctype>
#include <unordered_map>
#include <utility>

using namespace std;

static const unordered_map<string, TokenType> keywords = {
    {"int", TokenType::KEYWORD_INT},
    {"float", TokenType::KEYWORD_FLOAT},
    {"bool", TokenType::KEYWORD_BOOL},
    {"char", TokenType::KEYWORD_CHAR},
    {"void", TokenType::KEYWORD_VOID},
    {"if", TokenType::KEYWORD_IF},
    {"else", TokenType::KEYWORD_ELSE},
    {"while", TokenType::KEYWORD_WHILE},
    {"return", TokenType::KEYWORD_RETURN},
    {"true", TokenType::KEYWORD_TRUE},
    {"false", TokenType::KEYWORD_FALSE}
};

Lexer::Lexer(string source) : src(move(source)) {
}

char Lexer::peek() const {
    if (isAtEnd()) {
        return '\0';
    }
    return src[index];
}

char Lexer::peekNext() const {
    if (index + 1 >= src.size()) {
        return '\0';
    }
    return src[index + 1];
}

char Lexer::advance() {
    if (isAtEnd()) {
        return '\0';
    }
    char c = src[index++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (isAtEnd() || src[index] != expected) {
        return false;
    }
    advance();
    return true;
}

bool Lexer::isAtEnd() const {
    return index >= src.size();
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peekNext() == '/') {
            // Single-line comment
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else if (c == '/' && peekNext() == '*') {
            // Multi-line comment
            advance(); // consume '/'
            advance(); // consume '*'
            while (!isAtEnd()) {
                if (peek() == '*' && peekNext() == '/') {
                    advance(); // consume '*'
                    advance(); // consume '/'
                    break;
                }
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::makeToken(TokenType type, const string& text, int startCol) {
    return Token(type, text, line, startCol);
}

Token Lexer::identifierOrKeyword(int startCol) {
    string text;
    while (!isAtEnd() && (isalnum(peek()) || peek() == '_')) {
        text += advance();
    }
    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return Token(it->second, text, line, startCol);
    }
    return Token(TokenType::IDENTIFIER, text, line, startCol);
}

Token Lexer::number(int startCol) {
    string text;
    bool isFloat = false;

    while (!isAtEnd() && isdigit(peek())) {
        text += advance();
    }

    if (peek() == '.' && isdigit(peekNext())) {
        isFloat = true;
        text += advance(); // consume '.'
        while (!isAtEnd() && isdigit(peek())) {
            text += advance();
        }
    }

    return Token(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL, text, line, startCol);
}

Token Lexer::character(int startCol) {
    advance(); // consume opening quote '\''
    string text;
    if (peek() == '\\') {
        text += advance();
        if (!isAtEnd()) {
            text += advance();
        }
    } else if (!isAtEnd() && peek() != '\'') {
        text += advance();
    }
    if (peek() == '\'') {
        advance(); // consume closing quote '\''
    }
    return Token(TokenType::CHAR_LITERAL, text, line, startCol);
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    if (isAtEnd()) {
        return Token(TokenType::TOK_EOF, "", line, column);
    }

    int startCol = column;
    char c = peek();

    if (isalpha(c) || c == '_') {
        return identifierOrKeyword(startCol);
    }

    if (isdigit(c)) {
        return number(startCol);
    }

    if (c == '\'') {
        return character(startCol);
    }

    // Operators and delimiters
    advance();
    switch (c) {
        case '+':
            return Token(TokenType::PLUS, "+", line, startCol);
        case '-':
            return Token(TokenType::MINUS, "-", line, startCol);
        case '*':
            return Token(TokenType::STAR, "*", line, startCol);
        case '/':
            return Token(TokenType::SLASH, "/", line, startCol);
        case '%':
            return Token(TokenType::PERCENT, "%", line, startCol);
        case ';':
            return Token(TokenType::SEMICOLON, ";", line, startCol);
        case ',':
            return Token(TokenType::COMMA, ",", line, startCol);
        case '(':
            return Token(TokenType::LPAREN, "(", line, startCol);
        case ')':
            return Token(TokenType::RPAREN, ")", line, startCol);
        case '{':
            return Token(TokenType::LBRACE, "{", line, startCol);
        case '}':
            return Token(TokenType::RBRACE, "}", line, startCol);
        case '[':
            return Token(TokenType::LBRACKET, "[", line, startCol);
        case ']':
            return Token(TokenType::RBRACKET, "]", line, startCol);
        case '=':
            if (match('=')) {
                return Token(TokenType::EQ, "==", line, startCol);
            }
            return Token(TokenType::ASSIGN, "=", line, startCol);
        case '!':
            if (match('=')) {
                return Token(TokenType::NEQ, "!=", line, startCol);
            }
            return Token(TokenType::NOT, "!", line, startCol);
        case '<':
            if (match('=')) {
                return Token(TokenType::LE, "<=", line, startCol);
            }
            return Token(TokenType::LT, "<", line, startCol);
        case '>':
            if (match('=')) {
                return Token(TokenType::GE, ">=", line, startCol);
            }
            return Token(TokenType::GT, ">", line, startCol);
        case '&':
            if (match('&')) {
                return Token(TokenType::AND, "&&", line, startCol);
            }
            return Token(TokenType::TOK_UNKNOWN, "&", line, startCol);
        case '|':
            if (match('|')) {
                return Token(TokenType::OR, "||", line, startCol);
            }
            return Token(TokenType::TOK_UNKNOWN, "|", line, startCol);
        default:
            return Token(TokenType::TOK_UNKNOWN, string(1, c), line, startCol);
    }
}

vector<Token> Lexer::tokenize() {
    vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::TOK_EOF) {
            break;
        }
    }
    return tokens;
}
