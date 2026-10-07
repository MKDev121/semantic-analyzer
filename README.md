# Semantic Analyzer - Milestone 1 (approximately 25% scope)

This checkout is reduced to the lexical-analysis milestone of the cloned C++17
compiler front end. It reads a source file and displays tokens with line and
column numbers. The original project's MIT license and attribution remain in
`LICENSE`.

## Milestone scope

| Stage | Scope | Active status |
| --- | --- | --- |
| 1 | Token definitions, lexer, token-output CLI and sample tests | Available |
| 2 | Parser and abstract syntax tree | Deferred |
| 3 | Types, declarations and scoped symbol table | Deferred |
| 4 | Semantic checks and integrated diagnostics | Deferred |

The 25% label describes the first of four broad milestones, not a measured
percentage of development effort or a claim about authorship. Later stages may
require more work than the lexer.

The active lexer recognizes keywords, identifiers, integer/decimal/character
literals, operators and delimiters; skips whitespace and comments; and tracks
source positions. The CLI reports unknown characters with a source excerpt, a
caret marking the location, a reason and a possible correction, and returns a
nonzero exit code. Suggestions explain lexical rules; they do not automatically
modify source code or perform semantic analysis.
The inherited lexer still needs stricter validation of malformed character
literals and unterminated block comments in a future revision.

This milestone does **not** parse grammar, evaluate expressions, execute programs,
or check types, scopes, declarations or function calls. A zero lexical-error
count only describes tokenization; it does not prove the program is valid.

## Active structure

```text
src/
  Token.hpp       Token kinds and source locations
  Lexer.hpp       Lexer interface
  Lexer.cpp       Scanner implementation
  main.cpp        Token table and lexical-error output
tests/
  test1_declarations.txt
  test2_operators.txt
  test3_unknown_character.txt
Makefile          Optional g++ build via make
build.ps1         Windows build with the compiler runtime linked in
run_tests.ps1     Windows build, demo tests and diagnostic regression checks
README.md
LICENSE
.gitignore
later/            Local archive of deferred original files (ignored)
```

`later/` preserves the original parser, AST, type system, symbol table, semantic
analyzer, error reporter and driver, plus all ten original tests, documentation,
README, Makefile, contribution guide and Bash test runner. The active build uses
only `src/main.cpp` and `src/Lexer.cpp`.

## Build and run on Windows (PowerShell)

Open a terminal inside `semantic-analyzer`. A C++17-capable `g++` must be on PATH.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
.\lexer_demo.exe .\tests\test1_declarations.txt
.\lexer_demo.exe .\tests\test2_operators.txt
```

Each command prints a table of line, column, token category and lexeme, followed
by the token count and lexical-error count.

The build script works relative to its own directory and prints the compiler
and output paths. On Windows, it links the runtime into the executable so that
other programs' C++ runtime DLLs on PATH cannot affect this demo. To compile
manually with the same settings:

```powershell
g++ -std=c++17 -Wall -Wextra -O2 -static -Isrc src/main.cpp src/Lexer.cpp -o lexer_demo.exe
```

If input unexpectedly shows zero bytes, verify the exact file on disk:

```powershell
Get-Item .\tests\test1_declarations.txt | Select-Object FullName, Length
Get-Content .\tests\test1_declarations.txt
```

Rebuild with `build.ps1`, then rerun the executable. The reader checks the size
of the opened file and reads that many bytes directly. A failed or incomplete
read is an error, rather than an empty-input notice. The declarations example
must produce 20 tokens; its byte count can vary with line endings.

| Test | Expected tokens (excluding EOF) | Expected lexical errors | Exit code |
| --- | ---: | ---: | ---: |
| Declarations | 20 | 0 | 0 |
| Operators and comment skipping | 27 | 0 | 0 |
| Unknown character `@` | 7 | 1 | 1 |

For the error demonstration:

```powershell
.\lexer_demo.exe .\tests\test3_unknown_character.txt
```

Expected diagnostic:

```text
[ERROR] line 1, col 16: unrecognized character '@'
    int count = 10 @ 2;
                   ^
Reason: '@' is not a supported token in this language.
Possible correction: Remove '@' if accidental, or replace it with the intended supported token.
```

A lone `&` suggests `&&` if logical AND was intended; a lone `|` suggests `||`
if logical OR was intended. These are conditional suggestions, not automatic fixes.

The CLI also prints the input filename and bytes read. An empty file produces an
explicit notice asking you to save source code and rerun. If only whitespace or
comments were skipped, a separate notice explains why there are zero tokens.

Build and check all three cases, plus empty input and multiline diagnostics
with tabs and Windows line endings:

```powershell
powershell -ExecutionPolicy Bypass -File .\run_tests.ps1
```

For Linux/macOS, run `make`, then
`./lexer_demo tests/test1_declarations.txt` and
`./lexer_demo tests/test2_operators.txt`.

## Git behavior

`/later/` is ignored and is not included in new commits by default. The original
tracked paths moved there appear as deletions until you commit the reduced
project. Git history still contains the original full implementation; ignoring
the archive does not erase that history. Keep a local backup of `later/` if you
need its files outside this checkout. No commit or push is required to run the
milestone.
