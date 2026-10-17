# Repository map

Run build and test commands through `scripts/` from the project root. The
scripts locate the repository independently of the caller's working directory.
Generated output belongs in `build/`; documentation and presentations belong
in `docs/` and remain part of the repository.

## Source ownership

| Location | Purpose |
| --- | --- |
| `src/main_1.cpp`, `src/Lexer.cpp`, `src/Lexer.hpp`, `src/Token.hpp` | Original 25% source; lexer/token files also serve both semantic stages |
| Other files in `src/` | Core 75% parser, AST, types, semantic checks, diagnostics, and driver |
| `shared/SymbolTable.hpp`, `shared/SymbolTable.cpp` | One common scope/symbol implementation, compiled separately for each semantic stage |
| `later/src/` | Preserved advanced Type/AST/parser/analyzer/diagnostic variants and `main_3.cpp` |
| `tests/review_1_tests/` | Three 25% demonstration inputs |
| `tests/review_2_tests/` | Eight core 75% demonstration inputs |
| `later/tests/semantic/` | Ten saved extended semantic inputs |
| `later/tests/` | Four focused array/return-path/forward-call examples |
| `scripts/build.ps1`, `scripts/test.ps1` | Repeatable build and verification entry points |
| `docs/` | Language rules, architecture, demonstration commands, and presentations |
| `build/` | Generated executables and temporary output; excluded from Git |

## Intentional stage variants

The core and final implementations share language concepts but have different
source requirements:

| Component | Core 75% | Preserved final extension |
| --- | --- | --- |
| Type and AST | Scalar/function types, basic AST, scalar assignments | Array types, array access/assignment nodes, typed AST display |
| Parser | Scalar declarations and expressions | Fixed-size array declarations and subscripts |
| SemanticAnalyzer | Source-order declarations and explicit return-type checks | Two-pass registration, forward calls/mutual recursion, array checks, missing-return paths |
| ErrorReporter and LexicalValidation | Locations, carets, and reasons | Additional correction guidance and extended diagnostic representation |
| SymbolTable | Common scoped lookup and hierarchy | Same common scoped lookup and hierarchy |

The SymbolTable pair was identical and is now shared. The other variants
remain separate so the 75% source itself presents the reduced scope.

## Include and build boundaries

`shared/SymbolTable.hpp` includes `Type.hpp`, and the two Type definitions have
different fields and enum values. The build script compiles the shared source
against the appropriate stage headers:

```text
75%:   -I src -I shared
Final: -I later/src -I shared -I src
```

The final build uses `src/` for the unchanged lexer/token files. It uses
`later/src/` for its AST, Type, parser, analyzer, and diagnostics. Do not combine
both variants or glob all source files into one executable; each target has
one driver and an explicit source list.

## Canonical outputs

```text
build/review_1.exe       25% lexer demo
build/semantic_demo.exe  75% core semantic demo
build/semantic_final.exe Preserved final extension demo
```

Use `scripts/build.ps1 -Milestone All`, `25`, `75`, or `100`. A custom output
folder is supported through `-OutputDirectory`; pass the same folder to
`scripts/test.ps1 -BuildDirectory`. The test script builds all stages by default
and supports `-SkipBuild` for existing outputs.

The final source preserves advanced work already implemented. Uninitialized
variables, unused variables, unreachable code, and constant array-index bounds
checks remain future work in both semantic stages.
