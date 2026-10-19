# System Architecture

## End-to-End Pipeline

```text
src/main.cpp: read source and select output views
  |
  +--> LexicalValidation: literals, comments, source character validation
  +--> Lexer: token stream with source positions
  +--> unknown-token diagnostics
  +--> Parser: recursive descent and AST construction
  +--> SemanticAnalyzer (Pass 1): top-level global & function signature registration
  +--> SemanticAnalyzer (Pass 2): lexical scoping, shadowing traces, bidirectional type checking,
  |                               flow-aware definite-assignment lattice, and return-path verification
  +--> token / initial AST / typed AST / scope views and diagnostic report
```

The pipeline stops after lexical or syntax errors so later passes never mistake an invalid earlier-stage program for a semantic failure. `scripts/build.sh` compiles all translation units in `src/` into `build/semantic_final.exe`.

## Parser and AST

The parser uses one function per precedence level. Binary operators are left-associative. Parenthesized expressions and unary operators recurse. AST nodes retain 1-indexed source positions (`line`, `col`) and use `std::unique_ptr` for child ownership. Expression nodes carry an `inferredType`, populated during semantic analysis.

On a syntax failure, a private parser exception unwinds the current production. Recovery at block or top-level boundaries seeks the next statement or declaration while tracking delimiters and guaranteeing progress.

## Semantic Analysis, Definite Assignment, and Symbols

Semantic analysis operates in two passes:
- **Pass 1 (`registerTopLevelDeclarations`)** registers all global variables and function signatures before visiting initializers and function bodies, supporting forward calls and mutual recursion.
- **Pass 2** validates variable initializers, function bodies, scalar and 1D array types, implicit `int` to `float` widening, function call arguments, boolean control-flow conditions, structural return paths (`guaranteesReturn`), and flow-aware definite assignment.

The scoped symbol table (`src/SymbolTable.hpp`, `src/SymbolTable.cpp`) is a persistent tree of `Scope` objects with parent pointers and `lookupShadowed()` support. Each symbol tracks its initialization state across the lattice `{UNINITIALIZED, MAYBE_INITIALIZED, DEFINITELY_INITIALIZED}` along with control-flow path provenance (`uninitReason`, `uninitBranchLine`).

`SemanticAnalyzer` snapshots and restores active initialization states (`captureInitEnv` / `restoreInitEnv`) across `if`, `if-else`, early-return branches, and `while` loops, merging states conservatively at join points.

## Diagnostics and Cascading Error Suppression

`ErrorReporter` (`src/ErrorReporter.hpp`) retains source lines and the current compiler phase. Every diagnostic prints the phase, line/column coordinates, source excerpt, column caret (`^`), detailed `Reason:` (citing declaration lines, branch/loop provenance, and lexical shadowing traces when applicable), and `Possible correction:` guidance.

Four layers prevent cascading secondary diagnostics:
1. `Type::getError()` expression propagation.
2. Scope-aware undeclared identifier poisoning (`poisonedIdentifiers`).
3. Basic-block uninitialized read deduplication (`reportedUninitRead`).
4. Post-error initialization promotion on ill-typed initializers and assignments.
