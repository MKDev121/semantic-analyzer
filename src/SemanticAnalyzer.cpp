#include "SemanticAnalyzer.hpp"

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symbolTable, ErrorReporter& reporter)
    : symTab(symbolTable), reporter(reporter) {}

void SemanticAnalyzer::analyze(ProgramNode* program) {
    if (!program) return;

    symTab.reset();

    // Register and check declarations in source order. Each function can call
    // itself or earlier definitions. Forward registration belongs to later/.
    for (const auto& declaration : program->declarations) {
        registerTopLevelDeclaration(declaration.get());
        declaration->accept(this);
    }
}

void SemanticAnalyzer::registerTopLevelDeclaration(ASTNode* declaration) {
    if (auto funcDecl = dynamic_cast<FunctionDeclNode*>(declaration)) {
        std::vector<TypePtr> paramTypes;
        for (const auto& p : funcDecl->params) {
            if (p.type->isVoid()) {
                reporter.reportError(p.line, p.column, "Parameter '" + p.name + "' cannot have type 'void'");
                paramTypes.push_back(Type::getError());
            } else {
                paramTypes.push_back(p.type);
            }
        }
        TypePtr funcType = Type::makeFunction(funcDecl->returnType, paramTypes);
        Symbol sym(funcDecl->name, funcType, SymbolKind::FUNCTION, 0, funcDecl->line, funcDecl->column, true);
        if (!symTab.declare(sym)) {
            Symbol* prev = symTab.lookupCurrentScope(funcDecl->name);
            int prevLine = prev ? prev->line : 0;
            reporter.reportError(funcDecl->line, funcDecl->column,
                "Redeclaration of function '" + funcDecl->name + "' (previously declared at line " + std::to_string(prevLine) + ")");
        }
    } else if (auto varDecl = dynamic_cast<VarDeclNode*>(declaration)) {
        // Global types are validated when their declarations are visited.
        TypePtr symbolType = varDecl->declaredType->isVoid()
            ? Type::getError() : varDecl->declaredType;
        Symbol sym(varDecl->varName, symbolType, SymbolKind::VARIABLE, 0, varDecl->line, varDecl->column, varDecl->initExpr != nullptr);
        if (!symTab.declare(sym)) {
            Symbol* prev = symTab.lookupCurrentScope(varDecl->varName);
            int prevLine = prev ? prev->line : 0;
            reporter.reportError(varDecl->line, varDecl->column,
                "Redeclaration of global variable '" + varDecl->varName + "' (previously declared at line " + std::to_string(prevLine) + ")");
        }
    }
}

void SemanticAnalyzer::visit(ProgramNode* node) {
    for (const auto& decl : node->declarations) {
        decl->accept(this);
    }
}

void SemanticAnalyzer::visit(VarDeclNode* node) {
    const bool invalidType = node->declaredType->isVoid();
    if (invalidType) {
        reporter.reportError(node->line, node->column,
            "Variable '" + node->varName + "' cannot be declared with type 'void'");
    }

    // If inside a function or local block, declare in current scope
    if (symTab.getCurrentLevel() > 0) {
        Symbol sym(node->varName, invalidType ? Type::getError() : node->declaredType, SymbolKind::VARIABLE,
                   symTab.getCurrentLevel(), node->line, node->column, node->initExpr != nullptr);
        if (!symTab.declare(sym)) {
            Symbol* prev = symTab.lookupCurrentScope(node->varName);
            int prevLine = prev ? prev->line : 0;
            reporter.reportError(node->line, node->column,
                "Redeclaration of identifier '" + node->varName + "' in the same scope (previously declared at line " + std::to_string(prevLine) + ")");
        }
    }

    // Type check initialization expression if present
    if (node->initExpr) {
        node->initExpr->accept(this);
        if (!invalidType && !node->declaredType->isError() &&
            !node->initExpr->inferredType->isError() &&
            !node->initExpr->inferredType->isAssignableTo(*node->declaredType)) {
            reporter.reportError(node->line, node->column,
                "Type mismatch in variable initialization: cannot assign '" +
                node->initExpr->inferredType->toString() + "' to variable '" +
                node->varName + "' of type '" + node->declaredType->toString() + "'");
        }
    }
}

void SemanticAnalyzer::visit(ParamNode* node) {
    (void)node;
    // Parameter handling is coordinated in FunctionDeclNode
}

void SemanticAnalyzer::visit(FunctionDeclNode* node) {
    symTab.enterScope("Function " + node->name);
    inFunction = true;
    currentFunctionName = node->name;
    currentFunctionReturnType = node->returnType;

    // Register parameters in the function scope
    for (const auto& p : node->params) {
        // Invalid parameter types were diagnosed when signatures were collected.
        TypePtr parameterType = p.type->isVoid() ? Type::getError() : p.type;
        Symbol sym(p.name, parameterType, SymbolKind::PARAMETER, symTab.getCurrentLevel(), p.line, p.column, true);
        if (!symTab.declare(sym)) {
            reporter.reportError(p.line, p.column,
                "Duplicate parameter name '" + p.name + "' in function definition '" + node->name + "'");
        }
    }

    // Analyze function body
    if (node->body) {
        for (const auto& stmt : node->body->statements) {
            stmt->accept(this);
        }
    }

    inFunction = false;
    currentFunctionName = "";
    currentFunctionReturnType = nullptr;
    symTab.exitScope();
}

void SemanticAnalyzer::visit(BlockNode* node) {
    symTab.enterScope("Block@L" + std::to_string(node->line));
    for (const auto& stmt : node->statements) {
        stmt->accept(this);
    }
    symTab.exitScope();
}

void SemanticAnalyzer::visit(AssignStmtNode* node) {
    if (node->rhs) {
        node->rhs->accept(this);
    }

    Symbol* sym = symTab.lookup(node->varName);
    if (!sym) {
        reporter.reportError(node->line, node->column,
            "Undeclared identifier '" + node->varName + "' in assignment statement");
        return;
    }

    if (sym->type->isError()) return;

    if (sym->kind == SymbolKind::FUNCTION) {
        reporter.reportError(node->line, node->column,
            "Cannot assign to function '" + node->varName + "' (not a modifiable l-value)");
        return;
    }

    if (node->rhs && !node->rhs->inferredType->isError() && !node->rhs->inferredType->isAssignableTo(*sym->type)) {
        reporter.reportError(node->line, node->column,
            "Type mismatch in assignment: cannot assign '" + node->rhs->inferredType->toString() +
            "' to variable '" + node->varName + "' of type '" + sym->type->toString() + "'");
    }
}

void SemanticAnalyzer::visit(IfStmtNode* node) {
    if (node->condition) {
        node->condition->accept(this);
        if (!node->condition->inferredType->isError() && !node->condition->inferredType->isBoolean()) {
            reporter.reportError(node->condition->line, node->condition->column,
                "Condition in 'if' statement must evaluate to 'bool', got '" + node->condition->inferredType->toString() + "'");
        }
    }
    if (node->thenBranch) node->thenBranch->accept(this);
    if (node->elseBranch) node->elseBranch->accept(this);
}

void SemanticAnalyzer::visit(WhileStmtNode* node) {
    if (node->condition) {
        node->condition->accept(this);
        if (!node->condition->inferredType->isError() && !node->condition->inferredType->isBoolean()) {
            reporter.reportError(node->condition->line, node->condition->column,
                "Condition in 'while' statement must evaluate to 'bool', got '" + node->condition->inferredType->toString() + "'");
        }
    }
    if (node->body) node->body->accept(this);
}

void SemanticAnalyzer::visit(ReturnStmtNode* node) {
    if (!inFunction) {
        reporter.reportError(node->line, node->column,
            "'return' statement outside of function definition");
        return;
    }

    if (node->value) {
        node->value->accept(this);
        if (!node->value->inferredType->isError()) {
            if (currentFunctionReturnType->isVoid()) {
                reporter.reportError(node->line, node->column,
                    "Void function '" + currentFunctionName + "' cannot return a value");
            } else if (!node->value->inferredType->isAssignableTo(*currentFunctionReturnType)) {
                reporter.reportError(node->line, node->column,
                    "Return type mismatch: function '" + currentFunctionName + "' expects return type '" +
                    currentFunctionReturnType->toString() + "', but got '" + node->value->inferredType->toString() + "'");
            }
        }
    } else {
        if (!currentFunctionReturnType->isVoid()) {
            reporter.reportError(node->line, node->column,
                "Non-void function '" + currentFunctionName + "' must return a value of type '" +
                currentFunctionReturnType->toString() + "'");
        }
    }
}

void SemanticAnalyzer::visit(ExprStmtNode* node) {
    if (node->expr) {
        node->expr->accept(this);
    }
}

void SemanticAnalyzer::visit(LiteralExprNode* node) {
    (void)node;
    // Literal type already set in constructor
}

void SemanticAnalyzer::visit(VariableExprNode* node) {
    Symbol* sym = symTab.lookup(node->name);
    if (!sym) {
        reporter.reportError(node->line, node->column,
            "Undeclared identifier '" + node->name + "'");
        node->inferredType = Type::getError();
        return;
    }

    if (sym->kind == SymbolKind::FUNCTION) {
        reporter.reportError(node->line, node->column,
            "Identifier '" + node->name + "' refers to a function, not a variable");
        node->inferredType = Type::getError();
        return;
    }

    node->inferredType = sym->type;
}

void SemanticAnalyzer::visit(BinaryExprNode* node) {
    node->lhs->accept(this);
    node->rhs->accept(this);

    TypePtr leftT = node->lhs->inferredType;
    TypePtr rightT = node->rhs->inferredType;

    if (leftT->isError() || rightT->isError()) {
        node->inferredType = Type::getError();
        return;
    }

    switch (node->op) {
        // Arithmetic operators: +, -, *, /
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::STAR:
        case TokenType::SLASH: {
            if (!leftT->isNumeric() || !rightT->isNumeric()) {
                reporter.reportError(node->line, node->column,
                    "Arithmetic operator '" + node->opStr + "' requires numeric operands, got '" +
                    leftT->toString() + "' and '" + rightT->toString() + "'");
                node->inferredType = Type::getError();
            } else if (leftT->isFloat() || rightT->isFloat()) {
                node->inferredType = Type::getFloat();
            } else {
                node->inferredType = Type::getInt();
            }
            break;
        }

        // Modulo operator: %
        case TokenType::PERCENT: {
            if (!leftT->isInteger() || !rightT->isInteger()) {
                reporter.reportError(node->line, node->column,
                    "Modulo operator '%' requires integer operands, got '" +
                    leftT->toString() + "' and '" + rightT->toString() + "'");
                node->inferredType = Type::getError();
            } else {
                node->inferredType = Type::getInt();
            }
            break;
        }

        // Relational operators: <, <=, >, >=
        case TokenType::LT:
        case TokenType::LE:
        case TokenType::GT:
        case TokenType::GE: {
            if (!leftT->isNumeric() || !rightT->isNumeric()) {
                reporter.reportError(node->line, node->column,
                    "Relational operator '" + node->opStr + "' requires numeric operands, got '" +
                    leftT->toString() + "' and '" + rightT->toString() + "'");
                node->inferredType = Type::getError();
            } else {
                node->inferredType = Type::getBool();
            }
            break;
        }

        // Equality operators: ==, !=
        case TokenType::EQ:
        case TokenType::NEQ: {
            bool comparable = false;
            if (leftT->isNumeric() && rightT->isNumeric()) comparable = true;
            else if (leftT->isBoolean() && rightT->isBoolean()) comparable = true;
            else if (leftT->isChar() && rightT->isChar()) comparable = true;

            if (!comparable) {
                reporter.reportError(node->line, node->column,
                    "Cannot compare incompatible types '" + leftT->toString() +
                    "' and '" + rightT->toString() + "' with equality operator '" + node->opStr + "'");
                node->inferredType = Type::getError();
            } else {
                node->inferredType = Type::getBool();
            }
            break;
        }

        // Logical operators: &&, ||
        case TokenType::AND:
        case TokenType::OR: {
            if (!leftT->isBoolean() || !rightT->isBoolean()) {
                reporter.reportError(node->line, node->column,
                    "Logical operator '" + node->opStr + "' requires boolean operands, got '" +
                    leftT->toString() + "' and '" + rightT->toString() + "'");
                node->inferredType = Type::getError();
            } else {
                node->inferredType = Type::getBool();
            }
            break;
        }

        default:
            node->inferredType = Type::getError();
            break;
    }
}

void SemanticAnalyzer::visit(UnaryExprNode* node) {
    node->expr->accept(this);
    TypePtr innerT = node->expr->inferredType;

    if (innerT->isError()) {
        node->inferredType = Type::getError();
        return;
    }

    if (node->op == TokenType::NOT) {
        if (!innerT->isBoolean()) {
            reporter.reportError(node->line, node->column,
                "Logical NOT operator '!' requires boolean operand, got '" + innerT->toString() + "'");
            node->inferredType = Type::getError();
        } else {
            node->inferredType = Type::getBool();
        }
    } else if (node->op == TokenType::MINUS || node->op == TokenType::PLUS) {
        if (!innerT->isNumeric()) {
            reporter.reportError(node->line, node->column,
                "Unary operator '" + node->opStr + "' requires numeric operand, got '" + innerT->toString() + "'");
            node->inferredType = Type::getError();
        } else {
            node->inferredType = innerT;
        }
    } else {
        node->inferredType = Type::getError();
    }
}

void SemanticAnalyzer::visit(CallExprNode* node) {
    for (const auto& arg : node->args) {
        arg->accept(this);
    }

    Symbol* sym = symTab.lookup(node->callee);
    if (!sym) {
        reporter.reportError(node->line, node->column,
            "Call to undeclared function '" + node->callee + "'");
        node->inferredType = Type::getError();
        return;
    }

    if (sym->type->isError()) {
        node->inferredType = Type::getError();
        return;
    }

    if (sym->kind != SymbolKind::FUNCTION) {
        reporter.reportError(node->line, node->column,
            "Called object '" + node->callee + "' is not a function (type is '" + sym->type->toString() + "')");
        node->inferredType = Type::getError();
        return;
    }

    const auto& expectedParams = sym->type->paramTypes;
    if (node->args.size() != expectedParams.size()) {
        reporter.reportError(node->line, node->column,
            "Function '" + node->callee + "' expects " + std::to_string(expectedParams.size()) +
            " arguments, but " + std::to_string(node->args.size()) + " were provided");
        node->inferredType = Type::getError();
        return;
    }

    bool invalidArgument = false;
    for (size_t i = 0; i < node->args.size(); ++i) {
        TypePtr argT = node->args[i]->inferredType;
        TypePtr paramT = expectedParams[i];
        if (argT->isError() || paramT->isError()) {
            invalidArgument = true;
            continue;
        }
        if (!argT->isAssignableTo(*paramT)) {
            reporter.reportError(node->args[i]->line, node->args[i]->column,
                "Argument " + std::to_string(i + 1) + " of function '" + node->callee +
                "': type mismatch, expected '" + paramT->toString() + "', got '" + argT->toString() + "'");
            invalidArgument = true;
        }
    }

    node->inferredType = invalidArgument ? Type::getError() : sym->type->returnType;
}
