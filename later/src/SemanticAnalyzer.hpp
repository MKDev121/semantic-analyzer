#pragma once

#include "AST.hpp"
#include "SymbolTable.hpp"
#include "ErrorReporter.hpp"
#include <map>
#include <set>
#include <string>
#include <utility>

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
    struct VarInitSnapshot {
        InitState state = InitState::UNINITIALIZED;
        std::string reason;
        int branchLine = 0;
        bool reportedUninitRead = false;
    };

    using SymbolKey = std::pair<int, std::string>;
    using InitEnv = std::map<SymbolKey, VarInitSnapshot>;

    SymbolTable& symTab;
    ErrorReporter& reporter;

    // Current function context for return statements
    bool inFunction = false;
    std::string currentFunctionName;
    TypePtr currentFunctionReturnType = nullptr;
    int currentFunctionDeclLine = 0;

    // Track poisoned (undeclared) identifiers per scope ID to suppress cascading errors
    std::map<int, std::set<std::string>> poisonedIdentifiers;

    void registerTopLevelDeclarations(ProgramNode* program);
    void registerTopLevelDeclaration(ASTNode* declaration);

    InitEnv captureInitEnv() const;
    void restoreInitEnv(const InitEnv& env);

    bool isPoisoned(const std::string& name) const;
    void poisonIdentifier(const std::string& name);

    std::string formatShadowTrace(const Symbol* sym) const;
    std::string collectExprContextNotes(const ExprNode* expr, bool includeDeclLine = true) const;
    static std::string exprToString(const ExprNode* expr);
};
