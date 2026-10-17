# Semantic Analyzer

A C++17 compiler front-end for a small, statically typed, C-like language.
It analyzes source code without executing it or generating machine code.

## Milestones

| Review | Source | Executable | Scope |
| --- | --- | --- | --- |
| 25% | `src/main_1.cpp` | `build/review_1.exe` | Tokens, source positions, and lexical errors |
| 75% | `src/main_2.cpp` | `build/semantic_demo.exe` | Parser, basic AST, scoped symbols, scalar type checks, and basic function checks |
| Final extension | `later/src/main_3.cpp` | `build/semantic_final.exe` | Preserved arrays, forward calls, return-path checks, typed AST, and correction guidance |

The 75% implementation contains the core analyzer. Existing advanced features
are kept in `later/src/` for the final review. The original 25% lexer and token
files remain shared and unchanged. Identical symbol-table code lives in
`shared/`; each semantic build compiles it against its own type definitions.

## Build, test, and demonstrate

Use PowerShell in the project root with a C++17-capable `g++` on PATH:

```powershell
.\scripts\build.ps1 -Milestone All
.\scripts\test.ps1 -SkipBuild

# 25%: token recognition.
.\build\review_1.exe .\tests\review_1_tests\test1_declarations.txt

# 75%: tokens, basic AST, symbols, and scalar/function checks.
.\build\semantic_demo.exe --all .\tests\review_2_tests\test3_valid_functions.txt
.\build\semantic_demo.exe .\tests\review_2_tests\test6_error_type_mismatch.txt

# Final extension: the preserved advanced implementation.
.\build\semantic_final.exe --all .\later\tests\semantic\test3_valid_functions.txt
```

Use `-Milestone 25`, `75`, or `100` to build one executable. By default,
`scripts/test.ps1` builds all milestones before checking them; use `-SkipBuild`
after a build. Build output defaults to `build/`; custom locations use the
build script's `-OutputDirectory` and the test script's `-BuildDirectory`.
The scripts resolve project paths independently of the caller's working
directory. Rebuild after editing source; input-file changes need only a rerun.

## 75% scope and output

The core analyzer detects undeclared names and duplicate declarations,
maintains nested scopes with shadowing, checks scalar assignments/operators,
and checks function argument counts/types and explicit return values.
It supports `int`, `float`, `bool`, `char`, and `void` function returns,
with implicit `int` to `float` widening and Boolean `if`/`while` conditions.
Globals and functions become visible in source order; direct self-recursion
and calls to earlier definitions are allowed.

```text
--dump-tokens   Token categories, lexemes, and positions
--dump-ast      Basic AST before semantic analysis
--dump-symbols  Hierarchical symbol table
--all           All three views
-h, --help      Usage
```

The final executable additionally accepts `--dump-typed-ast`, and its `--all`
shows four views. Arrays and later function definitions exceed the 75% scope.
Both analyzers show error positions, source excerpts, carets, and reasons;
the final implementation also prints possible corrections.

Input is one source file, or standard input when the path is omitted or `-`
is supplied. Exit codes are `0` for no detected errors, `1` for source/usage/
input errors, and `2` for an unexpected analyzer failure. Lexical and syntax
failures stop later phases. Empty units and files without `main` are allowed.

## Repository layout

```text
src/                      Original 25% lexer and core 75% implementation
shared/                   Common symbol table, compiled separately per stage
later/src/                Preserved advanced final implementation
tests/review_1_tests/      25% inputs
tests/review_2_tests/      75% inputs
later/tests/              Extended final inputs
scripts/                  Build and verification entry points
docs/                     Language, architecture, demo guide, and presentations
build/                    Generated executables and temporary output
```

See the [repository map](docs/REPOSITORY_MAP.md),
[language specification](docs/LANGUAGE_SPEC.md),
[architecture](docs/ARCHITECTURE.md), and [demo guide](docs/DEMO_GUIDE.md).
The [final extension map](later/README.md) identifies the deferred features.

## Remaining work

Uninitialized-variable detection, unused-variable warnings, unreachable-code
checks, and constant array-index bounds checks remain unimplemented.
The final extension preserves existing work; it does not imply that those
further analyses or final report/presentation preparation are complete.

This teaching language also excludes strings, casts, pointers, structs,
array parameters/initializers, multidimensional arrays, prototypes, `for`,
`break`, `continue`, `++`, `+=`, and chained assignments. Numeric literals
are typed without evaluation or overflow checks. Return-path analysis is
conservative and does not prove loop execution or constant-condition branches.

The original MIT license and attribution remain in `LICENSE`.
