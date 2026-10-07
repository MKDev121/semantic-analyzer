# Semantic Analyzer - 25% Presentation Milestone

The active C++17 version demonstrates lexical analysis: source-file input,
token recognition, source positions and explanatory unknown-character errors.
The full project's later stages are preserved locally in `later/` for subsequent
presentations. The 25% label describes the presentation scope.

## Active structure

```text
src/
  Token.hpp                       Token categories and source locations
  Lexer.hpp                       Scanner interface
  Lexer.cpp                       Token recognition and comment skipping
  main.cpp                        File input, token table and diagnostics
tests/
  test1_declarations.txt           Valid declarations
  test2_operators.txt              Operators and comment skipping
  test3_unknown_character.txt      Invalid character example
README.md
LICENSE
.gitignore
later/                            Future stages; ignored by Git
build/                            Generated verification files; ignored
lexer_demo.exe                    Generated executable; ignored
```

Only the four files in `src/` are needed to compile this milestone. `later/`
contains the parser, AST, types, symbol table, semantic analyzer, original driver,
error reporter, full-project documentation, original tests, Makefile and Bash
test runner. These are excluded from the active build. The original license and
attribution are preserved in `LICENSE`.

## Compile and run (PowerShell)

Open a terminal in `semantic-analyzer`, then compile:

```powershell
g++ -std=c++17 -static src/main.cpp src/Lexer.cpp -o lexer_demo.exe
```

The static build avoids external C++ runtime DLL conflicts. Add `-Wall -Wextra`
to display additional compiler warnings if desired. No Makefile or build script
is needed for this command.

Run the sample inputs:

```powershell
.\lexer_demo.exe .\tests\test1_declarations.txt
.\lexer_demo.exe .\tests\test2_operators.txt
.\lexer_demo.exe .\tests\test3_unknown_character.txt
```

| Input | Tokens excluding EOF | Lexical errors | Exit code |
| --- | ---: | ---: | ---: |
| Declarations | 20 | 0 | 0 |
| Operators and comments | 27 | 0 | 0 |
| Unknown character | 7 | 1 | 1 |

Recompile after editing C++ code. If only an input `.txt` changes, save it and
rerun the executable.

## Output

The program prints the input path and bytes read, followed by a table of line,
column, token category and lexeme. EOF marks the end and is excluded from the
token count. Empty input has an explicit notice; incomplete file reads fail.

For `int count = 10 @ 2;`, the diagnostic is:

```text
[ERROR] line 1, col 16: unrecognized character '@'
    int count = 10 @ 2;
                   ^
Reason: '@' is not a supported token in this language.
Possible correction: Remove '@' if accidental, or replace it with the intended supported token.
```

A lone `&` suggests `&&` if logical AND was intended; a lone `|` suggests `||`
if logical OR was intended. Suggestions do not automatically edit the input.

## Current scope and next stages

The active lexer recognizes keywords, identifiers, integer/decimal/character
literals, operators and delimiters. It skips whitespace and comments and tracks
line and column positions. It does not parse grammar, evaluate expressions,
execute programs, or check declarations, scopes, function calls or types.
Zero lexical errors does not establish that a program is semantically valid.
Stricter malformed-character-literal and unterminated-block-comment validation
remains future work.

The 50% presentation can introduce parsing, ASTs and symbol/type handling; the
100% presentation can demonstrate integrated semantic checks and diagnostics.

## Git behavior

`/later/`, build files and the executable are ignored. Files moved out of tracked
locations appear as deletions in a future commit; the ignored local archive is
not uploaded. Existing Git history retains previous versions. Keep a local
backup of the archive if you need it independently of this checkout.
