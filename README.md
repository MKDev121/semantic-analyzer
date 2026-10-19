# Explainable Semantic Analyzer with Scope and Definite-Assignment Checking

A C++17 compiler front-end for a statically typed, C-like language featuring lexical scoping, shadowing traces, bidirectional type checking, and **flow-aware definite-assignment (initialization) analysis** with explainable diagnostics.

## Overview

| Component | Source | Executable | Features |
| --- | --- | --- | --- |
| **Explainable Semantic Analyzer** | `src/` (`src/main.cpp`) | `build/semantic_final.exe` | **Flow-aware definite assignment (`DEFINITELY_INITIALIZED`, `MAYBE_INITIALIZED`, `UNINITIALIZED`)**, branch/loop initialization merging, lexical shadowing scope traces, cascading error suppression, 1D arrays, forward/mutual recursion, return-path checks, typed AST, and correction guidance |

## Build, Test, and Run the Evaluation Benchmark

### Linux / macOS (Bash)
```bash
# 1. Build the semantic analyzer
chmod +x scripts/build.sh scripts/test.sh
./scripts/build.sh

# 2. Run the 40-program automated evaluation benchmark suite
python3 evaluation/run_eval.py

# 3. Run individual demonstrations
./build/semantic_final.exe --all ./tests/semantic/test3_valid_functions.txt
./build/semantic_final.exe ./evaluation/test_corpus/catB_05_error_if_without_else.txt
./build/semantic_final.exe ./evaluation/test_corpus/catC_03_error_while_only_init.txt
./build/semantic_final.exe ./evaluation/test_corpus/catD_06_error_shadowed_type_mismatch_assign.txt
```

### Direct `g++` Compilation Command
```bash
mkdir -p build
g++ -std=c++17 -Wall -Wextra -I src \
    src/main.cpp src/Lexer.cpp src/LexicalValidation.cpp src/Parser.cpp \
    src/AST.cpp src/Type.cpp src/SemanticAnalyzer.cpp src/SymbolTable.cpp \
    -o build/semantic_final.exe
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

## Key Features (`src/`)

- **Flow-Aware Definite-Assignment Analysis**:
  - Tracks per-symbol initialization states across a 4-point lattice: `DEFINITELY_INITIALIZED`, `MAYBE_INITIALIZED`, `UNINITIALIZED`, and $\top$ (early return).
  - Clones and merges initialization environments across `if` without `else`, `if-else` branches, early-return paths (`guaranteesReturn`), and `while` loops (accounting for zero-trip loop execution).
  - Detects self-referential uninitialized reads (`int x = x + 1;` and `x = x + 10;`).
- **Explainable Diagnostics & Scope Shadowing Traces**:
  - Cites exact declaration lines, use lines, violated semantic rules, control-flow branch/loop provenance, and actionable repair hints (`Possible correction:`).
  - Automatically includes a lexical scope trace (`Scope trace: 'x' resolves to the <type> declared on line <inner_line> (shadowing the <type> declared on line <outer_line>).`) when a shadowed identifier is involved in a type or initialization error.
- **Cascading Error Suppression**:
  - Suppresses secondary diagnostic cascades via `TYPE_ERROR` expression propagation, scope-level undeclared identifier poisoning, basic-block uninitialized read deduplication (`reportedUninitRead`), and post-error initialization promotion.
- **Empirical Evaluation Suite (`evaluation/`)**:
  - 40 labeled benchmark programs across 5 categories (`evaluation/test_corpus/`).
  - Automated evaluation harness (`evaluation/run_eval.py`) generating `evaluation/RESULTS.md` and `evaluation/results_summary.csv`.
  - Achieves **100.0% Recall (25/25 TP)**, **100.0% Precision (0 FP)**, **0 Cascading Diagnostics**, and **100.0% Explanation Completeness**.

## Repository Layout

```text
src/                      Unified C++17 source code (lexer, parser, AST, type system, symbol table, flow-aware semantic analyzer, and CLI driver)
tests/                    Semantic test inputs (tests/semantic/ and extended control-flow/array tests)
evaluation/               40-program benchmark corpus (test_corpus/), run_eval.py, and RESULTS.md
deliverables/             Final Project Report, IEEE Manuscript Draft, and Final Presentation Slides
scripts/                  Build and test automation scripts (build.sh, test.sh)
docs/                     Language specification, architecture, research basis, and demo guide
build/                    Compiled executable (semantic_final.exe)
```

## Documentation & Final Deliverables

- [Technical & Algorithmic Novelty (`docs/NOVELTY.md`)](docs/NOVELTY.md)
- [Language Specification (`docs/LANGUAGE_SPEC.md`)](docs/LANGUAGE_SPEC.md)
- [System Architecture (`docs/ARCHITECTURE.md`)](docs/ARCHITECTURE.md)
- [Demonstration Guide (`docs/DEMO_GUIDE.md`)](docs/DEMO_GUIDE.md)
- [Final Project Report (`deliverables/Final_Project_Report.md`)](deliverables/Final_Project_Report.md)
- [IEEE Research Manuscript Draft (`deliverables/Manuscript_Draft_IEEE.md`)](deliverables/Manuscript_Draft_IEEE.md)
- [Final Presentation Slides Outline (`deliverables/Final_Presentation_Slides.md`)](deliverables/Final_Presentation_Slides.md)
- [Empirical Benchmark Results (`evaluation/RESULTS.md`)](evaluation/RESULTS.md)

The original MIT license and attribution remain in `LICENSE`.
