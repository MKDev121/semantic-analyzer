#include "Parser.hpp"

#include <stdexcept>
#include <utility>

Parser::Parser(std::vector<Token> inputTokens, ErrorReporter& reporter)
    : tokens(std::move(inputTokens)), reporter(reporter) {
    // The lexer normally supplies EOF. Keep this class safe for empty token
    // streams and callers that construct tokens themselves, too.
    if (tokens.empty()) {
        tokens.emplace_back(TokenType::TOK_EOF, "", 1, 1);
    } else if (tokens.back().type != TokenType::TOK_EOF) {
        const Token& last = tokens.back();
        tokens.emplace_back(TokenType::TOK_EOF, "", last.line,
                            last.column + static_cast<int>(last.text.size()));
    }
}

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::previous() const {
    return tokens[current == 0 ? 0 : current - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::TOK_EOF;
}

Token Parser::advance() {
    Token token = peek();
    if (!isAtEnd()) ++current;
    return token;
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (match(type)) return true;
    }
    return false;
}

[[noreturn]] void Parser::fail(const Token& token, const std::string& message,
                             const std::string& correction) {
    const std::string actual = token.type == TokenType::TOK_EOF
        ? "end of file" : "'" + token.text + "'";
    reporter.reportError(token.line, token.column,
                         message + " (got " + actual + ")", correction);
    throw ParseError{};
}

Token Parser::consume(TokenType type, const std::string& message,
                      const std::string& correction) {
    if (check(type)) return advance();
    fail(peek(), message, correction);
}

void Parser::synchronize(std::size_t failedStart, bool inBlock) {
    // Never consume the closing brace of the block that owns this recovery.
    // Never return at the failed token unless the failed production consumed
    // something; these two rules prevent both runaway blocks and infinite loops.
    int parentheses = 0;
    int brackets = 0;
    int braces = 0;
    auto trackDelimiter = [&](TokenType type) {
        if (type == TokenType::LPAREN) ++parentheses;
        if (type == TokenType::RPAREN && parentheses > 0) --parentheses;
        if (type == TokenType::LBRACKET) ++brackets;
        if (type == TokenType::RBRACKET && brackets > 0) --brackets;
        if (type == TokenType::LBRACE) ++braces;
        if (type == TokenType::RBRACE && braces > 0) --braces;
    };
    for (std::size_t index = failedStart; index < current; ++index) {
        trackDelimiter(tokens[index].type);
    }

    while (!isAtEnd()) {
        if (check(TokenType::RBRACE) && braces == 0) {
            if (!inBlock) advance();
            return;
        }

        if (current > failedStart && parentheses == 0 && brackets == 0 && braces == 0) {
            if (isTypeStart(peek().type)) return;
            if (inBlock && (check(TokenType::KEYWORD_IF) ||
                            check(TokenType::KEYWORD_WHILE) ||
                            check(TokenType::KEYWORD_RETURN))) return;
        }

        const Token skipped = advance();
        trackDelimiter(skipped.type);
        // Discard the whole malformed body/initializer, rather than treating
        // declarations inside it as new top-level declarations.
        if (skipped.type == TokenType::RBRACE && braces == 0) {
            match(TokenType::SEMICOLON);
            return;
        }
        if (skipped.type == TokenType::SEMICOLON && braces == 0) return;
    }
}

bool Parser::isTypeStart(TokenType type) const {
    return type == TokenType::KEYWORD_INT ||
           type == TokenType::KEYWORD_FLOAT ||
           type == TokenType::KEYWORD_BOOL ||
           type == TokenType::KEYWORD_CHAR ||
           type == TokenType::KEYWORD_VOID;
}

TypePtr Parser::parseType() {
    if (match(TokenType::KEYWORD_INT)) return Type::getInt();
    if (match(TokenType::KEYWORD_FLOAT)) return Type::getFloat();
    if (match(TokenType::KEYWORD_BOOL)) return Type::getBool();
    if (match(TokenType::KEYWORD_CHAR)) return Type::getChar();
    if (match(TokenType::KEYWORD_VOID)) return Type::getVoid();
    fail(peek(), "Expected type specifier (int, float, bool, char, void)",
         "Give each declaration or parameter an explicit type.");
}

std::unique_ptr<ProgramNode> Parser::parseProgram() {
    auto program = std::make_unique<ProgramNode>(1, 1);
    while (!isAtEnd()) {
        const std::size_t start = current;
        try {
            program->declarations.push_back(parseTopLevelDecl());
        } catch (const ParseError&) {
            synchronize(start, false);
        }
    }
    return program;
}

std::unique_ptr<ASTNode> Parser::parseTopLevelDecl() {
    if (!isTypeStart(peek().type)) {
        fail(peek(), "Expected top-level declaration (variable or function)",
             "Place executable statements inside a function body.");
    }

    TypePtr type = parseType();
    const Token name = consume(TokenType::IDENTIFIER, "Expected identifier after type",
                               "Add a variable or function name after the type.");
    if (check(TokenType::LPAREN)) return parseFunctionDecl(type, name);
    return parseVarDecl(type, name);
}

std::unique_ptr<FunctionDeclNode> Parser::parseFunctionDecl(TypePtr returnType,
                                                          const Token& name) {
    consume(TokenType::LPAREN, "Expected '(' after function name");
    auto parameters = parseParamList();
    consume(TokenType::RPAREN, "Expected ')' after parameter list",
            "Close the parameter list with ')'.");
    if (!check(TokenType::LBRACE)) {
        fail(peek(), "Expected '{' to begin function body",
             "Provide a function definition with a body in braces; prototypes are unsupported.");
    }
    auto body = parseBlock();
    return std::make_unique<FunctionDeclNode>(name.line, name.column, returnType,
                                              name.text, std::move(parameters), std::move(body));
}

std::vector<ParamNode> Parser::parseParamList() {
    std::vector<ParamNode> parameters;
    if (check(TokenType::RPAREN)) return parameters;

    do {
        TypePtr type = parseType();
        if (type->isVoid() && check(TokenType::RPAREN)) {
            fail(previous(), "Use an empty parameter list for a function with no parameters",
                 "Write '()' instead of '(void)'.");
        }
        const Token name = consume(TokenType::IDENTIFIER, "Expected parameter name",
                                   "Write each parameter as a type followed by its name.");
        if (check(TokenType::LBRACKET)) {
            fail(peek(), "Array parameters are not supported",
                 "Pass individual scalar values; declare fixed-size arrays inside a function or globally.");
        }
        parameters.emplace_back(name.line, name.column, type, name.text);
    } while (match(TokenType::COMMA));
    return parameters;
}

std::unique_ptr<VarDeclNode> Parser::parseVarDecl(TypePtr type, const Token& name) {
    bool isArray = false;
    int arraySize = 0;
    if (match(TokenType::LBRACKET)) {
        isArray = true;
        const Token size = consume(TokenType::INT_LITERAL,
                                   "Expected integer literal for array size",
                                   "Use a positive fixed size, for example 'int values[5];'.");
        std::size_t parsed = 0;
        try {
            arraySize = std::stoi(size.text, &parsed);
        } catch (const std::invalid_argument&) {
            fail(size, "Invalid array size literal", "Use a positive integer literal.");
        } catch (const std::out_of_range&) {
            fail(size, "Array size exceeds the supported integer range",
                 "Use an array size between 1 and 2147483647.");
        }
        if (parsed != size.text.size()) {
            fail(size, "Invalid array size literal", "Use a positive integer literal.");
        }
        if (arraySize <= 0) {
            fail(size, "Array size must be greater than 0", "Choose a positive fixed array size.");
        }
        consume(TokenType::RBRACKET, "Expected ']' after array size",
                "Close the array size with ']'.");
        if (check(TokenType::LBRACKET)) {
            fail(peek(), "Multidimensional arrays are not supported",
                 "Use one fixed-size array dimension.");
        }
    }

    std::unique_ptr<ExprNode> initializer;
    if (match(TokenType::ASSIGN)) {
        if (isArray) {
            fail(previous(), "Direct array initialization with '=' is not supported",
                 "Declare the array first, then assign its elements individually.");
        }
        initializer = parseExpression();
    }
    if (check(TokenType::COMMA)) {
        fail(peek(), "Multiple variables in one declaration are not supported",
             "Write one variable declaration per statement.");
    }
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration",
            "End the variable declaration with ';'.");

    TypePtr declaredType = isArray ? Type::makeArray(type, arraySize) : type;
    return std::make_unique<VarDeclNode>(name.line, name.column, declaredType, name.text,
                                        std::move(initializer), isArray, arraySize);
}

std::unique_ptr<BlockNode> Parser::parseBlock() {
    const Token opening = consume(TokenType::LBRACE, "Expected '{'");
    auto block = std::make_unique<BlockNode>(opening.line, opening.column);
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        const std::size_t start = current;
        try {
            block->statements.push_back(parseStatement());
        } catch (const ParseError&) {
            synchronize(start, true);
        }
    }
    consume(TokenType::RBRACE, "Expected '}' after block",
            "Close the block opened on line " + std::to_string(opening.line) + " with '}'.");
    return block;
}

std::unique_ptr<StmtNode> Parser::parseStatement() {
    if (check(TokenType::LBRACE)) return parseBlock();
    if (check(TokenType::KEYWORD_IF)) return parseIfStatement();
    if (check(TokenType::KEYWORD_WHILE)) return parseWhileStatement();
    if (check(TokenType::KEYWORD_RETURN)) return parseReturnStatement();
    if (check(TokenType::RBRACE)) {
        fail(peek(), "Expected a statement before '}'",
             "Provide a statement or an empty block '{}' for the control-flow body.");
    }
    if (isAtEnd()) {
        fail(peek(), "Expected a statement before end of file",
             "Provide a statement or a block after the control-flow condition.");
    }
    if (check(TokenType::KEYWORD_ELSE)) {
        fail(peek(), "Unexpected 'else' without a matching 'if'",
             "Place 'else' immediately after the statement or block belonging to an 'if'.");
    }
    if (isTypeStart(peek().type)) {
        TypePtr type = parseType();
        const Token name = consume(TokenType::IDENTIFIER, "Expected identifier after type",
                                   "Add a variable name after the type.");
        if (check(TokenType::LPAREN)) {
            fail(peek(), "Nested function declarations are not supported",
                 "Define functions at the top level, outside other function bodies.");
        }
        return parseVarDecl(type, name);
    }
    return parseAssignmentOrExpr();
}

std::unique_ptr<IfStmtNode> Parser::parseIfStatement() {
    const Token keyword = consume(TokenType::KEYWORD_IF, "Expected 'if'");
    consume(TokenType::LPAREN, "Expected '(' after 'if'", "Enclose the condition in parentheses.");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after condition", "Close the condition with ')'.");
    auto thenBranch = parseStatement();
    std::unique_ptr<StmtNode> elseBranch;
    if (match(TokenType::KEYWORD_ELSE)) elseBranch = parseStatement();
    return std::make_unique<IfStmtNode>(keyword.line, keyword.column, std::move(condition),
                                       std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<WhileStmtNode> Parser::parseWhileStatement() {
    const Token keyword = consume(TokenType::KEYWORD_WHILE, "Expected 'while'");
    consume(TokenType::LPAREN, "Expected '(' after 'while'", "Enclose the condition in parentheses.");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after condition", "Close the condition with ')'.");
    auto body = parseStatement();
    return std::make_unique<WhileStmtNode>(keyword.line, keyword.column,
                                          std::move(condition), std::move(body));
}

std::unique_ptr<ReturnStmtNode> Parser::parseReturnStatement() {
    const Token keyword = consume(TokenType::KEYWORD_RETURN, "Expected 'return'");
    std::unique_ptr<ExprNode> value;
    if (!check(TokenType::SEMICOLON)) value = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after return statement",
            "End the return statement with ';'.");
    return std::make_unique<ReturnStmtNode>(keyword.line, keyword.column, std::move(value));
}

std::unique_ptr<StmtNode> Parser::parseAssignmentOrExpr() {
    const Token start = peek();
    auto expression = parseExpression();
    if (match(TokenType::ASSIGN)) {
        const Token assignment = previous();
        auto* variable = dynamic_cast<VariableExprNode*>(expression.get());
        auto* element = dynamic_cast<ArrayAccessExprNode*>(expression.get());
        if (!variable && !element) {
            fail(assignment, "Invalid assignment target",
                 "Assign to a variable or a single array element, such as 'x = 3;' or 'values[0] = 3;'.");
        }
        const std::string name = variable ? variable->name : element->name;
        std::unique_ptr<ExprNode> index;
        if (element) index = std::move(element->indexExpr);
        auto rhs = parseExpression();
        if (check(TokenType::ASSIGN)) {
            fail(peek(), "Chained assignments are not supported",
                 "Write each assignment as a separate statement.");
        }
        consume(TokenType::SEMICOLON, "Expected ';' after assignment",
                "End the assignment with ';'.");
        return std::make_unique<AssignStmtNode>(start.line, start.column, name,
                                               std::move(index), std::move(rhs));
    }
    consume(TokenType::SEMICOLON, "Expected ';' after expression statement",
            "End the expression statement with ';'.");
    return std::make_unique<ExprStmtNode>(start.line, start.column, std::move(expression));
}

std::unique_ptr<ExprNode> Parser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<ExprNode> Parser::parseLogicalOr() {
    auto expression = parseLogicalAnd();
    while (match(TokenType::OR)) {
        const Token op = previous();
        auto right = parseLogicalAnd();
        expression = std::make_unique<BinaryExprNode>(op.line, op.column, op.type, op.text,
                                                     std::move(expression), std::move(right));
    }
    return expression;
}

std::unique_ptr<ExprNode> Parser::parseLogicalAnd() {
    auto expression = parseEquality();
    while (match(TokenType::AND)) {
        const Token op = previous();
        auto right = parseEquality();
        expression = std::make_unique<BinaryExprNode>(op.line, op.column, op.type, op.text,
                                                     std::move(expression), std::move(right));
    }
    return expression;
}

std::unique_ptr<ExprNode> Parser::parseEquality() {
    auto expression = parseRelational();
    while (match({TokenType::EQ, TokenType::NEQ})) {
        const Token op = previous();
        auto right = parseRelational();
        expression = std::make_unique<BinaryExprNode>(op.line, op.column, op.type, op.text,
                                                     std::move(expression), std::move(right));
    }
    return expression;
}

std::unique_ptr<ExprNode> Parser::parseRelational() {
    auto expression = parseAdditive();
    while (match({TokenType::LT, TokenType::LE, TokenType::GT, TokenType::GE})) {
        const Token op = previous();
        auto right = parseAdditive();
        expression = std::make_unique<BinaryExprNode>(op.line, op.column, op.type, op.text,
                                                     std::move(expression), std::move(right));
    }
    return expression;
}

std::unique_ptr<ExprNode> Parser::parseAdditive() {
    auto expression = parseMultiplicative();
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        const Token op = previous();
        auto right = parseMultiplicative();
        expression = std::make_unique<BinaryExprNode>(op.line, op.column, op.type, op.text,
                                                     std::move(expression), std::move(right));
    }
    return expression;
}

std::unique_ptr<ExprNode> Parser::parseMultiplicative() {
    auto expression = parseUnary();
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
        const Token op = previous();
        auto right = parseUnary();
        expression = std::make_unique<BinaryExprNode>(op.line, op.column, op.type, op.text,
                                                     std::move(expression), std::move(right));
    }
    return expression;
}

std::unique_ptr<ExprNode> Parser::parseUnary() {
    if (match({TokenType::NOT, TokenType::MINUS, TokenType::PLUS})) {
        const Token op = previous();
        auto operand = parseUnary();
        return std::make_unique<UnaryExprNode>(op.line, op.column, op.type, op.text,
                                              std::move(operand));
    }
    return parsePrimary();
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
    if (match(TokenType::INT_LITERAL)) {
        const Token literal = previous();
        return std::make_unique<LiteralExprNode>(literal.line, literal.column, Type::getInt(), literal.text);
    }
    if (match(TokenType::FLOAT_LITERAL)) {
        const Token literal = previous();
        return std::make_unique<LiteralExprNode>(literal.line, literal.column, Type::getFloat(), literal.text);
    }
    if (match(TokenType::CHAR_LITERAL)) {
        const Token literal = previous();
        return std::make_unique<LiteralExprNode>(literal.line, literal.column, Type::getChar(), literal.text);
    }
    if (match({TokenType::KEYWORD_TRUE, TokenType::KEYWORD_FALSE, TokenType::BOOL_LITERAL})) {
        const Token literal = previous();
        return std::make_unique<LiteralExprNode>(literal.line, literal.column, Type::getBool(), literal.text);
    }

    if (match(TokenType::IDENTIFIER)) {
        const Token name = previous();
        if (match(TokenType::LPAREN)) {
            std::vector<std::unique_ptr<ExprNode>> arguments;
            if (!check(TokenType::RPAREN)) {
                do {
                    arguments.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RPAREN, "Expected ')' after function arguments",
                    "Close the argument list with ')'.");
            return std::make_unique<CallExprNode>(name.line, name.column, name.text, std::move(arguments));
        }
        if (match(TokenType::LBRACKET)) {
            auto index = parseExpression();
            consume(TokenType::RBRACKET, "Expected ']' after array index",
                    "Close the array subscript with ']'.");
            if (check(TokenType::LBRACKET)) {
                fail(peek(), "Multidimensional array subscripts are not supported",
                     "Access a single index of a one-dimensional array.");
            }
            return std::make_unique<ArrayAccessExprNode>(name.line, name.column, name.text, std::move(index));
        }
        return std::make_unique<VariableExprNode>(name.line, name.column, name.text);
    }

    if (match(TokenType::LPAREN)) {
        auto expression = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression",
                "Close the grouped expression with ')'.");
        return expression;
    }

    fail(peek(), "Expected expression",
         "Use a literal, variable, function call, or array element, and provide operands for each operator.");
}
