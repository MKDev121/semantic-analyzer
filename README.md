# Semantic Analyzer — 25% and 75% Demonstrations

A C++17 compiler front-end for a small, statically typed, C-like language.
The two milestones have separate entry points and executables. The analyzer
checks source code; it does not execute it or generate machine code.

## Separate milestones

| Milestone | Entry point | Executable | Demonstrates |
| --- | --- | --- | --- |
| 25% | `src/main_1.cpp` | `build/lexer_25.exe` | Tokens, line/column positions, and lexical error explanations |
| 75% | `src/main_2.cpp` | `build/semantic_75.exe` | Lexing, parsing, ASTs, scoped symbols, type checking, and explained diagnostics |

The existing `main_1.cpp`, `Lexer.cpp`, `Lexer.hpp`, and `Token.hpp` are
preserved unchanged. Both builds reuse that lexer. Additional lexical
validation is used only by the 75% driver. The saved components have been
integrated into `src/`, `tests/semantic/`, and `docs/`; redundant archive copies
have been removed.

## Build and run on Windows

Use PowerShell in the `semantic-analyzer` directory. A C++17-capable `g++`
must be available on PATH.

```powershell
.\build.ps1 -Milestone All

# Show the 25% lexer demonstration.
.\build\lexer_25.exe .\tests\test1_declarations.txt
.\build\lexer_25.exe .\tests\test2_operators.txt
.\build\lexer_25.exe .\tests\test3_unknown_character.txt

# Show the complete 75% pipeline.
.\build\semantic_75.exe --all .\tests\semantic\test3_valid_functions.txt

# Show explained semantic errors.
.\build\semantic_75.exe .\tests\semantic\test6_error_type_mismatch.txt
```

Use `-Milestone 25` or `-Milestone 75` to build just one demo. The build
script explicitly selects one main per executable; compiling `src/*.cpp`
together would include two main functions. Static linking avoids external
C++ runtime DLL conflicts. Rebuild after editing source files; changes to
input `.txt` files only require rerunning the executable.

## 75% capabilities

- Recursive-descent parsing with operator precedence and recovery from malformed statements.
- AST output before semantic analysis and typed AST output afterward.
- A persistent scope tree for globals, functions, parameters, and nested blocks.
- Undeclared-name and duplicate-declaration detection, including scope exit and shadowing.
- Primitive types `int`, `float`, `bool`, `char`, and `void` for function returns.
- Assignment and operator checks, implicit `int` to `float` widening, and strictly Boolean conditions.
- Two-pass function registration supporting forward calls and mutual recursion.
- Argument count/type validation and return-value validation.
- Conservative missing-return checks: explicit returns or both `if/else` branches must cover the function.
- Fixed-size one-dimensional arrays, integer subscripts, element type checks, and invalid assignment-target detection.
- Rejection of `void` variables/arrays, malformed character literals, and unfinished block comments.
- Source excerpts, carets, rule explanations, and correction guidance for lexical, syntax, and semantic errors.

Analysis stops after lexical or syntax failures. Multiple independent semantic
errors can be reported in one run; an error type suppresses follow-on type
errors caused by an earlier invalid expression.

## Output options

```text
--dump-tokens     Token categories, lexemes, and source positions
--dump-ast        AST before semantic analysis
--dump-typed-ast  AST annotated with inferred expression types
--dump-symbols    Hierarchical symbol table
--all            All four views
-h, --help       Usage
```

Input comes from one source file, or from standard input when the path is
omitted or `-` is supplied:

```powershell
'int main() { return 1 + 2 * 3; }' | .\build\semantic_75.exe --dump-typed-ast
```

Exit code `0` means no detected errors; `1` means source, usage, or file-input
errors; `2` means an unexpected analyzer failure. Empty or comments-only input
is an empty compilation unit and prints an explicit notice. A `main` function
is not required because files may contain library functions or global declarations.

## Verify both milestones

```powershell
.\test.ps1

# After building, run tests without rebuilding.
.\test.ps1 -SkipBuild
```

The suite contains 48 cases: the three existing 25% examples, ten saved
semantic fixtures, and 35 regression/CLI cases. Negative cases assert the
error phase, diagnostic count, reason, correction guidance, and expected
message. Syntax failures cannot accidentally satisfy a semantic test.
Malformed-input processes have a timeout so a parser regression cannot
hang the suite. The runner works with Windows PowerShell 5.1 and PowerShell 7.
It sends regression examples through stdin without creating scratch files.

## Files added for 75%

```text
src/main_2.cpp                  75% pipeline driver
src/LexicalValidation.hpp/.cpp  Extra validation without modifying the lexer
src/Parser.hpp/.cpp             Grammar and AST construction
src/AST.hpp/.cpp                AST nodes, visitor interface, and output
src/Type.hpp/.cpp               Primitive, array, and function types
src/SymbolTable.hpp/.cpp        Nested scopes and symbol lookup
src/SemanticAnalyzer.hpp/.cpp   Declaration and type checks
src/ErrorReporter.hpp          Source-aware diagnostics
tests/semantic/                Ten saved semantic fixtures
build.ps1                      Separate executable builds
test.ps1                       Diagnostic-aware regression runner
docs/                          Language, architecture, and demonstration guide
```

## Scope and remaining work

This is a small language rather than a complete C/C++ implementation. Use
`f()` for a function without parameters, scalar named parameters, and one
variable per declaration. Strings, casts, pointers, structs, array parameters,
array initializers, multidimensional arrays, prototypes, `for`, `break`,
`continue`, `++`, `+=`, and chained assignments are outside this milestone.

Return analysis deliberately does not evaluate constant conditions or prove
that a loop always executes. Array bounds, uninitialized variables, unused
variables, and unreachable code are not checked. Numeric literals are assigned
a type without evaluating their value or checking overflow.

The final milestone can add deeper control-flow/data-flow checks, broader
regression coverage, and final report/presentation work.

See [the language specification](docs/LANGUAGE_SPEC.md),
[the architecture](docs/ARCHITECTURE.md), and
[the demonstration guide](docs/DEMO_GUIDE.md).
The original MIT license and attribution remain in `LICENSE`.
