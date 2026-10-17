#pragma once

#include "Token.hpp"
#include "AST.hpp"
#include "ErrorReporter.hpp"
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

// Recursive-descent parser for the deliberately small teaching language.
// A failed production is discarded; recovery resumes at a statement boundary.
class Parser {
public:
    Parser(std::vector<Token> tokens, ErrorReporter& reporter);
    std::unique_ptr<ProgramNode> parseProgram();

private:
    struct ParseError {};

    std::vector<Token> tokens;
    std::size_t current = 0;
    ErrorReporter& reporter;

    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    [[noreturn]] void fail(const Token& token, const std::string& message);
    void synchronize(std::size_t failedStart, bool inBlock);

    std::unique_ptr<ASTNode> parseTopLevelDecl();
    TypePtr parseType();
    std::unique_ptr<FunctionDeclNode> parseFunctionDecl(TypePtr retType, const Token& nameTok);
    std::unique_ptr<VarDeclNode> parseVarDecl(TypePtr type, const Token& nameTok);
    std::vector<ParamNode> parseParamList();

    std::unique_ptr<StmtNode> parseStatement();
    std::unique_ptr<BlockNode> parseBlock();
    std::unique_ptr<IfStmtNode> parseIfStatement();
    std::unique_ptr<WhileStmtNode> parseWhileStatement();
    std::unique_ptr<ReturnStmtNode> parseReturnStatement();
    std::unique_ptr<StmtNode> parseAssignmentOrExpr();

    std::unique_ptr<ExprNode> parseExpression();
    std::unique_ptr<ExprNode> parseLogicalOr();
    std::unique_ptr<ExprNode> parseLogicalAnd();
    std::unique_ptr<ExprNode> parseEquality();
    std::unique_ptr<ExprNode> parseRelational();
    std::unique_ptr<ExprNode> parseAdditive();
    std::unique_ptr<ExprNode> parseMultiplicative();
    std::unique_ptr<ExprNode> parseUnary();
    std::unique_ptr<ExprNode> parsePrimary();

    bool isTypeStart(TokenType type) const;
};
