#pragma once

#include <string>
#include <utility>

enum class TokenType {
    // Keywords
    KEYWORD_INT,
    KEYWORD_FLOAT,
    KEYWORD_BOOL,
    KEYWORD_CHAR,
    KEYWORD_VOID,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    KEYWORD_RETURN,
    KEYWORD_TRUE,
    KEYWORD_FALSE,

    // Identifiers & Literals
    IDENTIFIER,
    INT_LITERAL,
    FLOAT_LITERAL,
    CHAR_LITERAL,
    BOOL_LITERAL,

    // Operators
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    PERCENT,        // %
    ASSIGN,         // =
    EQ,             // ==
    NEQ,            // !=
    LT,             // <
    LE,             // <=
    GT,             // >
    GE,             // >=
    AND,            // &&
    OR,             // ||
    NOT,            // !

    // Delimiters
    SEMICOLON,      // ;
    COMMA,          // ,
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]

    // End of File / Error
    TOK_EOF,
    TOK_UNKNOWN
};

struct Token {
    TokenType type;
    std::string text;
    int line;
    int column;

    Token(TokenType type = TokenType::TOK_EOF, std::string text = "", int line = 1, int column = 1)
        : type(type),
          text(std::move(text)),
          line(line),
          column(column) {
    }

    static std::string typeName(TokenType t) {
        switch (t) {
            case TokenType::KEYWORD_INT:
                return "KEYWORD_INT";
            case TokenType::KEYWORD_FLOAT:
                return "KEYWORD_FLOAT";
            case TokenType::KEYWORD_BOOL:
                return "KEYWORD_BOOL";
            case TokenType::KEYWORD_CHAR:
                return "KEYWORD_CHAR";
            case TokenType::KEYWORD_VOID:
                return "KEYWORD_VOID";
            case TokenType::KEYWORD_IF:
                return "KEYWORD_IF";
            case TokenType::KEYWORD_ELSE:
                return "KEYWORD_ELSE";
            case TokenType::KEYWORD_WHILE:
                return "KEYWORD_WHILE";
            case TokenType::KEYWORD_RETURN:
                return "KEYWORD_RETURN";
            case TokenType::KEYWORD_TRUE:
                return "KEYWORD_TRUE";
            case TokenType::KEYWORD_FALSE:
                return "KEYWORD_FALSE";
            case TokenType::IDENTIFIER:
                return "IDENTIFIER";
            case TokenType::INT_LITERAL:
                return "INT_LITERAL";
            case TokenType::FLOAT_LITERAL:
                return "FLOAT_LITERAL";
            case TokenType::CHAR_LITERAL:
                return "CHAR_LITERAL";
            case TokenType::BOOL_LITERAL:
                return "BOOL_LITERAL";
            case TokenType::PLUS:
                return "+";
            case TokenType::MINUS:
                return "-";
            case TokenType::STAR:
                return "*";
            case TokenType::SLASH:
                return "/";
            case TokenType::PERCENT:
                return "%";
            case TokenType::ASSIGN:
                return "=";
            case TokenType::EQ:
                return "==";
            case TokenType::NEQ:
                return "!=";
            case TokenType::LT:
                return "<";
            case TokenType::LE:
                return "<=";
            case TokenType::GT:
                return ">";
            case TokenType::GE:
                return ">=";
            case TokenType::AND:
                return "&&";
            case TokenType::OR:
                return "||";
            case TokenType::NOT:
                return "!";
            case TokenType::SEMICOLON:
                return ";";
            case TokenType::COMMA:
                return ",";
            case TokenType::LPAREN:
                return "(";
            case TokenType::RPAREN:
                return ")";
            case TokenType::LBRACE:
                return "{";
            case TokenType::RBRACE:
                return "}";
            case TokenType::LBRACKET:
                return "[";
            case TokenType::RBRACKET:
                return "]";
            case TokenType::TOK_EOF:
                return "EOF";
            case TokenType::TOK_UNKNOWN:
                return "UNKNOWN";
        }
        return "UNKNOWN";
    }
};
