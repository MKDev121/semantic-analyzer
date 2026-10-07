#pragma once

#include "Token.hpp"

#include <cstddef>
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> tokenize();
    Token nextToken();

private:
    std::string src;
    std::size_t index = 0;
    int line = 1;
    int column = 1;

    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);
    bool isAtEnd() const;

    void skipWhitespaceAndComments();
    Token makeToken(TokenType type, const std::string& text, int startCol);
    Token identifierOrKeyword(int startCol);
    Token number(int startCol);
    Token character(int startCol);
};
