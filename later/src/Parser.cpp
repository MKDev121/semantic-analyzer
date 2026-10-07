#include "Parser.hpp"
#include <iostream>

Parser::Parser(std::vector<Token> tokens, ErrorReporter& reporter)
    : tokens(std::move(tokens)), reporter(reporter) {}

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::previous() const {
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::TOK_EOF;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    reporter.reportError(peek().line, peek().column, message + " (got '" + peek().text + "')");
    return peek();
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        switch (peek().type) {
            case TokenType::KEYWORD_INT:
            case TokenType::KEYWORD_FLOAT:
            case TokenType::KEYWORD_BOOL:
            case TokenType::KEYWORD_CHAR:
            case TokenType::KEYWORD_VOID:
            case TokenType::KEYWORD_IF:
            case TokenType::KEYWORD_WHILE:
            case TokenType::KEYWORD_RETURN:
            case TokenType::LBRACE:
            case TokenType::RBRACE:
                return;
            default:
                advance();
        }
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

    reporter.reportError(peek().line, peek().column, "Expected type specifier (int, float, bool, char, void)");
    return Type::getError();
}

std::unique_ptr<ProgramNode> Parser::parseProgram() {
    auto prog = std::make_unique<ProgramNode>(1, 1);
    while (!isAtEnd()) {
        try {
            auto decl = parseTopLevelDecl();
            if (decl) {
                prog->declarations.push_back(std::move(decl));
            }
        } catch (...) {
            synchronize();
        }
    }
    return prog;
}

std::unique_ptr<ASTNode> Parser::parseTopLevelDecl() {
    if (!isTypeStart(peek().type)) {
        reporter.reportError(peek().line, peek().column, "Expected top-level declaration (variable or function)");
        advance();
        return nullptr;
    }

    TypePtr type = parseType();
    Token nameTok = consume(TokenType::IDENTIFIER, "Expected identifier after type");

    if (check(TokenType::LPAREN)) {
        return parseFunctionDecl(type, nameTok);
    } else {
        return parseVarDecl(type, nameTok);
    }
}

std::unique_ptr<FunctionDeclNode> Parser::parseFunctionDecl(TypePtr retType, const Token& nameTok) {
    consume(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<ParamNode> params = parseParamList();
    consume(TokenType::RPAREN, "Expected ')' after parameter list");

    if (!check(TokenType::LBRACE)) {
        reporter.reportError(peek().line, peek().column, "Expected '{' to begin function body");
        return nullptr;
    }

    auto body = parseBlock();
    return std::make_unique<FunctionDeclNode>(nameTok.line, nameTok.column, retType, nameTok.text, std::move(params), std::move(body));
}

std::vector<ParamNode> Parser::parseParamList() {
    std::vector<ParamNode> params;
    if (check(TokenType::RPAREN)) return params;

    do {
        TypePtr pType = parseType();
        Token pName = consume(TokenType::IDENTIFIER, "Expected parameter name");
        params.emplace_back(pName.line, pName.column, pType, pName.text);
    } while (match(TokenType::COMMA));

    return params;
}

std::unique_ptr<VarDeclNode> Parser::parseVarDecl(TypePtr type, const Token& nameTok) {
    bool isArray = false;
    int arraySize = 0;

    if (match(TokenType::LBRACKET)) {
        isArray = true;
        Token sizeTok = consume(TokenType::INT_LITERAL, "Expected integer literal for array size");
        try {
            arraySize = std::stoi(sizeTok.text);
            if (arraySize <= 0) {
                reporter.reportError(sizeTok.line, sizeTok.column, "Array size must be greater than 0");
            }
        } catch (...) {
            reporter.reportError(sizeTok.line, sizeTok.column, "Invalid array size literal");
        }
        consume(TokenType::RBRACKET, "Expected ']' after array size");
    }

    std::unique_ptr<ExprNode> initExpr = nullptr;
    if (match(TokenType::ASSIGN)) {
        if (isArray) {
            reporter.reportError(nameTok.line, nameTok.column, "Direct array initialization with '=' is not supported");
        }
        initExpr = parseExpression();
    }

    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");

    TypePtr finalType = isArray ? Type::makeArray(type, arraySize) : type;
    return std::make_unique<VarDeclNode>(nameTok.line, nameTok.column, finalType, nameTok.text, std::move(initExpr), isArray, arraySize);
}

std::unique_ptr<BlockNode> Parser::parseBlock() {
    Token braceTok = consume(TokenType::LBRACE, "Expected '{'");
    auto block = std::make_unique<BlockNode>(braceTok.line, braceTok.column);

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
    }

    consume(TokenType::RBRACE, "Expected '}' after block");
    return block;
}

std::unique_ptr<StmtNode> Parser::parseStatement() {
    if (check(TokenType::LBRACE)) {
        return parseBlock();
    }
    if (check(TokenType::KEYWORD_IF)) {
        return parseIfStatement();
    }
    if (check(TokenType::KEYWORD_WHILE)) {
        return parseWhileStatement();
    }
    if (check(TokenType::KEYWORD_RETURN)) {
        return parseReturnStatement();
    }
    if (isTypeStart(peek().type)) {
        TypePtr type = parseType();
        Token nameTok = consume(TokenType::IDENTIFIER, "Expected identifier after type");
        return parseVarDecl(type, nameTok);
    }

    return parseVarDeclOrAssignmentOrExpr();
}

std::unique_ptr<IfStmtNode> Parser::parseIfStatement() {
    Token ifTok = consume(TokenType::KEYWORD_IF, "Expected 'if'");
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    auto cond = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after condition");

    auto thenBranch = parseStatement();
    std::unique_ptr<StmtNode> elseBranch = nullptr;

    if (match(TokenType::KEYWORD_ELSE)) {
        elseBranch = parseStatement();
    }

    return std::make_unique<IfStmtNode>(ifTok.line, ifTok.column, std::move(cond), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<WhileStmtNode> Parser::parseWhileStatement() {
    Token whileTok = consume(TokenType::KEYWORD_WHILE, "Expected 'while'");
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    auto cond = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after condition");

    auto body = parseStatement();
    return std::make_unique<WhileStmtNode>(whileTok.line, whileTok.column, std::move(cond), std::move(body));
}

std::unique_ptr<ReturnStmtNode> Parser::parseReturnStatement() {
    Token retTok = consume(TokenType::KEYWORD_RETURN, "Expected 'return'");
    std::unique_ptr<ExprNode> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after return statement");
    return std::make_unique<ReturnStmtNode>(retTok.line, retTok.column, std::move(value));
}

std::unique_ptr<StmtNode> Parser::parseVarDeclOrAssignmentOrExpr() {
    // Check if it's an assignment: IDENT = expr; OR IDENT[idx] = expr;
    if (peek().type == TokenType::IDENTIFIER) {
        // Lookahead to check if assignment
        if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::ASSIGN) {
            Token nameTok = advance(); // IDENT
            advance(); // =
            auto rhs = parseExpression();
            consume(TokenType::SEMICOLON, "Expected ';' after assignment");
            return std::make_unique<AssignStmtNode>(nameTok.line, nameTok.column, nameTok.text, nullptr, std::move(rhs));
        }

        if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::LBRACKET) {
            // Check if this array subscript is on the LHS of '='
            // Scan ahead until matching ']'
            size_t scan = current + 2;
            int bracketDepth = 1;
            while (scan < tokens.size() && bracketDepth > 0) {
                if (tokens[scan].type == TokenType::LBRACKET) bracketDepth++;
                else if (tokens[scan].type == TokenType::RBRACKET) bracketDepth--;
                scan++;
            }
            if (scan < tokens.size() && tokens[scan].type == TokenType::ASSIGN) {
                Token nameTok = advance(); // IDENT
                advance(); // '['
                auto idxExpr = parseExpression();
                consume(TokenType::RBRACKET, "Expected ']' after array index");
                consume(TokenType::ASSIGN, "Expected '='");
                auto rhs = parseExpression();
                consume(TokenType::SEMICOLON, "Expected ';' after array assignment");
                return std::make_unique<AssignStmtNode>(nameTok.line, nameTok.column, nameTok.text, std::move(idxExpr), std::move(rhs));
            }
        }
    }

    // Otherwise, parse as expression statement
    int line = peek().line;
    int col = peek().column;
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression statement");
    return std::make_unique<ExprStmtNode>(line, col, std::move(expr));
}

std::unique_ptr<ExprNode> Parser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<ExprNode> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    while (check(TokenType::OR)) {
        Token opTok = advance();
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExprNode>(opTok.line, opTok.column, opTok.type, opTok.text, std::move(expr), std::move(right));
    }
    return expr;
}

std::unique_ptr<ExprNode> Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    while (check(TokenType::AND)) {
        Token opTok = advance();
        auto right = parseEquality();
        expr = std::make_unique<BinaryExprNode>(opTok.line, opTok.column, opTok.type, opTok.text, std::move(expr), std::move(right));
    }
    return expr;
}

std::unique_ptr<ExprNode> Parser::parseEquality() {
    auto expr = parseRelational();
    while (check(TokenType::EQ) || check(TokenType::NEQ)) {
        Token opTok = advance();
        auto right = parseRelational();
        expr = std::make_unique<BinaryExprNode>(opTok.line, opTok.column, opTok.type, opTok.text, std::move(expr), std::move(right));
    }
    return expr;
}

std::unique_ptr<ExprNode> Parser::parseRelational() {
    auto expr = parseAdditive();
    while (check(TokenType::LT) || check(TokenType::LE) || check(TokenType::GT) || check(TokenType::GE)) {
        Token opTok = advance();
        auto right = parseAdditive();
        expr = std::make_unique<BinaryExprNode>(opTok.line, opTok.column, opTok.type, opTok.text, std::move(expr), std::move(right));
    }
    return expr;
}

std::unique_ptr<ExprNode> Parser::parseAdditive() {
    auto expr = parseMultiplicative();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token opTok = advance();
        auto right = parseMultiplicative();
        expr = std::make_unique<BinaryExprNode>(opTok.line, opTok.column, opTok.type, opTok.text, std::move(expr), std::move(right));
    }
    return expr;
}

std::unique_ptr<ExprNode> Parser::parseMultiplicative() {
    auto expr = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        Token opTok = advance();
        auto right = parseUnary();
        expr = std::make_unique<BinaryExprNode>(opTok.line, opTok.column, opTok.type, opTok.text, std::move(expr), std::move(right));
    }
    return expr;
}

std::unique_ptr<ExprNode> Parser::parseUnary() {
    if (check(TokenType::NOT) || check(TokenType::MINUS) || check(TokenType::PLUS)) {
        Token opTok = advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryExprNode>(opTok.line, opTok.column, opTok.type, opTok.text, std::move(operand));
    }
    return parsePrimary();
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
    if (match(TokenType::INT_LITERAL)) {
        return std::make_unique<LiteralExprNode>(previous().line, previous().column, Type::getInt(), previous().text);
    }
    if (match(TokenType::FLOAT_LITERAL)) {
        return std::make_unique<LiteralExprNode>(previous().line, previous().column, Type::getFloat(), previous().text);
    }
    if (match(TokenType::CHAR_LITERAL)) {
        return std::make_unique<LiteralExprNode>(previous().line, previous().column, Type::getChar(), previous().text);
    }
    if (match(TokenType::KEYWORD_TRUE)) {
        return std::make_unique<LiteralExprNode>(previous().line, previous().column, Type::getBool(), "true");
    }
    if (match(TokenType::KEYWORD_FALSE)) {
        return std::make_unique<LiteralExprNode>(previous().line, previous().column, Type::getBool(), "false");
    }

    if (match(TokenType::IDENTIFIER)) {
        Token idTok = previous();

        // Function call: ident ( args )
        if (match(TokenType::LPAREN)) {
            std::vector<std::unique_ptr<ExprNode>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RPAREN, "Expected ')' after function arguments");
            return std::make_unique<CallExprNode>(idTok.line, idTok.column, idTok.text, std::move(args));
        }

        // Array subscript access: ident [ expr ]
        if (match(TokenType::LBRACKET)) {
            auto idx = parseExpression();
            consume(TokenType::RBRACKET, "Expected ']' after array index");
            return std::make_unique<ArrayAccessExprNode>(idTok.line, idTok.column, idTok.text, std::move(idx));
        }

        // Simple variable access
        return std::make_unique<VariableExprNode>(idTok.line, idTok.column, idTok.text);
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    reporter.reportError(peek().line, peek().column, "Unexpected token in expression: '" + peek().text + "'");
    advance();
    return std::make_unique<LiteralExprNode>(peek().line, peek().column, Type::getError(), "<error>");
}
