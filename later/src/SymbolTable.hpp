#pragma once

#include "Type.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>

enum class SymbolKind {
    VARIABLE,
    PARAMETER,
    FUNCTION
};

inline std::string symbolKindToString(SymbolKind kind) {
    switch (kind) {
        case SymbolKind::VARIABLE: return "Variable";
        case SymbolKind::PARAMETER: return "Parameter";
        case SymbolKind::FUNCTION: return "Function";
    }
    return "Unknown";
}

struct Symbol {
    std::string name;
    TypePtr type;
    SymbolKind kind;
    int scopeLevel;
    int line;
    int col;
    bool isInitialized;

    Symbol() : name(""), type(nullptr), kind(SymbolKind::VARIABLE), scopeLevel(0), line(0), col(0), isInitialized(false) {}
    Symbol(std::string name, TypePtr type, SymbolKind kind, int scopeLevel, int line, int col, bool isInitialized = false)
        : name(std::move(name)), type(std::move(type)), kind(kind), scopeLevel(scopeLevel), line(line), col(col), isInitialized(isInitialized) {}
};

class Scope {
public:
    int id;
    std::string name;
    int level;
    Scope* parent;
    std::unordered_map<std::string, Symbol> symbols;
    std::vector<std::shared_ptr<Scope>> children;

    Scope(int id, std::string name, int level, Scope* parent = nullptr)
        : id(id), name(std::move(name)), level(level), parent(parent) {}
};

class SymbolTable {
public:
    SymbolTable();

    void enterScope(const std::string& scopeName);
    void exitScope();

    bool declare(const Symbol& sym);
    Symbol* lookup(const std::string& name);
    Symbol* lookupCurrentScope(const std::string& name);

    Scope* getCurrentScope() const { return currentScope; }
    std::shared_ptr<Scope> getRootScope() const { return rootScope; }
    int getCurrentLevel() const { return currentScope ? currentScope->level : 0; }

    void printHierarchy(std::ostream& out = std::cout) const;
    void reset();

private:
    std::shared_ptr<Scope> rootScope;
    Scope* currentScope;
    int nextScopeId;

    void printScope(std::ostream& out, const Scope* scope, int indent) const;
};
