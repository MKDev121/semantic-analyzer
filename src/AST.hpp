#pragma once

#include "Type.hpp"
#include "Token.hpp"
#include <string>
#include <vector>
#include <memory>
#include <iostream>

class ASTVisitor;

class ASTNode {
public:
    int line;
    int column;

    ASTNode(int line, int col) : line(line), column(col) {}
    virtual ~ASTNode() = default;

    virtual void accept(ASTVisitor* visitor) = 0;
    virtual void print(std::ostream& out, int indent = 0) const = 0;
};

class ExprNode : public ASTNode {
public:
    TypePtr inferredType;

    ExprNode(int line, int col) : ASTNode(line, col), inferredType(nullptr) {}
};

class StmtNode : public ASTNode {
public:
    StmtNode(int line, int col) : ASTNode(line, col) {}
};

// ================= Expressions =================

class LiteralExprNode : public ExprNode {
public:
    std::string rawValue;

    LiteralExprNode(int line, int col, TypePtr type, std::string raw)
        : ExprNode(line, col), rawValue(std::move(raw)) {
        inferredType = type;
    }

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class VariableExprNode : public ExprNode {
public:
    std::string name;

    VariableExprNode(int line, int col, std::string name)
        : ExprNode(line, col), name(std::move(name)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class ArrayAccessExprNode : public ExprNode {
public:
    std::string name;
    std::unique_ptr<ExprNode> indexExpr;

    ArrayAccessExprNode(int line, int col, std::string name, std::unique_ptr<ExprNode> indexExpr)
        : ExprNode(line, col), name(std::move(name)), indexExpr(std::move(indexExpr)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class BinaryExprNode : public ExprNode {
public:
    TokenType op;
    std::string opStr;
    std::unique_ptr<ExprNode> lhs;
    std::unique_ptr<ExprNode> rhs;

    BinaryExprNode(int line, int col, TokenType op, std::string opStr,
                   std::unique_ptr<ExprNode> lhs, std::unique_ptr<ExprNode> rhs)
        : ExprNode(line, col), op(op), opStr(std::move(opStr)),
          lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class UnaryExprNode : public ExprNode {
public:
    TokenType op;
    std::string opStr;
    std::unique_ptr<ExprNode> expr;

    UnaryExprNode(int line, int col, TokenType op, std::string opStr, std::unique_ptr<ExprNode> expr)
        : ExprNode(line, col), op(op), opStr(std::move(opStr)), expr(std::move(expr)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class CallExprNode : public ExprNode {
public:
    std::string callee;
    std::vector<std::unique_ptr<ExprNode>> args;

    CallExprNode(int line, int col, std::string callee, std::vector<std::unique_ptr<ExprNode>> args)
        : ExprNode(line, col), callee(std::move(callee)), args(std::move(args)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

// ================= Statements =================

class VarDeclNode : public StmtNode {
public:
    TypePtr declaredType;
    std::string varName;
    std::unique_ptr<ExprNode> initExpr;
    bool isArray;
    int arraySize;

    VarDeclNode(int line, int col, TypePtr declaredType, std::string varName,
                std::unique_ptr<ExprNode> initExpr = nullptr, bool isArray = false, int arraySize = 0)
        : StmtNode(line, col), declaredType(std::move(declaredType)),
          varName(std::move(varName)), initExpr(std::move(initExpr)),
          isArray(isArray), arraySize(arraySize) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class AssignStmtNode : public StmtNode {
public:
    std::string varName;
    std::unique_ptr<ExprNode> indexExpr; // nullptr if simple variable
    std::unique_ptr<ExprNode> rhs;

    AssignStmtNode(int line, int col, std::string varName,
                   std::unique_ptr<ExprNode> indexExpr, std::unique_ptr<ExprNode> rhs)
        : StmtNode(line, col), varName(std::move(varName)),
          indexExpr(std::move(indexExpr)), rhs(std::move(rhs)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class BlockNode : public StmtNode {
public:
    std::vector<std::unique_ptr<StmtNode>> statements;

    BlockNode(int line, int col) : StmtNode(line, col) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class IfStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> thenBranch;
    std::unique_ptr<StmtNode> elseBranch;

    IfStmtNode(int line, int col, std::unique_ptr<ExprNode> cond,
               std::unique_ptr<StmtNode> thenB, std::unique_ptr<StmtNode> elseB = nullptr)
        : StmtNode(line, col), condition(std::move(cond)),
          thenBranch(std::move(thenB)), elseBranch(std::move(elseB)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class WhileStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> condition;
    std::unique_ptr<StmtNode> body;

    WhileStmtNode(int line, int col, std::unique_ptr<ExprNode> cond, std::unique_ptr<StmtNode> body)
        : StmtNode(line, col), condition(std::move(cond)), body(std::move(body)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class ReturnStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> value;

    ReturnStmtNode(int line, int col, std::unique_ptr<ExprNode> val = nullptr)
        : StmtNode(line, col), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class ExprStmtNode : public StmtNode {
public:
    std::unique_ptr<ExprNode> expr;

    ExprStmtNode(int line, int col, std::unique_ptr<ExprNode> expr)
        : StmtNode(line, col), expr(std::move(expr)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

// ================= Top-level Declarations =================

class ParamNode : public ASTNode {
public:
    TypePtr type;
    std::string name;

    ParamNode(int line, int col, TypePtr type, std::string name)
        : ASTNode(line, col), type(std::move(type)), name(std::move(name)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class FunctionDeclNode : public ASTNode {
public:
    TypePtr returnType;
    std::string name;
    std::vector<ParamNode> params;
    std::unique_ptr<BlockNode> body;

    FunctionDeclNode(int line, int col, TypePtr retType, std::string name,
                     std::vector<ParamNode> params, std::unique_ptr<BlockNode> body)
        : ASTNode(line, col), returnType(std::move(retType)), name(std::move(name)),
          params(std::move(params)), body(std::move(body)) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> declarations; // VarDeclNode or FunctionDeclNode

    ProgramNode(int line = 1, int col = 1) : ASTNode(line, col) {}

    void accept(ASTVisitor* visitor) override;
    void print(std::ostream& out, int indent = 0) const override;
};

// ================= Visitor Interface =================

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(ProgramNode* node) = 0;
    virtual void visit(VarDeclNode* node) = 0;
    virtual void visit(ParamNode* node) = 0;
    virtual void visit(FunctionDeclNode* node) = 0;
    virtual void visit(BlockNode* node) = 0;
    virtual void visit(AssignStmtNode* node) = 0;
    virtual void visit(IfStmtNode* node) = 0;
    virtual void visit(WhileStmtNode* node) = 0;
    virtual void visit(ReturnStmtNode* node) = 0;
    virtual void visit(ExprStmtNode* node) = 0;
    virtual void visit(LiteralExprNode* node) = 0;
    virtual void visit(VariableExprNode* node) = 0;
    virtual void visit(ArrayAccessExprNode* node) = 0;
    virtual void visit(BinaryExprNode* node) = 0;
    virtual void visit(UnaryExprNode* node) = 0;
    virtual void visit(CallExprNode* node) = 0;
};
