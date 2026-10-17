# Preserved final implementation

This folder keeps the existing advanced analyzer separate from the active 75%
source in `../src/`. `src/main_3.cpp` builds a final demo that always includes
the extended features. It shares the original lexer and token definitions
from `../src/`, plus the common symbol table from `../shared/`. The executable
is generated in `../build/`.

## Deferred-feature map

| Feature reserved for final review | Preserved implementation |
| --- | --- |
| One-dimensional arrays and element checks | Array types in `Type`, array AST nodes, bracket parsing in `Parser`, and array access/assignment checks in `SemanticAnalyzer` |
| Forward calls and mutual recursion | Two-pass top-level registration in `SemanticAnalyzer` |
| Missing-return path analysis | `guaranteesReturn` and function-body checks in `SemanticAnalyzer.cpp` |
| Typed AST display | Type-aware AST printer and `main_3.cpp` output options |
| Possible correction guidance | Correction text in `ErrorReporter`, parser, lexical validation, and semantic checks |

The extended inputs are preserved in `tests/semantic/`. No new data-flow
analysis was added during this separation.

## Build and demonstrate

Run from the project root (`semantic-analyzer`) in PowerShell:

```powershell
.\scripts\build.ps1 -Milestone 100
.\build\semantic_final.exe --all .\later\tests\semantic\test3_valid_functions.txt
.\build\semantic_final.exe .\later\tests\semantic\test6_error_type_mismatch.txt
.\build\semantic_final.exe .\later\tests\test1_valid_forward_and_mutual_recursion.txt
.\build\semantic_final.exe .\later\tests\test3_error_missing_return.txt
```

The final driver accepts `--dump-tokens`, `--dump-ast`, `--dump-typed-ast`,
`--dump-symbols`, and `--all` (all four views). File input and standard input
work the same way as the core demo. Exit codes are `0` for no detected errors,
`1` for source/usage/input errors, and `2` for an unexpected analyzer failure.

`../scripts/test.ps1` builds and verifies all milestones. The final fixtures
include the ten saved examples in `tests/semantic/` and four focused advanced
examples in `tests/`. See [the repository map](../docs/REPOSITORY_MAP.md) for
the shared-source include rules and the intentional stage-specific files.

## Further work

Uninitialized-variable detection, unused-variable warnings, unreachable-code
checks, and constant array-index bounds checks are still unimplemented.
Return-path checks are conservative and do not prove loop execution or
constant-condition branches. Final report and presentation preparation remain
separate work.
