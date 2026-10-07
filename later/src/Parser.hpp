#pragma once

#include "Token.hpp"
#include "AST.hpp"
#include "ErrorReporter.hpp"
#include <vector>
#include <memory>

class Parser {
public:
    Parser(std::vector<Token> tokens, ErrorReporter& reporter);

    std::unique_ptr<ProgramNode> parseProgram();

private:
    std::vector<Token> tokens;
    size_t current = 0;
    ErrorReporter& reporter;

    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);

    void synchronize();

    // Grammar rules
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
    std::unique_ptr<StmtNode> parseVarDeclOrAssignmentOrExpr();

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
