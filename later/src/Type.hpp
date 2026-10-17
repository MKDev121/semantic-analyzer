#pragma once

#include <string>
#include <vector>
#include <memory>

enum class TypeKind {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_VOID,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_ERROR
};

class Type;
using TypePtr = std::shared_ptr<Type>;

class Type {
public:
    TypeKind kind;
    // For arrays
    TypePtr elementType;
    int arraySize;
    // For functions
    TypePtr returnType;
    std::vector<TypePtr> paramTypes;

    explicit Type(TypeKind kind);

    bool isNumeric() const;
    bool isInteger() const;
    bool isFloat() const;
    bool isBoolean() const;
    bool isChar() const;
    bool isVoid() const;
    bool isArray() const;
    bool isFunction() const;
    bool isError() const;

    bool equals(const Type& other) const;
    bool operator==(const Type& other) const { return equals(other); }
    bool operator!=(const Type& other) const { return !equals(other); }

    // Check if this type can be assigned to a destination of target type
    bool isAssignableTo(const Type& target) const;

    std::string toString() const;

    // Factory methods
    static TypePtr getInt();
    static TypePtr getFloat();
    static TypePtr getBool();
    static TypePtr getChar();
    static TypePtr getVoid();
    static TypePtr getError();
    static TypePtr makeArray(TypePtr elemType, int size);
    static TypePtr makeFunction(TypePtr retType, std::vector<TypePtr> params);
};
