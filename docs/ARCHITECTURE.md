# Architecture of the milestone demos

## 25% entry point

`main_1.cpp` reads a source file, calls the original `Lexer`, prints the token
table, and explains unknown characters. Its source and the three original
lexer/token files are unchanged. Its build includes only `main_1.cpp` and
`Lexer.cpp`. `scripts/build.ps1 -Milestone 25` writes `build/review_1.exe`.

## 75% pipeline

```text
src/main_2.cpp: read source and select output views
  |
  +--> LexicalValidation: literals, comments, source character validation
  +--> original Lexer: token stream with source positions
  +--> unknown-token diagnostics
  +--> Parser: recursive descent and AST construction
  +--> SemanticAnalyzer: resolve declarations/names and check expression types
  +--> token / basic AST / scope views and diagnostic report
```

The pipeline stops after lexical or syntax errors. Later passes cannot mistake
an invalid earlier-stage program for a semantic test failure.
`scripts/build.ps1 -Milestone 75` writes `build/semantic_demo.exe`.

## Parser and AST

The parser uses one function for each precedence level. Binary operators are
left associative. Parenthesized expressions and unary operators recurse.
AST nodes retain source positions and use `unique_ptr` for child ownership.
Expression nodes carry an `inferredType`, populated during analysis.

On a syntax failure, a private parser exception unwinds the current production.
Recovery at block or top-level boundaries seeks the next statement/declaration
while tracking delimiters and guaranteeing progress. EOF is a safe sentinel,
and block recovery preserves its owning closing brace.

## Semantic analysis and symbols

The active 75% analysis visits top-level declarations in source order. A function's
signature is registered before its body, allowing direct self-recursion and
calls to previously defined functions. Later globals and function definitions
are not yet visible. Initializers, function bodies, scalar operators,
assignments, argument counts/types, conditions, and explicit return values
are checked.

The preserved final implementation in `later/src/` has its own driver,
`main_3.cpp`. Its pass 1 registers all global variables and function signatures
before pass 2 visits initializers and bodies, supporting forward calls and
mutual recursion. It always uses the extended analyzer.
`scripts/build.ps1 -Milestone 100` writes `build/semantic_final.exe`.

The shared symbol table is a tree of scopes with parent lookup. Function parameters
and top-level function statements share one scope; nested blocks add child
scopes. Completed scopes remain available for the printed hierarchy.

The core types cover primitives, functions, and an error sentinel. The final
implementation additionally contains fixed-array types, array AST nodes,
array parsing, and element checks. Only `int` to `float` widening is implicit.
Expressions that already contain a type error propagate the sentinel to avoid
duplicate secondary diagnostics.

In the final implementation, missing-return analysis examines statement
structure without evaluating runtime values. This conservative check does not
prove constant branches or loop termination. The 75% implementation checks
explicit return types without checking whether all paths return.
Uninitialized-use, unused-variable, unreachable-code, and array-bounds checks
remain future work in both implementations.

## Diagnostics and preservation

`ErrorReporter` retains the source lines and the current compiler phase. The
75% report prints source coordinates, excerpt, caret, and rule explanation.
The separate final reporter also stores and prints possible correction
guidance. The final AST printer additionally displays inferred expression
types, and the final driver's `--all` includes that fourth view.
The driver prints a phase-specific summary and returns a failure
status when errors exist. No automatic correction modifies the input.

The active `src/` contains the original 25% files and the reduced 75% source.
Advanced variants are preserved in `later/src/`, with extended fixtures in
`later/tests/`. The `tests/review_2_tests/` inputs exercise the core 75% scope.
AST, Type, Parser, SemanticAnalyzer, ErrorReporter, and LexicalValidation remain
stage-specific because their supported structures, grammar, or diagnostics
differ. Both semantic stages share the original lexer/token files and the
identical SymbolTable implementation from `shared/`.

Each build compiles `shared/SymbolTable.cpp` separately with its stage's
`Type.hpp`. The 75% include order is `-I src -I shared`; final is
`-I later/src -I shared -I src`. The shared directory contains no `Type.hpp`,
so its quoted include selects the intended variant through that order.
Mixing stage-specific Type/AST/diagnostic headers within an executable would
produce incompatible definitions. The build script lists each stage's sources
explicitly and links one driver per executable.

Generated binaries belong in `build/`; tracked scripts, tests, documentation,
and presentations stay outside it. See [the repository map](REPOSITORY_MAP.md)
for the full layout.
