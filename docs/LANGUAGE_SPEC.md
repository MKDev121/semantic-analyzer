# Language specification for the 75% and final implementations

This is a small C-like language with static types. The executable analyzes a
compilation unit; it does not evaluate expressions or execute statements.
`build/semantic_demo.exe` covers the core 75% scope in `src/`.
`build/semantic_final.exe` uses the separate extended implementation, including
arrays, forward references, return-path checks, and correction guidance.

## Grammar

`*` means zero or more, `?` means optional, and `|` separates alternatives.
Literal punctuation is in quotes. Whitespace and comments separate tokens.
The grammar below describes the complete implemented language. Array
declarations and subscripts belong to `later/`; the core 75% parser rejects
array syntax.

```ebnf
Program       ::= Declaration*
Declaration   ::= Type Identifier "(" Parameters? ")" Block
                | VariableDecl
Type          ::= "int" | "float" | "bool" | "char" | "void"
Parameters    ::= Type Identifier ("," Type Identifier)*
VariableDecl  ::= Type Identifier ("=" Expression)? ";"
                | Type Identifier "[" IntegerLiteral "]" ";"
Block         ::= "{" Statement* "}"
Statement     ::= VariableDecl | Block
                | "if" "(" Expression ")" Statement ("else" Statement)?
                | "while" "(" Expression ")" Statement
                | "return" Expression? ";"
                | AssignTarget "=" Expression ";"
                | Expression ";"
AssignTarget  ::= Identifier | Identifier "[" Expression "]"
                | "(" AssignTarget ")"
Expression    ::= LogicalOr
LogicalOr     ::= LogicalAnd ("||" LogicalAnd)*
LogicalAnd    ::= Equality ("&&" Equality)*
Equality      ::= Relational (("==" | "!=") Relational)*
Relational    ::= Additive (("<" | "<=" | ">" | ">=") Additive)*
Additive      ::= Product (("+" | "-") Product)*
Product       ::= Unary (("*" | "/" | "%") Unary)*
Unary         ::= ("+" | "-" | "!") Unary | Primary
Primary       ::= IntegerLiteral | DecimalLiteral | CharacterLiteral
                | "true" | "false" | Identifier
                | Identifier "(" Arguments? ")"
                | Identifier "[" Expression "]"
                | "(" Expression ")"
Arguments     ::= Expression ("," Expression)*
```

An identifier starts with an ASCII letter or underscore, followed by ASCII
letters, digits, or underscores; keywords cannot be identifier names. Integers
are decimal digits. Decimal literals require digits on both sides of the dot,
such as `3.14`. Scientific notation and explicit casts are unsupported.

Character literals contain one ASCII character or one of `\n`, `\r`, `\t`,
`\0`, `\\`, and `\'`. Empty, multi-character, unsupported-escape, and unfinished
character literals are lexical errors. Comments use `//` to the end of a line
or `/* ... */`; block comments must close. Non-ASCII text is allowed in comments.
An optional UTF-8 BOM is accepted as file metadata by both semantic drivers.
Contiguous `++` and `--` are rejected as unsupported increment/decrement
operators. Separate unary operators such as `+ +value` remain valid.

## Declaration and scope rules

- Only variables and complete function definitions occur at the top level.
- Functions with no parameters use `()`. Each parameter has a scalar type and a name.
- A variable or parameter cannot have type `void`; arrays cannot have `void` elements.
- Each declaration introduces one variable. Array sizes are positive integer literals.
- In the 75% implementation, globals and function definitions become visible in source order. A function can call itself or a previously defined function.
- In the final implementation, functions and globals are registered first, permitting forward references and mutual recursion.
- A local variable is visible from its declaration onward in its containing scope.
- Function parameters and the outer function body share a scope. Nested braces create child scopes.
- Duplicate names in one scope are errors; an inner scope may shadow an outer name.
- Leaving a block makes its local declarations inaccessible from the parent scope.
- There is no required `main` function. Empty compilation units are allowed.

## Type rules

| Operation | Accepted types | Result |
| --- | --- | --- |
| `+`, `-`, `*`, `/` | `int` or `float` operands | `float` if either operand is `float`, otherwise `int` |
| `%` | Two `int` operands | `int` |
| `<`, `<=`, `>`, `>=` | Numeric operands | `bool` |
| `==`, `!=` | Numeric pair, Boolean pair, or character pair | `bool` |
| `&&`, `||` | Two `bool` operands | `bool` |
| Unary `+`, `-` | Numeric operand | Operand type |
| Unary `!` | `bool` operand | `bool` |

Assignments, initializers, function arguments, and return values require exact
primitive type matches or widening from `int` to `float`. Other implicit
conversions are rejected, including `float` to `int` and `int` to `bool`.
Conditions in `if` and `while` must be `bool`.

Calls must target a declared function and match its argument count and types.
`void` functions may use `return;` or fall through, but cannot return a value.
Explicit returns in non-void functions must supply a compatible value in both
implementations. The 75% analyzer does not check for a missing return on another
path. The final analyzer also requires a non-void function to have a structurally
guaranteed return: a direct return, a block containing a guaranteed return, or an
`if/else` whose two branches both guarantee a return. Loops alone do not
guarantee a return, even when their conditions are constants.

Arrays are implemented in `later/` and have one fixed dimension.
Subscripting requires an `int` expression.
Assignment targets must be variables or array elements. Assigning to a function
or to a whole array is rejected. No static bounds or initialization analysis is
performed. An array index may therefore type-check while exceeding its bounds.

## Diagnostics and phase boundaries

Lexical errors stop analysis before parsing. Syntax errors stop it before
semantic analysis. Semantic analysis reports independent errors together;
an internal error type prevents secondary type errors for already invalid
expressions. Each diagnostic includes the phase, line/column, source excerpt,
caret, and reason. The final implementation additionally shows possible
correction guidance and permits typed AST output. Suggestions do not edit
source code.

Uninitialized-variable, unused-variable, unreachable-code, and constant
array-index bounds checks are not implemented in either implementation.
