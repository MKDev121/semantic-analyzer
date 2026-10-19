# Language Specification

This is a statically typed, C-like language analyzed by `build/semantic_final.exe` (`src/`). The compiler front-end performs lexical validation, tokenization, recursive-descent parsing, scoped symbol resolution, bidirectional type checking, return-path checking, and flow-aware definite-assignment analysis.

## Grammar

`*` means zero or more, `?` means optional, and `|` separates alternatives. Literal punctuation is in quotes. Whitespace and comments separate tokens.

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

An identifier starts with an ASCII letter or underscore, followed by ASCII letters, digits, or underscores; keywords cannot be identifier names. Integers are decimal digits. Decimal literals require digits on both sides of the dot, such as `3.14`.

Character literals contain one ASCII character or one of `\n`, `\r`, `\t`, `\0`, `\\`, and `\'`. Empty, multi-character, unsupported-escape, and unfinished character literals are lexical errors. Comments use `//` to the end of a line or `/* ... */`; block comments must close. Contiguous `++` and `--` are rejected as unsupported increment/decrement operators.

## Declaration, Scope, and Definite-Assignment Rules

- Only variables and complete function definitions occur at the top level.
- Functions and globals are registered in a first pass, permitting forward references and mutual recursion.
- A variable or parameter cannot have type `void`; arrays cannot have `void` elements.
- Function parameters start as `DEFINITELY_INITIALIZED`.
- Local scalar variables declared without an initializer (`int x;`) start as `UNINITIALIZED` and must be definitely assigned along all incoming control-flow paths before being read.
- Conditional `if-else` statements merge to `DEFINITELY_INITIALIZED` when both branches assign the variable (or one branch returns early via `return` and the other assigns it). Partial branches (`if` without `else`, or one-sided `if-else`) and assignments inside `while` loops (which may execute zero times) merge to `MAYBE_INITIALIZED` with branch/loop provenance.
- Duplicate names in one scope are errors; an inner scope may shadow an outer name. When a shadowed variable is involved in a semantic error, the diagnostic includes a lexical scope trace citing both the inner and shadowed outer declarations.

## Type Rules

| Operation | Accepted types | Result |
| --- | --- | --- |
| `+`, `-`, `*`, `/` | `int` or `float` operands | `float` if either operand is `float`, otherwise `int` |
| `%` | Two `int` operands | `int` |
| `<`, `<=`, `>`, `>=` | Numeric operands | `bool` |
| `==`, `!=` | Numeric pair, Boolean pair, or character pair | `bool` |
| `&&`, `||` | Two `bool` operands | `bool` |
| Unary `+`, `-` | Numeric operand | Operand type |
| Unary `!` | `bool` operand | `bool` |

Assignments, initializers, function arguments, and return values require exact primitive type matches or widening from `int` to `float`. Conditions in `if` and `while` must be `bool`. Non-void functions must guarantee a return across all structural paths. Fixed-size 1D arrays require `int` subscript expressions.
