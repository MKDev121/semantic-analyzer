# Explainable Semantic Analyzer with Scope and Definite-Assignment Checking

A C++17 compiler front-end for a statically typed, C-like language featuring lexical scoping, shadowing traces, bidirectional type checking, and **flow-aware definite-assignment (initialization) analysis** with explainable diagnostics.

## Milestones & Completion Status

| Review | Source | Executable | Scope & Status |
| --- | --- | --- | --- |
| **25%** | `src/main_1.cpp` | `build/review_1.exe` | Tokens, source positions, and lexical errors (**Completed**) |
| **75% (Baseline)** | `src/main_2.cpp` | `build/semantic_demo.exe` | Parser, basic AST, scoped symbols, scalar type checks, and basic function checks (**Completed**) |
| **100% (Final Enhanced)** | `later/src/main_3.cpp` | `build/semantic_final.exe` | **Flow-aware definite assignment (`DEFINITELY_INITIALIZED`, `MAYBE_INITIALIZED`, `UNINITIALIZED`)**, branch/loop initialization merging, lexical shadowing scope traces, cascading error suppression, 1D arrays, forward/mutual recursion, return-path checks, typed AST, and correction guidance (**Completed — 100%**) |

The 75% implementation (`src/`) serves as the baseline analyzer. The 100% final implementation lives in `later/src/` and shares the original 25% lexer/token files (`src/Lexer.cpp`, `src/Token.hpp`) and the scoped symbol table (`shared/SymbolTable.hpp`, `shared/SymbolTable.cpp`).

## Build, Test, and Run the Evaluation Benchmark

### Linux / macOS (Bash)
```bash
# 1. Build all three milestones (25%, 75% baseline, 100% enhanced)
chmod +x scripts/build.sh scripts/test.sh
./scripts/build.sh

# 2. Run the 40-program automated evaluation benchmark suite
python3 evaluation/run_eval.py

# 3. Run individual demonstrations
# 25%: token recognition
./build/review_1.exe ./tests/review_1_tests/test1_declarations.txt

# 75% baseline: core semantic checks
./build/semantic_demo.exe --all ./tests/review_2_tests/test3_valid_functions.txt
./build/semantic_demo.exe ./tests/review_2_tests/test6_error_type_mismatch.txt

# 100% final enhanced: flow-aware definite assignment & explainable diagnostics
./build/semantic_final.exe --all ./later/tests/semantic/test3_valid_functions.txt
./build/semantic_final.exe ./evaluation/test_corpus/catB_05_error_if_without_else.txt
./build/semantic_final.exe ./evaluation/test_corpus/catC_03_error_while_only_init.txt
./build/semantic_final.exe ./evaluation/test_corpus/catD_06_error_shadowed_type_mismatch_assign.txt
```

### Direct `g++` Compilation Commands
```bash
mkdir -p build
g++ -std=c++17 -Wall -Wextra -I src src/main_1.cpp src/Lexer.cpp -o build/review_1.exe
g++ -std=c++17 -Wall -Wextra -I src -I shared src/main_2.cpp src/Lexer.cpp src/LexicalValidation.cpp src/Parser.cpp src/AST.cpp src/Type.cpp src/SemanticAnalyzer.cpp shared/SymbolTable.cpp -o build/semantic_demo.exe
g++ -std=c++17 -Wall -Wextra -I later/src -I shared -I src later/src/main_3.cpp src/Lexer.cpp later/src/LexicalValidation.cpp later/src/Parser.cpp later/src/AST.cpp later/src/Type.cpp later/src/SemanticAnalyzer.cpp shared/SymbolTable.cpp -o build/semantic_final.exe
```

## CLI Options (`semantic_final.exe`)

```text
--dump-tokens     Token categories, lexemes, and positions
--dump-ast        Basic AST before semantic analysis
--dump-typed-ast  AST annotated with synthesized expression types
--dump-symbols    Hierarchical scoped symbol table
--all             All four views
-h, --help        Usage
```

## Completed Features in the 100% Final Implementation (`later/src/`)

- **Flow-Aware Definite-Assignment Analysis (Completed)**:
  - Tracks per-symbol initialization states across a 4-point lattice: `DEFINITELY_INITIALIZED`, `MAYBE_INITIALIZED`, `UNINITIALIZED`, and $\top$ (early return).
  - Clones and merges initialization environments across `if` without `else`, `if-else` branches, early-return paths (`guaranteesReturn`), and `while` loops (accounting for zero-trip loop execution).
  - Detects self-referential uninitialized reads (`int x = x + 1;` and `x = x + 10;`).
- **Explainable Diagnostics & Scope Shadowing Traces (Completed)**:
  - Cites exact declaration lines, use lines, violated semantic rules, control-flow branch/loop provenance, and actionable repair hints (`Possible correction:`).
  - Automatically includes a lexical scope trace (`Scope trace: 'x' resolves to the <type> declared on line <inner_line> (shadowing the <type> declared on line <outer_line>).`) when a shadowed identifier is involved in a type or initialization error.
- **Cascading Error Suppression (Completed)**:
  - Suppresses secondary diagnostic cascades via `TYPE_ERROR` expression propagation, scope-level undeclared identifier poisoning, basic-block uninitialized read deduplication (`reportedUninitRead`), and post-error initialization promotion.
- **Empirical Evaluation Suite (`evaluation/`) (Completed)**:
  - 40 labeled benchmark programs across 5 categories (`evaluation/test_corpus/`).
  - Automated evaluation harness (`evaluation/run_eval.py`) generating `evaluation/RESULTS.md` and `evaluation/results_summary.csv`.
  - Achieved **100.0% Recall (25/25 TP vs. 8/25 in Baseline)**, **100.0% Precision (0 FP)**, **0 Cascading Diagnostics**, and **100.0% Explanation Completeness**.

## Repository Layout

```text
src/                      Original 25% lexer and core 75% baseline implementation
shared/                   Common scoped symbol table with InitState and shadowing lookup
later/src/                100% flow-aware explainable semantic analyzer and main_3.cpp
tests/review_1_tests/     25% lexical inputs
tests/review_2_tests/     75% baseline semantic inputs
later/tests/              Extended final semantic inputs
evaluation/               40-program benchmark corpus (test_corpus/), run_eval.py, and RESULTS.md
deliverables/             Final Project Report, IEEE Manuscript Draft, and Final Presentation Slides
scripts/                  Build and test automation scripts (build.sh, test.sh)
docs/                     Language specification, architecture, research basis, and demo guide
build/                    Compiled executables (review_1.exe, semantic_demo.exe, semantic_final.exe)
```

## Final Deliverables (`deliverables/`)

- [Final Project Report](deliverables/Final_Project_Report.md)
- [IEEE Research Manuscript Draft](deliverables/Manuscript_Draft_IEEE.md)
- [Final Presentation Slides Outline](deliverables/Final_Presentation_Slides.md)
- [Empirical Benchmark Results](evaluation/RESULTS.md)

The original MIT license and attribution remain in `LICENSE`.
