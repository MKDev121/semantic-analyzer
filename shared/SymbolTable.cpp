#include "SymbolTable.hpp"
#include <iomanip>

SymbolTable::SymbolTable() : nextScopeId(0) {
    reset();
}

void SymbolTable::reset() {
    nextScopeId = 0;
    rootScope = std::make_shared<Scope>(nextScopeId++, "Global", 0, nullptr);
    currentScope = rootScope.get();
}

void SymbolTable::enterScope(const std::string& scopeName) {
    int nextLevel = currentScope ? currentScope->level + 1 : 0;
    auto newScope = std::make_shared<Scope>(nextScopeId++, scopeName, nextLevel, currentScope);
    if (currentScope) {
        currentScope->children.push_back(newScope);
    }
    currentScope = newScope.get();
}

void SymbolTable::exitScope() {
    if (currentScope && currentScope->parent) {
        currentScope = currentScope->parent;
    }
}

bool SymbolTable::declare(const Symbol& sym) {
    if (!currentScope) return false;
    if (currentScope->symbols.find(sym.name) != currentScope->symbols.end()) {
        return false; // Redeclaration in same scope
    }
    currentScope->symbols[sym.name] = sym;
    return true;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    Scope* s = currentScope;
    while (s != nullptr) {
        auto it = s->symbols.find(name);
        if (it != s->symbols.end()) {
            return &(it->second);
        }
        s = s->parent;
    }
    return nullptr;
}

Symbol* SymbolTable::lookupCurrentScope(const std::string& name) {
    if (!currentScope) return nullptr;
    auto it = currentScope->symbols.find(name);
    if (it != currentScope->symbols.end()) {
        return &(it->second);
    }
    return nullptr;
}

void SymbolTable::printScope(std::ostream& out, const Scope* scope, int indent) const {
    if (!scope) return;
    std::string pad(indent * 2, ' ');
    out << pad << "Scope ID: " << scope->id << " | Name: \"" << scope->name << "\" | Level: " << scope->level << "\n";
    out << pad << "  +----------------------+--------------------+------------+-------+-------+\n";
    out << pad << "  | Symbol Name          | Type               | Kind       | Line  | Col   |\n";
    out << pad << "  +----------------------+--------------------+------------+-------+-------+\n";

    if (scope->symbols.empty()) {
        out << pad << "  | (empty)                                                           |\n";
    } else {
        for (const auto& pair : scope->symbols) {
            const auto& sym = pair.second;
            out << pad << "  | "
                << std::left << std::setw(20) << sym.name << " | "
                << std::setw(18) << (sym.type ? sym.type->toString() : "null") << " | "
                << std::setw(10) << symbolKindToString(sym.kind) << " | "
                << std::right << std::setw(5) << sym.line << " | "
                << std::setw(5) << sym.col << " |\n";
        }
    }
    out << pad << "  +----------------------+--------------------+------------+-------+-------+\n\n";

    for (const auto& child : scope->children) {
        printScope(out, child.get(), indent + 1);
    }
}

void SymbolTable::printHierarchy(std::ostream& out) const {
    out << "=========================== SYMBOL TABLE HIERARCHY ===========================\n";
    printScope(out, rootScope.get(), 0);
    out << "==============================================================================\n";
}
