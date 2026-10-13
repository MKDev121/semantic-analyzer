#include "Lexer.hpp"

#include <cctype>
#include <unordered_map>
#include <utility>

using namespace std;

static const unordered_map<string, TokenType> keywords = {      //Similar to python dictionary. KV pair.
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

Lexer::Lexer(string source) {
    this->src = move(source);  //Constructor call.
}

char Lexer::peek() const {      //peek() next character.
    if (isAtEnd()) {        //If end , the return null else return the current character. WITHOUT MOVING
        return '\0';
    }
    return src[index];
}

char Lexer::peekNext() const {
    if (index + 1 >= src.size()) {      //Return null if index is out of range or at last. Else return the next character
        return '\0';                       //WITHOUT MOVING.
    }
    return src[index + 1];
}

char Lexer::advance() {
    if (isAtEnd()) {            
        return '\0';        //Return null at end
    }

    char c = src[index++];      //Reads current character and advances forward.

    if (c == '\n') {            //Newline found. Increase line count and reset column. Else increase column only.
        line++;
        column = 1;
    } 
    else {
        column++;
    }

    return c;               //Return current character and move forward
}

bool Lexer::match(char expected) {
    if (isAtEnd() || src[index] != expected) {      //If the current character is the one I expect consume it.
        return false;
    }

    advance();
    
    return true;
}

bool Lexer::isAtEnd() const {
    return index >= src.size();             //Checks whether we’ve read all the input.
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {            //While traversing till end of input.
        char c = peek();            //Current character
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {          //Esccape seq. check only advance and consume it. 
            advance();
        } 
        
        else if (c == '/' && peekNext() == '/') {       //Consume single line comment. 
            while (!isAtEnd() && peek() != '\n') {      //Consume everything up to the new line.
                advance();
            }
        } 
        
        else if (c == '/' && peekNext() == '*') {           // Multi-line comment. Here consume both /*. Then search for newline and traverse multiple lines until encountering */.
            advance(); // consume /
            advance(); // consume *
            while (!isAtEnd()) {
                if (peek() == '*' && peekNext() == '/') {
                    advance(); // consume *
                    advance(); // consume /
                    break;
                }

                advance();  
            }
        } 
        
        else {
            break;
        }
    }
}

Token Lexer::makeToken(TokenType type, const string& text, int startCol) {
    return Token(type, text, line, startCol);       //Creates a token object by taking type, text and starting column as input.
}

Token Lexer::identifierOrKeyword(int startCol) {
    string text;                //It reads a word and decides whether it’s a keyword like int or an identifier like count.

    while (!isAtEnd() && (isalnum(peek()) || peek() == '_')) {      //Check alphabets or underscores.
        text += advance();      //Build the identifier.
    }

    auto it = keywords.find(text);  //Search the keyword table.
    
    if (it != keywords.end()) {
        return Token(it->second, text, line, startCol); //If found in table, return keyword token.
    }

    return Token(TokenType::IDENTIFIER, text, line, startCol);  //If not, return identifier token.
}

Token Lexer::number(int startCol) {     //reads a number and decides whether a decimal or not.
    string text;                    
    bool isFloat = false;                   

    while (!isAtEnd() && isdigit(peek())) {
        text += advance();                      //Build string integer.
    }

    if (peek() == '.' && isdigit(peekNext())) {         
        isFloat = true;                         //Set flag to true as . was found.
        text += advance();                      //Move forward until end of line.
        while (!isAtEnd() && isdigit(peek())) {         
            text += advance();          
        }
    }

    //Use isFloat flag to decide float or integer.
    return Token(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL, text, line, startCol);
}

Token Lexer::character(int startCol) {  
    advance();              // consume opening quote/
    string text;
    
    if (peek() == '\\') {       
        text += advance();  //Store backslash.
    
        if (!isAtEnd()) {       
            text += advance();
        }
    } 
    
    else if (!isAtEnd() && peek() != '\'') {
        text += advance();      //Read till end.

    }
    
    if (peek() == '\'') {
        advance();  // consume closing quote 
    
    }

    //Return character token.
    return Token(TokenType::CHAR_LITERAL, text, line, startCol);
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();    //Skip spaces.

    if (isAtEnd()) {    //Check whether input is finished. Send EOF token.
        return Token(TokenType::TOK_EOF, "", line, column);
    }

    int startCol = column;  //Remember starting position and inspect character.
    char c = peek();

    if (isalpha(c) || c == '_') {       //For identifier.
        return identifierOrKeyword(startCol);
    }

    if (isdigit(c)) {                   //For digits.
        return number(startCol);
    }

    if (c == '\'') {                    //For strings.
        return character(startCol);
    }

    // Operators and delimiters
    advance(); //Consume.

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
                return Token(TokenType::EQ, "==", line, startCol);  //For = and == difference.
            }

            return Token(TokenType::ASSIGN, "=", line, startCol);
        case '!':
            if (match('=')) {
                return Token(TokenType::NEQ, "!=", line, startCol);     //For ! and != 
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
            return Token(TokenType::TOK_UNKNOWN, string(1, c), line, startCol);     //Default create one string copy of c.
    }
}

vector<Token> Lexer::tokenize() {       //Collection of tokens. 
    vector<Token> tokens;   //Empty list.

    while (true) {  
        Token tok = nextToken();        //Read one token. Append.
        tokens.push_back(tok);
    
        if (tok.type == TokenType::TOK_EOF) {   //Stop at EOF token.
            break;
        }
    }
    
    return tokens;
}
