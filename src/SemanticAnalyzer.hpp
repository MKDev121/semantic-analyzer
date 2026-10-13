#pragma once

#include "AST.hpp"
#include "SymbolTable.hpp"
#include "ErrorReporter.hpp"
#include <string>

class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer(SymbolTable& symbolTable, ErrorReporter& reporter);

    void analyze(ProgramNode* program);

    // Visitor methods
    void visit(ProgramNode* node) override;
    void visit(VarDeclNode* node) override;
    void visit(ParamNode* node) override;
    void visit(FunctionDeclNode* node) override;
    void visit(BlockNode* node) override;
    void visit(AssignStmtNode* node) override;
    void visit(IfStmtNode* node) override;
    void visit(WhileStmtNode* node) override;
    void visit(ReturnStmtNode* node) override;
    void visit(ExprStmtNode* node) override;
    void visit(LiteralExprNode* node) override;
    void visit(VariableExprNode* node) override;
    void visit(ArrayAccessExprNode* node) override;
    void visit(BinaryExprNode* node) override;
    void visit(UnaryExprNode* node) override;
    void visit(CallExprNode* node) override;

private:
    SymbolTable& symTab;
    ErrorReporter& reporter;

    // Current function context for return statements
    bool inFunction = false;
    std::string currentFunctionName;
    TypePtr currentFunctionReturnType = nullptr;

    // Two-pass support: Pass 1 collects top-level declarations
    bool isDeclarationPass = false;

    void registerTopLevelDeclarations(ProgramNode* program);
};
