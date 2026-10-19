# Repository Map

Run build and test commands through `scripts/` from the project root. The scripts locate the repository independently of the caller's working directory. Generated output belongs in `build/`.

## Directory Structure

| Location | Purpose |
| --- | --- |
| `src/Token.hpp`, `src/Lexer.hpp`, `src/Lexer.cpp`, `src/LexicalValidation.hpp`, `src/LexicalValidation.cpp` | Lexical scanner, source position tracking, and pre-parse lexical validation |
| `src/Type.hpp`, `src/Type.cpp`, `src/AST.hpp`, `src/AST.cpp`, `src/Parser.hpp`, `src/Parser.cpp` | Type algebra, untyped/typed AST representation, and recursive-descent parser |
| `src/SymbolTable.hpp`, `src/SymbolTable.cpp` | Hierarchical scoped symbol table with `InitState` lattice and `lookupShadowed()` |
| `src/SemanticAnalyzer.hpp`, `src/SemanticAnalyzer.cpp`, `src/ErrorReporter.hpp` | Two-pass flow-aware semantic analyzer, scope-shadowing traces, cascading suppression, and diagnostic formatter |
| `src/main.cpp` | Unified command-line driver (`build/semantic_final.exe`) |
| `tests/semantic/` | Core semantic test cases (`test1_valid_basic.txt` – `test10_error_invalid_lvalue.txt`) |
| `tests/` | Additional array, return-path, and forward/mutual-recursion test cases |
| `evaluation/` | 40-program labeled benchmark corpus (`evaluation/test_corpus/`), evaluation harness (`evaluation/run_eval.py`), and generated reports (`RESULTS.md`, `results_summary.csv`) |
| `deliverables/` | Final project report, IEEE manuscript draft, and presentation slides |
| `scripts/build.sh`, `scripts/test.sh` | Repeatable build and verification entry points |
| `docs/` | Language specification, architecture, research basis, and demonstration guide |
| `build/` | Generated executable (`build/semantic_final.exe`) |
