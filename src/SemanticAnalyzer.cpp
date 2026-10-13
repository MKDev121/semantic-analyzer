#include "SemanticAnalyzer.hpp"

namespace {

bool hasVoidElementType(const TypePtr& type) {
    return type && (type->isVoid() ||
        (type->isArray() && hasVoidElementType(type->elementType)));
}

// A loop can execute zero times. Only explicit returns and both branches of an
// if/else establish a guaranteed return without reasoning about runtime values.
bool guaranteesReturn(const StmtNode* statement) {
    if (!statement) return false;
    if (dynamic_cast<const ReturnStmtNode*>(statement)) return true;

    if (const auto* block = dynamic_cast<const BlockNode*>(statement)) {
        for (const auto& child : block->statements) {
            if (guaranteesReturn(child.get())) return true;
        }
    } else if (const auto* conditional = dynamic_cast<const IfStmtNode*>(statement)) {
        return guaranteesReturn(conditional->thenBranch.get()) &&
               guaranteesReturn(conditional->elseBranch.get());
    }
    return false;
}

} // namespace

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symbolTable, ErrorReporter& reporter)
    : symTab(symbolTable), reporter(reporter) {}

void SemanticAnalyzer::analyze(ProgramNode* program) {
    if (!program) return;

    symTab.reset();

    // Pass 1: Register top-level function signatures and global variables
    registerTopLevelDeclarations(program);

    // Pass 2: Full semantic analysis of expressions and function bodies
    isDeclarationPass = false;
    program->accept(this);
}

void SemanticAnalyzer::registerTopLevelDeclarations(ProgramNode* program) {
    isDeclarationPass = true;
    for (const auto& decl : program->declarations) {
        if (auto funcDecl = dynamic_cast<FunctionDeclNode*>(decl.get())) {
            std::vector<TypePtr> paramTypes;
            for (const auto& p : funcDecl->params) {
                if (hasVoidElementType(p.type)) {
                    reporter.reportError(p.line, p.column, "Parameter '" + p.name + "' cannot have type 'void'",
                        "Use a value type such as int, float, bool, or char for this parameter.");
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
        } else if (auto varDecl = dynamic_cast<VarDeclNode*>(decl.get())) {
            // Variable type validation is shared with local declarations in pass 2.
            TypePtr symbolType = hasVoidElementType(varDecl->declaredType)
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
}

void SemanticAnalyzer::visit(ProgramNode* node) {
    for (const auto& decl : node->declarations) {
        decl->accept(this);
    }
}

void SemanticAnalyzer::visit(VarDeclNode* node) {
    if (isDeclarationPass) return; // Handled in Pass 1 for globals

    // Validate global and local types once, including array element types.
    const bool invalidType = hasVoidElementType(node->declaredType);
    if (invalidType) {
        const std::string message = node->declaredType->isArray()
            ? "Array '" + node->varName + "' cannot have element type 'void'"
            : "Variable '" + node->varName + "' cannot be declared with type 'void'";
        reporter.reportError(node->line, node->column,
            message, "Use int, float, bool, or char as the variable or array element type.");
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
                node->varName + "' of type '" + node->declaredType->toString() + "'",
                "Use an initializer of type '" + node->declaredType->toString() + "' or change the declared type to match the value.");
        }
    }
}

void SemanticAnalyzer::visit(ParamNode* node) {
    (void)node;
    // Parameter handling is coordinated in FunctionDeclNode
}

void SemanticAnalyzer::visit(FunctionDeclNode* node) {
    if (isDeclarationPass) return;

    symTab.enterScope("Function " + node->name);
    inFunction = true;
    currentFunctionName = node->name;
    currentFunctionReturnType = node->returnType;

    // Register parameters in the function scope
    for (const auto& p : node->params) {
        // Invalid parameter types were diagnosed when signatures were collected.
        TypePtr parameterType = hasVoidElementType(p.type) ? Type::getError() : p.type;
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

    if (!node->returnType->isVoid() && !node->returnType->isError() &&
        !guaranteesReturn(node->body.get())) {
        reporter.reportError(node->line, node->column,
            "Non-void function '" + node->name + "' may reach the end without returning a value of type '" +
            node->returnType->toString() + "'",
            "Return a compatible value on every path, including an else branch or a return after the conditional or loop.");
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
    if (node->indexExpr) {
        node->indexExpr->accept(this);
    }

    Symbol* sym = symTab.lookup(node->varName);
    if (!sym) {
        reporter.reportError(node->line, node->column,
            "Undeclared identifier '" + node->varName + "' in assignment statement",
            "Declare '" + node->varName + "' in this scope or an enclosing scope before assigning to it.");
        return;
    }

    if (sym->type->isError()) return;

    if (sym->kind == SymbolKind::FUNCTION) {
        reporter.reportError(node->line, node->column,
            "Cannot assign to function '" + node->varName + "' (not a modifiable l-value)",
            "Assign to a variable or an array element instead of a function name.");
        return;
    }

    if (node->indexExpr == nullptr) {
        // Simple variable assignment
        if (sym->type->isArray()) {
            reporter.reportError(node->line, node->column,
                "Cannot assign directly to entire array '" + node->varName + "' of type '" + sym->type->toString() + "'",
                "Assign values to individual elements using an integer subscript.");
            return;
        }

        if (node->rhs && !node->rhs->inferredType->isError() && !node->rhs->inferredType->isAssignableTo(*sym->type)) {
            reporter.reportError(node->line, node->column,
                "Type mismatch in assignment: cannot assign '" + node->rhs->inferredType->toString() +
                "' to variable '" + node->varName + "' of type '" + sym->type->toString() + "'",
                "Assign a value compatible with '" + sym->type->toString() + "'; only int-to-float widening is implicit.");
        }
    } else {
        // Array subscript assignment: arr[idx] = rhs
        if (!sym->type->isArray()) {
            reporter.reportError(node->line, node->column,
                "Subscripted variable '" + node->varName + "' is not an array (type is '" + sym->type->toString() + "')");
            return;
        }

        if (!node->indexExpr->inferredType->isError() && !node->indexExpr->inferredType->isInteger()) {
            reporter.reportError(node->indexExpr->line, node->indexExpr->column,
                "Array subscript index must be of type 'int', got '" + node->indexExpr->inferredType->toString() + "'",
                "Use an integer expression for the array index.");
        }

        if (node->rhs && !node->rhs->inferredType->isError() && !node->rhs->inferredType->isAssignableTo(*sym->type->elementType)) {
            reporter.reportError(node->line, node->column,
                "Type mismatch in array assignment: cannot assign '" + node->rhs->inferredType->toString() +
                "' to element of type '" + sym->type->elementType->toString() + "'",
                "Assign a value compatible with the array element type '" + sym->type->elementType->toString() + "'.");
        }
    }
}

void SemanticAnalyzer::visit(IfStmtNode* node) {
    if (node->condition) {
        node->condition->accept(this);
        if (!node->condition->inferredType->isError() && !node->condition->inferredType->isBoolean()) {
            reporter.reportError(node->condition->line, node->condition->column,
                "Condition in 'if' statement must evaluate to 'bool', got '" + node->condition->inferredType->toString() + "'",
                "Use a Boolean value or a comparison such as value != 0.");
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
                "Condition in 'while' statement must evaluate to 'bool', got '" + node->condition->inferredType->toString() + "'",
                "Use a Boolean value or a comparison such as value != 0.");
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
                    "Void function '" + currentFunctionName + "' cannot return a value",
                    "Use return; or change the function's declared return type to match the value.");
            } else if (!node->value->inferredType->isAssignableTo(*currentFunctionReturnType)) {
                reporter.reportError(node->line, node->column,
                    "Return type mismatch: function '" + currentFunctionName + "' expects return type '" +
                    currentFunctionReturnType->toString() + "', but got '" + node->value->inferredType->toString() + "'",
                    "Return a value compatible with '" + currentFunctionReturnType->toString() + "' or change the function's declared return type.");
            }
        }
    } else {
        if (!currentFunctionReturnType->isVoid()) {
            reporter.reportError(node->line, node->column,
                "Non-void function '" + currentFunctionName + "' must return a value of type '" +
                currentFunctionReturnType->toString() + "'",
                "Supply a compatible expression after return.");
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
            "Undeclared identifier '" + node->name + "'",
            "Declare '" + node->name + "' in this scope or an enclosing scope before using it.");
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

void SemanticAnalyzer::visit(ArrayAccessExprNode* node) {
    // Visit independent subexpressions even when the array name is invalid.
    if (node->indexExpr) node->indexExpr->accept(this);

    Symbol* sym = symTab.lookup(node->name);
    if (!sym) {
        reporter.reportError(node->line, node->column,
            "Undeclared identifier '" + node->name + "'",
            "Declare the array in this scope or an enclosing scope before using it.");
        node->inferredType = Type::getError();
        return;
    }

    if (sym->type->isError()) {
        node->inferredType = Type::getError();
        return;
    }

    if (!sym->type->isArray()) {
        reporter.reportError(node->line, node->column,
            "Subscripted value '" + node->name + "' is not an array (type is '" + sym->type->toString() + "')");
        node->inferredType = Type::getError();
        return;
    }

    if (node->indexExpr) {
        if (node->indexExpr->inferredType->isError()) {
            node->inferredType = Type::getError();
            return;
        }
        if (!node->indexExpr->inferredType->isInteger()) {
            reporter.reportError(node->indexExpr->line, node->indexExpr->column,
                "Array subscript index must be of type 'int', got '" + node->indexExpr->inferredType->toString() + "'",
                "Use an integer expression for the array index.");
            node->inferredType = Type::getError();
            return;
        }
    }

    node->inferredType = sym->type->elementType;
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
            "Call to undeclared function '" + node->callee + "'",
            "Define '" + node->callee + "' as a top-level function; its definition may appear later in the file.");
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
            " arguments, but " + std::to_string(node->args.size()) + " were provided",
            "Pass exactly " + std::to_string(expectedParams.size()) + " arguments in the order declared by the function.");
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
                "': type mismatch, expected '" + paramT->toString() + "', got '" + argT->toString() + "'",
                "Use a value compatible with parameter type '" + paramT->toString() + "' for argument " + std::to_string(i + 1) + ".");
            invalidArgument = true;
        }
    }

    node->inferredType = invalidArgument ? Type::getError() : sym->type->returnType;
}
