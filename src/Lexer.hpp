#pragma once

#include "Token.hpp"

#include <cstddef>
#include <string>
#include <vector>

class Lexer {
    public:
        explicit Lexer(std::string source); //prevents automatic conversion from a string to a lexer

        std::vector<Token> tokenize(); //Vector of token datatype. 
        Token nextToken();          //Return the next single token

    private:
        std::string src;                //source text
        std::size_t index = 0;          //index points to i.
        int line = 1;
        int column = 1;

        //no modifications of non static data members of the class.

        char peek() const;                 //Returns the current character without moving the position.
        char peekNext() const;              //Returns the character one position ahead, without moving.
        char advance();                     //Returns the current character and moves forward.
        bool match(char expected);           //Checks whether the current character matches. If yes then consumes the character and returns true. Else no changes and false.
        bool isAtEnd() const;               //Returns true when the position has reached the end of the input

        void skipWhitespaceAndComments();       //Skips past spaces, tabs, newlines and comments. These don’t become tokens.
        Token makeToken(TokenType type, const std::string& text, int startCol); //Creates and returns a token using the supplied category, text and starting column. It uses the lexer’s current line number.
        Token identifierOrKeyword(int startCol);        //Reads a complete word, then checks whether it’s a keyword.
        Token number(int startCol);                     //Reads a number and determines its category.
        Token character(int startCol);                  //It returns a CHAR_LITERAL token. Takesn input of startcol as it helps in that.
};
