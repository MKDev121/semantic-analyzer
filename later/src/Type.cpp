#include "Type.hpp"
#include <sstream>

Type::Type(TypeKind kind) : kind(kind), arraySize(0) {}

bool Type::isNumeric() const {
    return kind == TypeKind::TYPE_INT || kind == TypeKind::TYPE_FLOAT;
}

bool Type::isInteger() const {
    return kind == TypeKind::TYPE_INT;
}

bool Type::isFloat() const {
    return kind == TypeKind::TYPE_FLOAT;
}

bool Type::isBoolean() const {
    return kind == TypeKind::TYPE_BOOL;
}

bool Type::isChar() const {
    return kind == TypeKind::TYPE_CHAR;
}

bool Type::isVoid() const {
    return kind == TypeKind::TYPE_VOID;
}

bool Type::isArray() const {
    return kind == TypeKind::TYPE_ARRAY;
}

bool Type::isFunction() const {
    return kind == TypeKind::TYPE_FUNCTION;
}

bool Type::isError() const {
    return kind == TypeKind::TYPE_ERROR;
}

bool Type::equals(const Type& other) const {
    if (kind == TypeKind::TYPE_ERROR || other.kind == TypeKind::TYPE_ERROR) {
        return false;
    }
    if (kind != other.kind) {
        return false;
    }
    if (kind == TypeKind::TYPE_ARRAY) {
        if (arraySize != other.arraySize) return false;
        if (!elementType || !other.elementType) return false;
        return elementType->equals(*other.elementType);
    }
    if (kind == TypeKind::TYPE_FUNCTION) {
        if (!returnType || !other.returnType) return false;
        if (!returnType->equals(*other.returnType)) return false;
        if (paramTypes.size() != other.paramTypes.size()) return false;
        for (size_t i = 0; i < paramTypes.size(); i++) {
            if (!paramTypes[i]->equals(*other.paramTypes[i])) return false;
        }
        return true;
    }
    return true;
}

bool Type::isAssignableTo(const Type& target) const {
    if (isError() || target.isError()) {
        return false;
    }
    // Cannot assign function types or void
    if (isFunction() || target.isFunction() || isVoid() || target.isVoid()) {
        return false;
    }
    // Arrays must match completely
    if (isArray() || target.isArray()) {
        return equals(target);
    }
    // Exact match for primitives
    if (equals(target)) {
        return true;
    }
    // Numeric implicit widening: int can be assigned to float
    if (isInteger() && target.isFloat()) {
        return true;
    }
    return false;
}

std::string Type::toString() const {
    switch (kind) {
        case TypeKind::TYPE_INT: return "int";
        case TypeKind::TYPE_FLOAT: return "float";
        case TypeKind::TYPE_BOOL: return "bool";
        case TypeKind::TYPE_CHAR: return "char";
        case TypeKind::TYPE_VOID: return "void";
        case TypeKind::TYPE_ERROR: return "<type-error>";
        case TypeKind::TYPE_ARRAY: {
            std::string elem = elementType ? elementType->toString() : "unknown";
            return elem + "[" + std::to_string(arraySize) + "]";
        }
        case TypeKind::TYPE_FUNCTION: {
            std::ostringstream oss;
            oss << (returnType ? returnType->toString() : "void") << "(";
            for (size_t i = 0; i < paramTypes.size(); i++) {
                if (i > 0) oss << ", ";
                oss << (paramTypes[i] ? paramTypes[i]->toString() : "unknown");
            }
            oss << ")";
            return oss.str();
        }
    }
    return "unknown";
}

TypePtr Type::getInt() {
    static TypePtr instance = std::make_shared<Type>(TypeKind::TYPE_INT);
    return instance;
}

TypePtr Type::getFloat() {
    static TypePtr instance = std::make_shared<Type>(TypeKind::TYPE_FLOAT);
    return instance;
}

TypePtr Type::getBool() {
    static TypePtr instance = std::make_shared<Type>(TypeKind::TYPE_BOOL);
    return instance;
}

TypePtr Type::getChar() {
    static TypePtr instance = std::make_shared<Type>(TypeKind::TYPE_CHAR);
    return instance;
}

TypePtr Type::getVoid() {
    static TypePtr instance = std::make_shared<Type>(TypeKind::TYPE_VOID);
    return instance;
}

TypePtr Type::getError() {
    static TypePtr instance = std::make_shared<Type>(TypeKind::TYPE_ERROR);
    return instance;
}

TypePtr Type::makeArray(TypePtr elemType, int size) {
    auto t = std::make_shared<Type>(TypeKind::TYPE_ARRAY);
    t->elementType = elemType;
    t->arraySize = size;
    return t;
}

TypePtr Type::makeFunction(TypePtr retType, std::vector<TypePtr> params) {
    auto t = std::make_shared<Type>(TypeKind::TYPE_FUNCTION);
    t->returnType = retType;
    t->paramTypes = std::move(params);
    return t;
}
