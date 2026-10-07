# Language Specification & Formal Grammar

This document provides the formal language specification for the imperative language supported by the Semantic Analyzer.

---

## 1. Formal Grammar (EBNF)

```ebnf
Program             ::= TopLevelDecl* EOF

TopLevelDecl        ::= VarDecl
                      | FunctionDecl

Type                ::= ("int" | "float" | "bool" | "char" | "void") ("[" INT_LITERAL "]")?

VarDecl             ::= Type IDENTIFIER ("=" Expression)? ";"

FunctionDecl        ::= Type IDENTIFIER "(" ParameterList? ")" Block

ParameterList       ::= Parameter ("," Parameter)*
Parameter           ::= Type IDENTIFIER

Block               ::= "{" Statement* "}"

Statement           ::= Block
                      | VarDecl
                      | IfStatement
                      | WhileStatement
                      | ReturnStatement
                      | AssignmentStatement
                      | ExpressionStatement

IfStatement         ::= "if" "(" Expression ")" Statement ("else" Statement)?
WhileStatement      ::= "while" "(" Expression ")" Statement
ReturnStatement     ::= "return" Expression? ";"
AssignmentStatement ::= (IDENTIFIER | IDENTIFIER "[" Expression "]") "=" Expression ";"
ExpressionStatement ::= Expression ";"

Expression          ::= LogicalOr
LogicalOr           ::= LogicalAnd ("||" LogicalAnd)*
LogicalAnd          ::= Equality ("&&" Equality)*
Equality            ::= Relational (("==" | "!=") Relational)*
Relational          ::= Additive (("<" | "<=" | ">" | ">=") Additive)*
Additive            ::= Multiplicative (("+" | "-") Multiplicative)*
Multiplicative      ::= Unary (("*" | "/" | "%") Unary)*
Unary               ::= ("+" | "-" | "!") Unary
                      | Primary

Primary             ::= IDENTIFIER
                      | IDENTIFIER "[" Expression "]"
                      | IDENTIFIER "(" ArgumentList? ")"
                      | INT_LITERAL
                      | FLOAT_LITERAL
                      | CHAR_LITERAL
                      | "true"
                      | "false"
                      | "(" Expression ")"

ArgumentList        ::= Expression ("," Expression)*
```

---

## 2. Lexical Specification

### 2.1 Keywords
The following tokens are reserved keywords:
```
int, float, bool, char, void, if, else, while, return, true, false
```

### 2.2 Literals
- **Integer Literals (`INT_LITERAL`):** Sequences of ASCII digits `0-9` (e.g., `0`, `42`, `1000`).
- **Float Literals (`FLOAT_LITERAL`):** Decimal point notation with digits on both or either side (e.g., `3.14`, `0.5`, `5.0`).
- **Char Literals (`CHAR_LITERAL`):** Single characters enclosed in single quotes `'c'`. Supports escape sequences `'\n'`, `'\t'`, `'\0'`, `'\\'`, and `\'`.
- **Boolean Literals (`BOOL_LITERAL`):** `true` and `false`.

### 2.3 Identifiers
- Must start with an ASCII letter (`a-z`, `A-Z`) or underscore `_`.
- Subsequent characters may include letters, digits (`0-9`), and underscores `_`.
- Must not collide with reserved keywords.

---

## 3. Type System

### 3.1 Primitive Types
- `int`: Signed integer.
- `float`: Single/double precision floating point numeric.
- `bool`: Truth value (`true` or `false`).
- `char`: Single byte character.
- `void`: Represents absence of value. Cannot be used for variable declarations or parameter types.

### 3.2 Array Types
- Syntax: `T[N]` where `T` is a primitive type (excluding `void`) and `N` is an integer constant $> 0$.
- Fixed-size 1D arrays only.
- Arrays cannot be assigned as whole values (`arr1 = arr2;` is rejected). Elements must be accessed via subscript notation `arr[i]`.

### 3.3 Function Types
- Expressed as `ReturnType(ParamType1, ParamType2, ...)`.
- Functions are first-class symbols in the symbol table, but not first-class values in expressions (cannot be assigned to variables).

### 3.4 Type Compatibility & Widening Matrix

| Source Type | Target Type | Compatible? | Notes |
|:---|:---|:---:|:---|
| `int` | `int` | **Yes** | Exact match |
| `int` | `float` | **Yes** | Implicit numeric promotion (widening) |
| `float` | `float` | **Yes** | Exact match |
| `float` | `int` | **No** | Implicit narrowing conversion disallowed |
| `bool` | `bool` | **Yes** | Exact match |
| `char` | `char` | **Yes** | Exact match |
| `T[N]` | `T[N]` | **No** | Whole array assignment disallowed |
| `T` | `void` | **No** | `void` cannot accept values |

---

## 4. Semantic Rules & Constraints

### 4.1 Declarations
1. **No Duplicate Declarations:** An identifier cannot be declared more than once in the same scope.
2. **No Void Variables or Parameters:** Declaring a variable or parameter of type `void` triggers an immediate error.
3. **Forward Reference Support:** Top-level functions may be referenced before their syntactic definition. Functions can be mutually recursive.

### 4.2 Scoping & Lifetimes
1. **Lexical Scoping:** Variables declared within an inner block `{ ... }` shadow outer-scope variables with the same identifier.
2. **Scope Exit:** Variables declared within an inner block are removed from scope visibility when that block terminates.
3. **Lookup Resolution:** Identifier resolution begins at the innermost current scope and walks parent scopes until the global scope. If not found, an `Undeclared identifier` error is raised.

### 4.3 Assignment Statements
1. **L-Value Requirement:** The LHS of an assignment must be a modifiable variable or an array subscript expression (`arr[idx]`). Assigning to a function name or an array variable directly is illegal.
2. **Type Match:** The RHS type must be assignable to the LHS type (allowing `int` $\rightarrow$ `float`).
3. **Array Subscripting:** Subscript assignments `arr[idx] = expr` require that:
   - `arr` resolves to an array type `T[N]`.
   - `idx` evaluates strictly to type `int`.
   - `expr` is assignable to element type `T`.

### 4.4 Expressions & Operator Semantics
1. **Arithmetic (`+`, `-`, `*`, `/`):**
   - Both operands must be numeric (`int` or `float`).
   - If either operand is `float`, the result is `float`. Otherwise `int`.
2. **Modulo (`%`):**
   - Both operands must evaluate to `int`. Result is `int`.
3. **Relational (`<`, `<=`, `>`, `>=`):**
   - Both operands must be numeric (`int` or `float`).
   - Result is `bool`.
4. **Equality (`==`, `!=`):**
   - Operands must be compatible (either both numeric, both `bool`, or both `char`).
   - Result is `bool`.
5. **Logical (`&&`, `||`, `!`):**
   - All operands must evaluate strictly to `bool`. Result is `bool`.
6. **Unary Plus/Minus (`+`, `-`):**
   - Operand must be numeric (`int` or `float`). Result matches operand type.

### 4.5 Control Flow Statements
1. **Conditionals (`if`, `while`):** Condition expressions must evaluate strictly to `bool`. Numeric or pointer values are not automatically truthy/falsy.
2. **Return Statements:**
   - Must appear within a function body.
   - For a `void` function, `return;` without an expression is required.
   - For a non-`void` function, `return <expr>;` is required and `<expr>` must be assignable to the function's return type.

### 4.6 Function Invocations
1. Callee must resolve to an identifier declared as a function.
2. The number of arguments provided must match the declared parameter count.
3. Each argument expression must be assignable to the corresponding parameter type.
