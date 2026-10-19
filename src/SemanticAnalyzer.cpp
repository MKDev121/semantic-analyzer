#include "SemanticAnalyzer.hpp"
#include <vector>

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

void collectExprVars(const ExprNode* expr, std::vector<std::string>& names) {
    if (!expr) return;
    if (const auto* varExpr = dynamic_cast<const VariableExprNode*>(expr)) {
        for (const auto& existing : names) {
            if (existing == varExpr->name) return;
        }
        names.push_back(varExpr->name);
    } else if (const auto* arrExpr = dynamic_cast<const ArrayAccessExprNode*>(expr)) {
        for (const auto& existing : names) {
            if (existing == arrExpr->name) return;
        }
        names.push_back(arrExpr->name);
        collectExprVars(arrExpr->indexExpr.get(), names);
    } else if (const auto* unExpr = dynamic_cast<const UnaryExprNode*>(expr)) {
        collectExprVars(unExpr->expr.get(), names);
    } else if (const auto* binExpr = dynamic_cast<const BinaryExprNode*>(expr)) {
        collectExprVars(binExpr->lhs.get(), names);
        collectExprVars(binExpr->rhs.get(), names);
    } else if (const auto* callExpr = dynamic_cast<const CallExprNode*>(expr)) {
        for (const auto& arg : callExpr->args) {
            collectExprVars(arg.get(), names);
        }
    }
}

} // namespace

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symbolTable, ErrorReporter& reporter)
    : symTab(symbolTable), reporter(reporter) {}

void SemanticAnalyzer::analyze(ProgramNode* program) {
    if (!program) return;

    symTab.reset();
    poisonedIdentifiers.clear();

    // Collect all signatures before checking bodies, allowing forward calls
    // and mutual recursion in the final implementation.
    registerTopLevelDeclarations(program);
    program->accept(this);
}

SemanticAnalyzer::InitEnv SemanticAnalyzer::captureInitEnv() const {
    InitEnv env;
    Scope* s = symTab.getCurrentScope();
    while (s != nullptr) {
        for (const auto& pair : s->symbols) {
            const Symbol& sym = pair.second;
            env[{s->id, sym.name}] = {
                sym.initState,
                sym.uninitReason,
                sym.uninitBranchLine,
                sym.reportedUninitRead,
                sym.shadowedByLine
            };
        }
        s = s->parent;
    }
    return env;
}

void SemanticAnalyzer::restoreInitEnv(const InitEnv& env) {
    Scope* s = symTab.getCurrentScope();
    while (s != nullptr) {
        for (auto& pair : s->symbols) {
            Symbol& sym = pair.second;
            auto it = env.find({s->id, sym.name});
            if (it != env.end()) {
                sym.initState = it->second.state;
                sym.isInitialized = (it->second.state == InitState::DEFINITELY_INITIALIZED);
                sym.uninitReason = it->second.reason;
                sym.uninitBranchLine = it->second.branchLine;
                sym.reportedUninitRead = it->second.reportedUninitRead;
                sym.shadowedByLine = it->second.shadowedByLine;
            }
        }
        s = s->parent;
    }
}

bool SemanticAnalyzer::isPoisoned(const std::string& name) const {
    Scope* s = symTab.getCurrentScope();
    while (s != nullptr) {
        auto it = poisonedIdentifiers.find(s->id);
        if (it != poisonedIdentifiers.end() && it->second.count(name) > 0) {
            return true;
        }
        s = s->parent;
    }
    return false;
}

void SemanticAnalyzer::poisonIdentifier(const std::string& name) {
    Scope* s = symTab.getCurrentScope();
    int scopeId = s ? s->id : 0;
    poisonedIdentifiers[scopeId].insert(name);
}

std::string SemanticAnalyzer::formatShadowTrace(const Symbol* sym) const {
    if (!sym || sym->kind == SymbolKind::FUNCTION) return "";
    Symbol* outer = symTab.lookupShadowed(sym->name);
    if (!outer || !sym->type || !outer->type) return "";
    return " Scope trace: '" + sym->name + "' resolves to the " +
           sym->type->toString() + " declared on line " + std::to_string(sym->line) +
           " (shadowing the " + outer->type->toString() +
           " declared on line " + std::to_string(outer->line) + ").";
}

std::string SemanticAnalyzer::collectExprContextNotes(const ExprNode* expr, bool includeDeclLine) const {
    std::vector<std::string> varNames;
    collectExprVars(expr, varNames);
    std::string declNotes;
    std::string shadowNotes;
    for (const auto& name : varNames) {
        Symbol* sym = symTab.lookup(name);
        if (!sym || sym->kind == SymbolKind::FUNCTION || !sym->type || sym->type->isError()) continue;
        if (includeDeclLine) {
            if (!declNotes.empty()) declNotes += ", ";
            declNotes += "'" + sym->name + "' declared as '" + sym->type->toString() +
                         "' on line " + std::to_string(sym->line);
        }
        shadowNotes += formatShadowTrace(sym);
    }
    std::string result;
    if (!declNotes.empty()) {
        result += " (" + declNotes + ")";
    }
    result += shadowNotes;
    return result;
}

std::string SemanticAnalyzer::exprToString(const ExprNode* expr) {
    if (!expr) return "<condition>";
    if (const auto* lit = dynamic_cast<const LiteralExprNode*>(expr)) {
        return lit->rawValue;
    }
    if (const auto* var = dynamic_cast<const VariableExprNode*>(expr)) {
        return var->name;
    }
    if (const auto* arr = dynamic_cast<const ArrayAccessExprNode*>(expr)) {
        return arr->name + "[" + exprToString(arr->indexExpr.get()) + "]";
    }
    if (const auto* un = dynamic_cast<const UnaryExprNode*>(expr)) {
        return un->opStr + exprToString(un->expr.get());
    }
    if (const auto* bin = dynamic_cast<const BinaryExprNode*>(expr)) {
        return exprToString(bin->lhs.get()) + " " + bin->opStr + " " + exprToString(bin->rhs.get());
    }
    if (const auto* call = dynamic_cast<const CallExprNode*>(expr)) {
        std::string res = call->callee + "(";
        for (std::size_t i = 0; i < call->args.size(); ++i) {
            if (i > 0) res += ", ";
            res += exprToString(call->args[i].get());
        }
        res += ")";
        return res;
    }
    return "<expr>";
}

void SemanticAnalyzer::registerTopLevelDeclarations(ProgramNode* program) {
    for (const auto& decl : program->declarations) {
        registerTopLevelDeclaration(decl.get());
    }
}

void SemanticAnalyzer::registerTopLevelDeclaration(ASTNode* declaration) {
    if (auto funcDecl = dynamic_cast<FunctionDeclNode*>(declaration)) {
        std::vector<TypePtr> paramTypes;
        for (const auto& p : funcDecl->params) {
            if (hasVoidElementType(p.type)) {
                reporter.reportError(p.line, p.column,
                    "Parameter '" + p.name + "' (declared on line " + std::to_string(p.line) + ") cannot have type 'void'",
                    "Use a value type such as int, float, bool, or char for this parameter.");
                paramTypes.push_back(Type::getError());
            } else {
                paramTypes.push_back(p.type);
            }
        }
        TypePtr funcType = Type::makeFunction(funcDecl->returnType, paramTypes);
        Symbol sym(funcDecl->name, funcType, SymbolKind::FUNCTION, 0, funcDecl->line, funcDecl->column,
                   true, InitState::DEFINITELY_INITIALIZED);
        if (!symTab.declare(sym)) {
            Symbol* prev = symTab.lookupCurrentScope(funcDecl->name);
            int prevLine = prev ? prev->line : 0;
            reporter.reportError(funcDecl->line, funcDecl->column,
                "Redeclaration of function '" + funcDecl->name + "' (previously declared at line " + std::to_string(prevLine) + ")",
                "Rename the function or remove the duplicate function definition.");
        }
    } else if (auto varDecl = dynamic_cast<VarDeclNode*>(declaration)) {
        // Global types are validated when their declarations are visited.
        TypePtr symbolType = hasVoidElementType(varDecl->declaredType)
            ? Type::getError() : varDecl->declaredType;
        // Global variables and arrays have static/allocated storage; scalar globals without
        // explicit initializers are treated as zero-initialized at program startup.
        Symbol sym(varDecl->varName, symbolType, SymbolKind::VARIABLE, 0, varDecl->line, varDecl->column,
                   true, InitState::DEFINITELY_INITIALIZED);
        if (!symTab.declare(sym)) {
            Symbol* prev = symTab.lookupCurrentScope(varDecl->varName);
            int prevLine = prev ? prev->line : 0;
            reporter.reportError(varDecl->line, varDecl->column,
                "Redeclaration of global variable '" + varDecl->varName + "' (previously declared at line " + std::to_string(prevLine) + ")",
                "Rename the global variable or assign to the existing global inside a function.");
        }
    }
}

void SemanticAnalyzer::visit(ProgramNode* node) {
    for (const auto& decl : node->declarations) {
        decl->accept(this);
    }
}

void SemanticAnalyzer::visit(VarDeclNode* node) {
    // Validate global and local types once, including array element types.
    const bool invalidType = hasVoidElementType(node->declaredType);
    if (invalidType) {
        const std::string message = node->declaredType->isArray()
            ? "Array '" + node->varName + "' (declared on line " + std::to_string(node->line) + ") cannot have element type 'void'"
            : "Variable '" + node->varName + "' (declared on line " + std::to_string(node->line) + ") cannot be declared with type 'void'";
        reporter.reportError(node->line, node->column,
            message, "Use int, float, bool, or char as the variable or array element type.");
    }

    // If inside a function or local block, declare in current scope.
    // Declare scalar locals without initializers (or while evaluating their own initializer)
    // as UNINITIALIZED first so self-referential initializers like `int x = x + 1;` are caught.
    bool declaredInLocalScope = false;
    if (symTab.getCurrentLevel() > 0) {
        bool initiallyReady = invalidType || node->isArray || (node->declaredType && node->declaredType->isArray());
        InitState startState = initiallyReady ? InitState::DEFINITELY_INITIALIZED : InitState::UNINITIALIZED;
        Symbol sym(node->varName, invalidType ? Type::getError() : node->declaredType, SymbolKind::VARIABLE,
                   symTab.getCurrentLevel(), node->line, node->column, initiallyReady, startState);
        if (!symTab.declare(sym)) {
            Symbol* prev = symTab.lookupCurrentScope(node->varName);
            int prevLine = prev ? prev->line : 0;
            reporter.reportError(node->line, node->column,
                "Redeclaration of identifier '" + node->varName + "' in the same scope (previously declared at line " + std::to_string(prevLine) + ")",
                "Rename the variable or assign a new value to the existing variable declared on line " + std::to_string(prevLine) + ".");
        } else {
            declaredInLocalScope = true;
        }
    }

    // Type check initialization expression if present
    if (node->initExpr) {
        node->initExpr->accept(this);
        if (!invalidType && !node->declaredType->isError() &&
            !node->initExpr->inferredType->isError() &&
            !node->initExpr->inferredType->isAssignableTo(*node->declaredType)) {
            Symbol* currentSym = symTab.lookupCurrentScope(node->varName);
            std::string shadowNote = formatShadowTrace(currentSym);
            std::string exprNotes = collectExprContextNotes(node->initExpr.get(), true);
            reporter.reportError(node->line, node->column,
                "Type mismatch in variable initialization: cannot assign '" +
                node->initExpr->inferredType->toString() + "' to variable '" +
                node->varName + "' of type '" + node->declaredType->toString() +
                "' (declared on line " + std::to_string(node->line) + ")" + exprNotes + shadowNote,
                "Use an initializer of type '" + node->declaredType->toString() + "' or change the declared type to match the value.");
        }

        // Transition local variable to DEFINITELY_INITIALIZED after its initializer is evaluated.
        // Even if the initializer had a type error, marking the variable initialized suppresses
        // cascading uninitialized-read diagnostics on later statements.
        if (declaredInLocalScope) {
            Symbol* currentSym = symTab.lookupCurrentScope(node->varName);
            if (currentSym) {
                currentSym->isInitialized = true;
                currentSym->initState = InitState::DEFINITELY_INITIALIZED;
                currentSym->uninitReason.clear();
                currentSym->uninitBranchLine = 0;
                currentSym->reportedUninitRead = false;
                currentSym->shadowedByLine = 0;
                if (Symbol* outer = symTab.lookupShadowed(node->varName)) {
                    if (outer->initState != InitState::DEFINITELY_INITIALIZED) {
                        outer->shadowedByLine = node->line;
                    }
                }
            }
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
    currentFunctionDeclLine = node->line;

    // Register parameters in the function scope as DEFINITELY_INITIALIZED
    for (const auto& p : node->params) {
        // Invalid parameter types were diagnosed when signatures were collected.
        TypePtr parameterType = hasVoidElementType(p.type) ? Type::getError() : p.type;
        Symbol sym(p.name, parameterType, SymbolKind::PARAMETER, symTab.getCurrentLevel(),
                   p.line, p.column, true, InitState::DEFINITELY_INITIALIZED);
        if (!symTab.declare(sym)) {
            Symbol* prev = symTab.lookupCurrentScope(p.name);
            int prevLine = prev ? prev->line : p.line;
            reporter.reportError(p.line, p.column,
                "Duplicate parameter name '" + p.name + "' in function definition '" + node->name +
                "' (previously declared at line " + std::to_string(prevLine) + ")",
                "Give each parameter in '" + node->name + "' a distinct identifier.");
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
            "Non-void function '" + node->name + "' (declared on line " + std::to_string(node->line) +
            ") may reach the end without returning a value of type '" +
            node->returnType->toString() + "'",
            "Return a compatible value on every path, including an else branch or a return after the conditional or loop.");
    }

    inFunction = false;
    currentFunctionName = "";
    currentFunctionReturnType = nullptr;
    currentFunctionDeclLine = 0;
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
    // Evaluate RHS and index expressions BEFORE updating the target symbol's initialization state,
    // so self-referential assignments like `x = x + 1;` while `x` is uninitialized are detected.
    if (node->rhs) {
        node->rhs->accept(this);
    }
    if (node->indexExpr) {
        node->indexExpr->accept(this);
    }

    Symbol* sym = symTab.lookup(node->varName);
    if (!sym) {
        if (!isPoisoned(node->varName)) {
            reporter.reportError(node->line, node->column,
                "Undeclared identifier '" + node->varName + "' in assignment statement (no declaration in scope)",
                "Declare '" + node->varName + "' in this scope or an enclosing scope before assigning to it.");
            poisonIdentifier(node->varName);
        }
        return;
    }

    if (sym->type->isError()) {
        sym->isInitialized = true;
        sym->initState = InitState::DEFINITELY_INITIALIZED;
        return;
    }

    if (sym->kind == SymbolKind::FUNCTION) {
        reporter.reportError(node->line, node->column,
            "Cannot assign to function '" + node->varName + "' (declared on line " + std::to_string(sym->line) + "; not a modifiable l-value)",
            "Assign to a variable or an array element instead of a function name.");
        return;
    }

    std::string targetShadow = formatShadowTrace(sym);
    std::string rhsContext = collectExprContextNotes(node->rhs.get(), false);

    if (node->indexExpr == nullptr) {
        // Simple variable assignment
        if (sym->type->isArray()) {
            reporter.reportError(node->line, node->column,
                "Cannot assign directly to entire array '" + node->varName + "' of type '" + sym->type->toString() +
                "' (declared on line " + std::to_string(sym->line) + ")" + targetShadow,
                "Assign values to individual elements using an integer subscript.");
            return;
        }

        if (node->rhs && !node->rhs->inferredType->isError() && !node->rhs->inferredType->isAssignableTo(*sym->type)) {
            std::string counterfactualNote;
            if (Symbol* outer = symTab.lookupShadowed(sym->name)) {
                if (outer->type && !outer->type->isError() &&
                    node->rhs->inferredType->isAssignableTo(*outer->type)) {
                    counterfactualNote = " Counterfactual check: '" + node->rhs->inferredType->toString() +
                        "' is compatible with the shadowed '" + outer->type->toString() + " " + outer->name +
                        "' on line " + std::to_string(outer->line) + ", but inner '" +
                        sym->type->toString() + " " + sym->name + "' on line " + std::to_string(sym->line) + " masks it.";
                }
            }
            reporter.reportError(node->line, node->column,
                "Type mismatch in assignment: cannot assign '" + node->rhs->inferredType->toString() +
                "' to variable '" + node->varName + "' of type '" + sym->type->toString() +
                "' (declared on line " + std::to_string(sym->line) + ")" + targetShadow + rhsContext + counterfactualNote,
                "Assign a value compatible with '" + sym->type->toString() + "'; only int-to-float widening is implicit.");
        }

        // Mark target variable as DEFINITELY_INITIALIZED on this control-flow path.
        sym->isInitialized = true;
        sym->initState = InitState::DEFINITELY_INITIALIZED;
        sym->uninitReason.clear();
        sym->uninitBranchLine = 0;
        sym->reportedUninitRead = false;
        sym->shadowedByLine = 0;
        if (Symbol* outer = symTab.lookupShadowed(sym->name)) {
            if (outer->initState != InitState::DEFINITELY_INITIALIZED) {
                outer->shadowedByLine = sym->line;
            }
        }
    } else {
        // Array subscript assignment: arr[idx] = rhs
        if (!sym->type->isArray()) {
            reporter.reportError(node->line, node->column,
                "Subscripted variable '" + node->varName + "' (declared on line " + std::to_string(sym->line) +
                ") is not an array (type is '" + sym->type->toString() + "')" + targetShadow,
                "Remove the array subscript or declare '" + node->varName + "' as an array.");
            return;
        }

        if (!node->indexExpr->inferredType->isError() && !node->indexExpr->inferredType->isInteger()) {
            std::string idxNotes = collectExprContextNotes(node->indexExpr.get(), true);
            reporter.reportError(node->indexExpr->line, node->indexExpr->column,
                "Array subscript index for '" + node->varName + "' (declared on line " + std::to_string(sym->line) +
                ") must be of type 'int', got '" + node->indexExpr->inferredType->toString() + "'" + idxNotes,
                "Use an integer expression for the array index.");
        }

        if (node->rhs && !node->rhs->inferredType->isError() && !node->rhs->inferredType->isAssignableTo(*sym->type->elementType)) {
            reporter.reportError(node->line, node->column,
                "Type mismatch in array assignment: cannot assign '" + node->rhs->inferredType->toString() +
                "' to element of '" + node->varName + "' of type '" + sym->type->elementType->toString() +
                "' (declared on line " + std::to_string(sym->line) + ")" + targetShadow + rhsContext,
                "Assign a value compatible with the array element type '" + sym->type->elementType->toString() + "'.");
        }
    }
}

void SemanticAnalyzer::visit(IfStmtNode* node) {
    if (node->condition) {
        node->condition->accept(this);
        if (!node->condition->inferredType->isError() && !node->condition->inferredType->isBoolean()) {
            std::string condNotes = collectExprContextNotes(node->condition.get(), true);
            reporter.reportError(node->condition->line, node->condition->column,
                "Condition in 'if' statement must evaluate to 'bool', got '" +
                node->condition->inferredType->toString() + "'" + condNotes,
                "Use a Boolean value or a comparison such as value != 0.");
        }
    }

    InitEnv preEnv = captureInitEnv();
    std::string condStr = exprToString(node->condition.get());

    // Analyze 'then' branch in a cloned initialization environment
    if (node->thenBranch) {
        bool isBlock = dynamic_cast<BlockNode*>(node->thenBranch.get()) != nullptr;
        if (!isBlock) symTab.enterScope("Then@L" + std::to_string(node->line));
        node->thenBranch->accept(this);
        if (!isBlock) symTab.exitScope();
    }
    InitEnv thenEnv = captureInitEnv();
    bool thenReturns = guaranteesReturn(node->thenBranch.get());

    // Restore pre-branch initialization state and analyze 'else' branch
    restoreInitEnv(preEnv);
    if (node->elseBranch) {
        bool isBlock = dynamic_cast<BlockNode*>(node->elseBranch.get()) != nullptr;
        if (!isBlock) symTab.enterScope("Else@L" + std::to_string(node->elseBranch->line));
        node->elseBranch->accept(this);
        if (!isBlock) symTab.exitScope();
    }
    InitEnv elseEnv = captureInitEnv();
    bool elseReturns = node->elseBranch ? guaranteesReturn(node->elseBranch.get()) : false;

    // Merge 'then' and 'else' initialization states
    InitEnv mergedEnv;
    for (const auto& pair : preEnv) {
        const SymbolKey& key = pair.first;
        const VarInitSnapshot& preInfo = pair.second;
        const VarInitSnapshot& tInfo = thenEnv[key];
        const VarInitSnapshot& eInfo = elseEnv[key];

        VarInitSnapshot merged = preInfo;
        if (preInfo.state == InitState::DEFINITELY_INITIALIZED) {
            merged.state = InitState::DEFINITELY_INITIALIZED;
            merged.reason.clear();
            merged.branchLine = 0;
            merged.reportedUninitRead = false;
        } else if (thenReturns && !elseReturns && eInfo.state == InitState::DEFINITELY_INITIALIZED) {
            merged.state = InitState::DEFINITELY_INITIALIZED;
            merged.reason.clear();
            merged.branchLine = 0;
            merged.reportedUninitRead = false;
        } else if (elseReturns && !thenReturns && tInfo.state == InitState::DEFINITELY_INITIALIZED) {
            merged.state = InitState::DEFINITELY_INITIALIZED;
            merged.reason.clear();
            merged.branchLine = 0;
            merged.reportedUninitRead = false;
        } else if (tInfo.state == InitState::DEFINITELY_INITIALIZED &&
                   eInfo.state == InitState::DEFINITELY_INITIALIZED) {
            merged.state = InitState::DEFINITELY_INITIALIZED;
            merged.reason.clear();
            merged.branchLine = 0;
            merged.reportedUninitRead = false;
        } else if (tInfo.state == InitState::UNINITIALIZED &&
                   eInfo.state == InitState::UNINITIALIZED) {
            merged.state = InitState::UNINITIALIZED;
            merged.reportedUninitRead =
                preInfo.reportedUninitRead || tInfo.reportedUninitRead || eInfo.reportedUninitRead;
        } else {
            merged.state = InitState::MAYBE_INITIALIZED;
            merged.reportedUninitRead =
                preInfo.reportedUninitRead || (tInfo.reportedUninitRead && eInfo.reportedUninitRead);

            if (node->elseBranch == nullptr) {
                merged.branchLine = node->line;
                merged.reason = "the branch where '" + condStr +
                                "' is false (missing else on line " + std::to_string(node->line) +
                                ") does not assign it";
            } else if (tInfo.state == InitState::DEFINITELY_INITIALIZED &&
                       eInfo.state == InitState::UNINITIALIZED) {
                merged.branchLine = node->elseBranch->line;
                merged.reason = "the else branch (where '" + condStr +
                                "' is false on line " + std::to_string(node->elseBranch->line) +
                                ") does not assign it";
            } else if (tInfo.state == InitState::UNINITIALIZED &&
                       eInfo.state == InitState::DEFINITELY_INITIALIZED) {
                merged.branchLine = node->line;
                merged.reason = "the then branch (where '" + condStr +
                                "' is true on line " + std::to_string(node->line) +
                                ") does not assign it";
            } else if (tInfo.state == InitState::MAYBE_INITIALIZED &&
                       eInfo.state == InitState::DEFINITELY_INITIALIZED) {
                merged.branchLine = tInfo.branchLine;
                merged.reason = tInfo.reason;
            } else if (eInfo.state == InitState::MAYBE_INITIALIZED &&
                       tInfo.state == InitState::DEFINITELY_INITIALIZED) {
                merged.branchLine = eInfo.branchLine;
                merged.reason = eInfo.reason;
            } else if (eInfo.state == InitState::UNINITIALIZED) {
                merged.branchLine = node->elseBranch->line;
                merged.reason = "the else branch (where '" + condStr +
                                "' is false on line " + std::to_string(node->elseBranch->line) +
                                ") does not assign it";
            } else if (tInfo.state == InitState::UNINITIALIZED) {
                merged.branchLine = node->line;
                merged.reason = "the then branch (where '" + condStr +
                                "' is true on line " + std::to_string(node->line) +
                                ") does not assign it";
            } else {
                merged.branchLine = tInfo.branchLine ? tInfo.branchLine : node->line;
                merged.reason = tInfo.reason;
            }
        }
        if (merged.state == InitState::DEFINITELY_INITIALIZED) {
            merged.shadowedByLine = 0;
        } else {
            merged.shadowedByLine = tInfo.shadowedByLine ? tInfo.shadowedByLine
                                  : (eInfo.shadowedByLine ? eInfo.shadowedByLine : preInfo.shadowedByLine);
        }
        mergedEnv[key] = merged;
    }
    restoreInitEnv(mergedEnv);
}

void SemanticAnalyzer::visit(WhileStmtNode* node) {
    if (node->condition) {
        node->condition->accept(this);
        if (!node->condition->inferredType->isError() && !node->condition->inferredType->isBoolean()) {
            std::string condNotes = collectExprContextNotes(node->condition.get(), true);
            reporter.reportError(node->condition->line, node->condition->column,
                "Condition in 'while' statement must evaluate to 'bool', got '" +
                node->condition->inferredType->toString() + "'" + condNotes,
                "Use a Boolean value or a comparison such as value != 0.");
        }
    }

    InitEnv preEnv = captureInitEnv();

    if (node->body) {
        bool isBlock = dynamic_cast<BlockNode*>(node->body.get()) != nullptr;
        if (!isBlock) symTab.enterScope("While@L" + std::to_string(node->line));
        node->body->accept(this);
        if (!isBlock) symTab.exitScope();
    }

    InitEnv bodyEnv = captureInitEnv();

    // Because a while loop body may execute zero times, an assignment inside the loop
    // to a previously uninitialized variable merges to MAYBE_INITIALIZED after the loop.
    InitEnv mergedEnv;
    for (const auto& pair : preEnv) {
        const SymbolKey& key = pair.first;
        const VarInitSnapshot& preInfo = pair.second;
        const VarInitSnapshot& bInfo = bodyEnv[key];

        VarInitSnapshot merged = preInfo;
        if (preInfo.state == InitState::DEFINITELY_INITIALIZED) {
            merged.state = InitState::DEFINITELY_INITIALIZED;
            merged.reason.clear();
            merged.branchLine = 0;
            merged.reportedUninitRead = false;
            merged.shadowedByLine = 0;
        } else if (preInfo.state == InitState::UNINITIALIZED) {
            merged.shadowedByLine = bInfo.shadowedByLine ? bInfo.shadowedByLine : preInfo.shadowedByLine;
            if (bInfo.state == InitState::DEFINITELY_INITIALIZED ||
                bInfo.state == InitState::MAYBE_INITIALIZED) {
                merged.state = InitState::MAYBE_INITIALIZED;
                merged.branchLine = node->line;
                merged.reason = "the while loop on line " + std::to_string(node->line) +
                                " may execute zero times";
                merged.reportedUninitRead = false;
            } else {
                merged.state = InitState::UNINITIALIZED;
                merged.reportedUninitRead = preInfo.reportedUninitRead || bInfo.reportedUninitRead;
            }
        } else {
            // preInfo.state == InitState::MAYBE_INITIALIZED
            merged.state = InitState::MAYBE_INITIALIZED;
            merged.shadowedByLine = bInfo.shadowedByLine ? bInfo.shadowedByLine : preInfo.shadowedByLine;
            if (bInfo.state == InitState::DEFINITELY_INITIALIZED) {
                merged.branchLine = node->line;
                merged.reason = preInfo.reason + ", and the while loop on line " +
                                std::to_string(node->line) + " may execute zero times";
                merged.reportedUninitRead = false;
            } else {
                merged.branchLine = preInfo.branchLine;
                merged.reason = preInfo.reason;
                merged.reportedUninitRead = preInfo.reportedUninitRead || bInfo.reportedUninitRead;
            }
        }
        mergedEnv[key] = merged;
    }
    restoreInitEnv(mergedEnv);
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
            std::string exprNotes = collectExprContextNotes(node->value.get(), true);
            if (currentFunctionReturnType->isVoid()) {
                reporter.reportError(node->line, node->column,
                    "Void function '" + currentFunctionName + "' (declared on line " +
                    std::to_string(currentFunctionDeclLine) + ") cannot return a value" + exprNotes,
                    "Use return; or change the function's declared return type to match the value.");
            } else if (!node->value->inferredType->isAssignableTo(*currentFunctionReturnType)) {
                reporter.reportError(node->line, node->column,
                    "Return type mismatch: function '" + currentFunctionName + "' (declared on line " +
                    std::to_string(currentFunctionDeclLine) + ") expects return type '" +
                    currentFunctionReturnType->toString() + "', but got '" +
                    node->value->inferredType->toString() + "'" + exprNotes,
                    "Return a value compatible with '" + currentFunctionReturnType->toString() + "' or change the function's declared return type.");
            }
        }
    } else {
        if (!currentFunctionReturnType->isVoid()) {
            reporter.reportError(node->line, node->column,
                "Non-void function '" + currentFunctionName + "' (declared on line " +
                std::to_string(currentFunctionDeclLine) + ") must return a value of type '" +
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
        if (!isPoisoned(node->name)) {
            reporter.reportError(node->line, node->column,
                "Undeclared identifier '" + node->name + "' (no declaration in scope)",
                "Declare '" + node->name + "' in this scope or an enclosing scope before using it.");
            poisonIdentifier(node->name);
        }
        node->inferredType = Type::getError();
        return;
    }

    if (sym->kind == SymbolKind::FUNCTION) {
        reporter.reportError(node->line, node->column,
            "Identifier '" + node->name + "' (declared on line " + std::to_string(sym->line) +
            ") refers to a function, not a variable",
            "Call '" + node->name + "' with parentheses or use a variable name.");
        node->inferredType = Type::getError();
        return;
    }

    if (!sym->type->isError() && !sym->type->isArray()) {
        std::string shadowTrace = formatShadowTrace(sym);
        std::string escapeTrace;
        if (sym->shadowedByLine > 0) {
            escapeTrace = " Shadow escape trace: an inner declaration of '" + sym->name +
                          "' on line " + std::to_string(sym->shadowedByLine) +
                          " shadowed this variable, leaving the outer '" + sym->name +
                          "' (declared on line " + std::to_string(sym->line) + ") uninitialized.";
        }
        if (sym->initState == InitState::UNINITIALIZED) {
            if (!sym->reportedUninitRead) {
                std::string fixHint = sym->shadowedByLine > 0
                    ? "Remove the inner type keyword on line " + std::to_string(sym->shadowedByLine) +
                      " if you intended to assign to the outer '" + sym->name + "' declared on line " +
                      std::to_string(sym->line) + ", or initialize '" + sym->name + "' before line " +
                      std::to_string(node->line) + "."
                    : "Initialize '" + sym->name + "' at its declaration on line " + std::to_string(sym->line) +
                      " or assign a '" + sym->type->toString() + "' value to it before line " + std::to_string(node->line) + ".";
                reporter.reportError(node->line, node->column,
                    "Variable '" + sym->name + "' declared on line " + std::to_string(sym->line) +
                    " is used on line " + std::to_string(node->line) +
                    " before any value is assigned to it." + shadowTrace + escapeTrace,
                    fixHint);
                sym->reportedUninitRead = true;
            }
        } else if (sym->initState == InitState::MAYBE_INITIALIZED) {
            if (!sym->reportedUninitRead) {
                reporter.reportError(node->line, node->column,
                    "Variable '" + sym->name + "' (declared on line " + std::to_string(sym->line) +
                    ") may be uninitialized here (used on line " + std::to_string(node->line) +
                    ") because " + sym->uninitReason + "." + shadowTrace + escapeTrace,
                    "Initialize '" + sym->name + "' at its declaration on line " + std::to_string(sym->line) +
                    " or ensure all control-flow paths assign a '" + sym->type->toString() +
                    "' value before line " + std::to_string(node->line) + ".");
                sym->reportedUninitRead = true;
            }
        }
    }

    node->inferredType = sym->type;
}

void SemanticAnalyzer::visit(ArrayAccessExprNode* node) {
    // Visit independent subexpressions even when the array name is invalid.
    if (node->indexExpr) node->indexExpr->accept(this);

    Symbol* sym = symTab.lookup(node->name);
    if (!sym) {
        if (!isPoisoned(node->name)) {
            reporter.reportError(node->line, node->column,
                "Undeclared identifier '" + node->name + "' (no declaration in scope)",
                "Declare the array in this scope or an enclosing scope before using it.");
            poisonIdentifier(node->name);
        }
        node->inferredType = Type::getError();
        return;
    }

    if (sym->type->isError()) {
        node->inferredType = Type::getError();
        return;
    }

    std::string shadowTrace = formatShadowTrace(sym);

    if (!sym->type->isArray()) {
        reporter.reportError(node->line, node->column,
            "Subscripted value '" + node->name + "' (declared on line " + std::to_string(sym->line) +
            ") is not an array (type is '" + sym->type->toString() + "')" + shadowTrace,
            "Remove the subscript or declare '" + node->name + "' as an array.");
        node->inferredType = Type::getError();
        return;
    }

    if (node->indexExpr) {
        if (node->indexExpr->inferredType->isError()) {
            node->inferredType = Type::getError();
            return;
        }
        if (!node->indexExpr->inferredType->isInteger()) {
            std::string idxNotes = collectExprContextNotes(node->indexExpr.get(), true);
            reporter.reportError(node->indexExpr->line, node->indexExpr->column,
                "Array subscript index for '" + node->name + "' (declared on line " + std::to_string(sym->line) +
                ") must be of type 'int', got '" + node->indexExpr->inferredType->toString() + "'" + idxNotes,
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

    std::string contextNotes = collectExprContextNotes(node, true);

    switch (node->op) {
        // Arithmetic operators: +, -, *, /
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::STAR:
        case TokenType::SLASH: {
            if (!leftT->isNumeric() || !rightT->isNumeric()) {
                reporter.reportError(node->line, node->column,
                    "Arithmetic operator '" + node->opStr + "' requires numeric operands, got '" +
                    leftT->toString() + "' and '" + rightT->toString() + "'" + contextNotes,
                    "Provide 'int' or 'float' operands to arithmetic operator '" + node->opStr + "'.");
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
                    leftT->toString() + "' and '" + rightT->toString() + "'" + contextNotes,
                    "Provide two 'int' operands to the modulo operator '%'.");
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
                    leftT->toString() + "' and '" + rightT->toString() + "'" + contextNotes,
                    "Compare two numeric ('int' or 'float') expressions.");
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
                    "' and '" + rightT->toString() + "' with equality operator '" + node->opStr + "'" + contextNotes,
                    "Compare two numeric values, two booleans, or two characters.");
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
                    leftT->toString() + "' and '" + rightT->toString() + "'" + contextNotes,
                    "Use boolean expressions on both sides of '" + node->opStr + "'.");
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

    std::string contextNotes = collectExprContextNotes(node->expr.get(), true);

    if (node->op == TokenType::NOT) {
        if (!innerT->isBoolean()) {
            reporter.reportError(node->line, node->column,
                "Logical NOT operator '!' requires boolean operand, got '" + innerT->toString() + "'" + contextNotes,
                "Apply '!' to a 'bool' expression or comparison.");
            node->inferredType = Type::getError();
        } else {
            node->inferredType = Type::getBool();
        }
    } else if (node->op == TokenType::MINUS || node->op == TokenType::PLUS) {
        if (!innerT->isNumeric()) {
            reporter.reportError(node->line, node->column,
                "Unary operator '" + node->opStr + "' requires numeric operand, got '" + innerT->toString() + "'" + contextNotes,
                "Apply unary '" + node->opStr + "' to an 'int' or 'float' expression.");
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
        if (!isPoisoned(node->callee)) {
            reporter.reportError(node->line, node->column,
                "Call to undeclared function '" + node->callee + "' (no declaration in scope)",
                "Define '" + node->callee + "' as a top-level function; its definition may appear later in the file.");
            poisonIdentifier(node->callee);
        }
        node->inferredType = Type::getError();
        return;
    }

    if (sym->type->isError()) {
        node->inferredType = Type::getError();
        return;
    }

    if (sym->kind != SymbolKind::FUNCTION) {
        std::string shadowTrace = formatShadowTrace(sym);
        reporter.reportError(node->line, node->column,
            "Called object '" + node->callee + "' (declared on line " + std::to_string(sym->line) +
            ") is not a function (type is '" + sym->type->toString() + "')" + shadowTrace,
            "Use a declared function name for the call or remove the argument list.");
        node->inferredType = Type::getError();
        return;
    }

    const auto& expectedParams = sym->type->paramTypes;
    if (node->args.size() != expectedParams.size()) {
        reporter.reportError(node->line, node->column,
            "Function '" + node->callee + "' (declared on line " + std::to_string(sym->line) +
            ") expects " + std::to_string(expectedParams.size()) +
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
            std::string argNotes = collectExprContextNotes(node->args[i].get(), true);
            reporter.reportError(node->args[i]->line, node->args[i]->column,
                "Argument " + std::to_string(i + 1) + " of function '" + node->callee +
                "' (declared on line " + std::to_string(sym->line) +
                "): type mismatch, expected '" + paramT->toString() + "', got '" + argT->toString() + "'" + argNotes,
                "Use a value compatible with parameter type '" + paramT->toString() + "' for argument " + std::to_string(i + 1) + ".");
            invalidArgument = true;
        }
    }

    node->inferredType = invalidArgument ? Type::getError() : sym->type->returnType;
}
