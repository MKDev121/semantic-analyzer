# Demonstration Guide

Run these commands from the project root in Bash:

```bash
# 1. Build the unified semantic analyzer
./scripts/build.sh

# 2. Run the 40-program automated evaluation suite
./scripts/test.sh
```

The executable is generated at `build/semantic_final.exe`.

## Core & Extended Semantic Analysis Demonstrations

```bash
# Full pipeline inspection (tokens, untyped AST, typed AST, scoped symbol table)
./build/semantic_final.exe --all ./tests/semantic/test3_valid_functions.txt
./build/semantic_final.exe --dump-symbols ./tests/semantic/test2_valid_scopes.txt

# Forward calls, mutual recursion, and return-path analysis
./build/semantic_final.exe ./tests/test1_valid_forward_and_mutual_recursion.txt
./build/semantic_final.exe ./tests/test3_error_missing_return.txt
./build/semantic_final.exe ./tests/test4_error_array_types.txt
```

## Flow-Aware Definite-Assignment & Shadowing Trace Demonstrations

```bash
# Valid multi-branch definite assignment (assigned in both then and else)
./build/semantic_final.exe ./evaluation/test_corpus/catB_01_valid_ifelse_both_assigned.txt

# Error: Branch-conditional uninitialized read (if without else)
./build/semantic_final.exe ./evaluation/test_corpus/catB_05_error_if_without_else.txt

# Error: Loop-conditional uninitialized read (while loop may execute zero times)
./build/semantic_final.exe ./evaluation/test_corpus/catC_03_error_while_only_init.txt

# Error: Lexical shadowing scope trace on type mismatch and uninitialized read
./build/semantic_final.exe ./evaluation/test_corpus/catD_06_error_shadowed_type_mismatch_assign.txt
./build/semantic_final.exe ./evaluation/test_corpus/catD_04_error_shadowed_inner_uninit.txt

# Error: Cascading error suppression (undeclared identifier poisoning & deduplication)
./build/semantic_final.exe ./evaluation/test_corpus/catE_04_error_cascade_undeclared_reuse.txt
./build/semantic_final.exe ./evaluation/test_corpus/catE_05_error_cascade_uninit_multiple_reads.txt
```

## Phase Boundary Demonstrations

```bash
printf 'int main() { bool flag = true & false; return 0; }\n' | ./build/semantic_final.exe
printf 'int main() { int value = ; return 0; }\n' | ./build/semantic_final.exe
printf 'int main() { bool flag = 7; return 0; }\n' | ./build/semantic_final.exe
```

These produce lexical, syntax, and semantic errors respectively. Lexical and syntax failures halt the pipeline before semantic analysis.
