#include "AST.hpp"

// Accept implementations
void LiteralExprNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void VariableExprNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void ArrayAccessExprNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void BinaryExprNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void UnaryExprNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void CallExprNode::accept(ASTVisitor* visitor) { visitor->visit(this); }

void VarDeclNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void AssignStmtNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void BlockNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void IfStmtNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void WhileStmtNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void ReturnStmtNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void ExprStmtNode::accept(ASTVisitor* visitor) { visitor->visit(this); }

void ParamNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void FunctionDeclNode::accept(ASTVisitor* visitor) { visitor->visit(this); }
void ProgramNode::accept(ASTVisitor* visitor) { visitor->visit(this); }

// Helper for AST indentation
static void printIndent(std::ostream& out, int indent) {
    for (int i = 0; i < indent; ++i) out << "  ";
}

void LiteralExprNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "LiteralExpr(" << rawValue;
    if (inferredType) out << " : " << inferredType->toString();
    out << ")\n";
}

void VariableExprNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "VariableExpr(" << name;
    if (inferredType) out << " : " << inferredType->toString();
    out << ")\n";
}

void ArrayAccessExprNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "ArrayAccessExpr(" << name;
    if (inferredType) out << " : " << inferredType->toString();
    out << ")\n";
    if (indexExpr) {
        indexExpr->print(out, indent + 1);
    }
}

void BinaryExprNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "BinaryExpr(" << opStr;
    if (inferredType) out << " : " << inferredType->toString();
    out << ")\n";
    if (lhs) lhs->print(out, indent + 1);
    if (rhs) rhs->print(out, indent + 1);
}

void UnaryExprNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "UnaryExpr(" << opStr;
    if (inferredType) out << " : " << inferredType->toString();
    out << ")\n";
    if (expr) expr->print(out, indent + 1);
}

void CallExprNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "CallExpr(" << callee;
    if (inferredType) out << " : " << inferredType->toString();
    out << ")\n";
    for (const auto& arg : args) {
        arg->print(out, indent + 1);
    }
}

void VarDeclNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "VarDecl(" << (declaredType ? declaredType->toString() : "unknown") << " " << varName;
    if (isArray) out << "[" << arraySize << "]";
    out << ")\n";
    if (initExpr) {
        initExpr->print(out, indent + 1);
    }
}

void AssignStmtNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "AssignStmt(" << varName;
    if (indexExpr) out << "[...]";
    out << ")\n";
    if (indexExpr) {
        printIndent(out, indent + 1);
        out << "Index:\n";
        indexExpr->print(out, indent + 2);
    }
    if (rhs) {
        printIndent(out, indent + 1);
        out << "Value:\n";
        rhs->print(out, indent + 2);
    }
}

void BlockNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "Block {\n";
    for (const auto& stmt : statements) {
        stmt->print(out, indent + 1);
    }
    printIndent(out, indent);
    out << "}\n";
}

void IfStmtNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "IfStmt\n";
    printIndent(out, indent + 1);
    out << "Condition:\n";
    if (condition) condition->print(out, indent + 2);
    printIndent(out, indent + 1);
    out << "Then:\n";
    if (thenBranch) thenBranch->print(out, indent + 2);
    if (elseBranch) {
        printIndent(out, indent + 1);
        out << "Else:\n";
        elseBranch->print(out, indent + 2);
    }
}

void WhileStmtNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "WhileStmt\n";
    printIndent(out, indent + 1);
    out << "Condition:\n";
    if (condition) condition->print(out, indent + 2);
    printIndent(out, indent + 1);
    out << "Body:\n";
    if (body) body->print(out, indent + 2);
}

void ReturnStmtNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "ReturnStmt\n";
    if (value) value->print(out, indent + 1);
}

void ExprStmtNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "ExprStmt\n";
    if (expr) expr->print(out, indent + 1);
}

void ParamNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "Param(" << (type ? type->toString() : "unknown") << " " << name << ")\n";
}

void FunctionDeclNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "FunctionDecl(" << (returnType ? returnType->toString() : "void") << " " << name << ")\n";
    if (!params.empty()) {
        printIndent(out, indent + 1);
        out << "Params:\n";
        for (const auto& p : params) {
            p.print(out, indent + 2);
        }
    }
    if (body) {
        body->print(out, indent + 1);
    }
}

void ProgramNode::print(std::ostream& out, int indent) const {
    printIndent(out, indent);
    out << "Program\n";
    for (const auto& decl : declarations) {
        decl->print(out, indent + 1);
    }
}
