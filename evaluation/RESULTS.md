# Empirical Evaluation Results: Explainable Semantic Analyzer

## 1. Executive Summary of Benchmark Suite
- **Total Labeled Test Programs**: 40 (`evaluation/test_corpus/`)
  - **Positive (Valid) Programs**: 15 (including deferred straight-line initialization, `if-else` both-branch initialization, early-return branches, pre-initialized loops, and multi-scope lexical shadowing)
  - **Negative (Faulty) Programs**: 25 (covering straight-line uninitialized reads, self-initialization, `if` without `else`, one-sided `if-else` branches, nested branch omissions, zero-trip `while` loop assignments, shadowed variable initialization/type errors, and cascading error chains)
- **Models Compared**:
  1. **Standard Type-Only Baseline**: Conventional semantic checking without flow-aware definite-assignment tracking, scope/shadowing traces, or declaration-linked explanations.
  2. **Explainable Flow-Aware Analyzer (`build/semantic_final.exe`)**: Unified semantic analyzer (`src/`) with lattice-based branch/loop merging (`DEFINITELY_INITIALIZED`, `MAYBE_INITIALIZED`, `UNINITIALIZED`), lexical shadowing traces, and poison-state cascading error suppression.

## 2. Aggregate Empirical Metrics

| Metric | Standard Type-Only Baseline | Explainable Analyzer (`semantic_final.exe`) | Absolute / Relative Improvement |
| :--- | :---: | :---: | :---: |
| **True Positives (TP)** *(Faulty programs correctly flagged, out of 25)* | 8 / 25 | **25 / 25** | **+17 (+68.0% recall)** |
| **False Negatives (FN)** *(Missed semantic/initialization errors)* | 17 | **0** | **-17 (100% elimination of missed errors)** |
| **True Negatives (TN)** *(Valid programs accepted, out of 15)* | 15 / 15 | **15 / 15** | Preserved 100% valid code acceptance |
| **False Positives (FP)** *(False alarms on valid code; naive syntax check = 11)* | 0 | **0** | **0 false alarms** (eliminates 11 naive-check FPs) |
| **Detection Precision (%)** | 100.0% | **100.0%** | Maintained 100.0% precision |
| **Detection Recall (%)** | 32.0% | **100.0%** | **+68.0 percentage points** |
| **F1-Score (%)** | 48.5% | **100.0%** | **+51.5 percentage points** |
| **Cascading / Redundant Diagnostics Count** | 2 | **0** | **-2 redundant cascading errors suppressed** |
| **Explanation Completeness Score (%)** *(Cites decl line + rule + branch/shadow cause + fix)* | 0.0% (0/25) | **100.0% (25/25)** | **+100.0 percentage points** |
| **Total Execution Time across 40 Programs (ms)** | N/A | **136.02 ms** | **3.40 ms/program** |

## 3. Category-Wise Breakdown

| Category | Total Tests (Valid / Faulty) | Baseline TP / Faulty | Explainable TP / Faulty | Baseline FP | Explainable FP | Naive Check FP (Avoided) | Baseline Cascading | Explainable Cascading | Explanation Completeness |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Cat A: Straight-Line & Types** | 8 (3V / 5F) | 2/5 | **5/5** | 0 | **0** | 2 | 0 | **0** | **5/5 (100%)** |
| **Cat B: Branching Flow** | 10 (4V / 6F) | 0/6 | **6/6** | 0 | **0** | 4 | 0 | **0** | **6/6 (100%)** |
| **Cat C: Loop Flow** | 6 (2V / 4F) | 0/4 | **4/4** | 0 | **0** | 1 | 0 | **0** | **4/4 (100%)** |
| **Cat D: Scoping & Shadowing** | 8 (3V / 5F) | 2/5 | **5/5** | 0 | **0** | 1 | 0 | **0** | **5/5 (100%)** |
| **Cat E: Cascading & Complex** | 8 (3V / 5F) | 4/5 | **5/5** | 0 | **0** | 3 | 2 | **0** | **5/5 (100%)** |

## 4. Per-Program Detailed Evaluation Table (All 40 Benchmark Programs)

| ID | Category | Test File | Ground Truth | Baseline Output (Errors) | Explainable Output (Errors) | Explanation Complete? | Primary Explainable Diagnostic / Verification Summary |
| :---: | :--- | :--- | :---: | :---: | :---: | :---: | :--- |
| `A01` | Cat A: Straight-Line & Types | `catA_01_valid_straight_line_init.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `A02` | Cat A: Straight-Line & Types | `catA_02_valid_widening_and_ops.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `A03` | Cat A: Straight-Line & Types | `catA_03_valid_reassign_and_params.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `A04` | Cat A: Straight-Line & Types | `catA_04_error_uninit_direct_read.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' declared on line 3 is used on line 4 before any value is assigned to it. |
| `A05` | Cat A: Straight-Line & Types | `catA_05_error_uninit_self_init.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' declared on line 3 is used on line 3 before any value is assigned to it. |
| `A06` | Cat A: Straight-Line & Types | `catA_06_error_uninit_self_assign.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' declared on line 3 is used on line 4 before any value is assigned to it. |
| `A07` | Cat A: Straight-Line & Types | `catA_07_error_type_mismatch_init.txt` | **Error** (1) | Error (1) | **Error (1)** | Yes | Type mismatch in variable initialization: cannot assign 'float' to variable 'count' of type 'int' (declared on line 4) ('ratio' declared as 'float' on line 3) |
| `A08` | Cat A: Straight-Line & Types | `catA_08_error_redeclaration_and_void.txt` | **Error** (2) | Error (2) | **Error (2)** | Yes | Redeclaration of identifier 'val' in the same scope (previously declared at line 3) |
| `B01` | Cat B: Branching Flow | `catB_01_valid_ifelse_both_assigned.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `B02` | Cat B: Branching Flow | `catB_02_valid_nested_ifelse_all_assigned.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `B03` | Cat B: Branching Flow | `catB_03_valid_branch_early_return.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `B04` | Cat B: Branching Flow | `catB_04_valid_assigned_after_partial_if.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `B05` | Cat B: Branching Flow | `catB_05_error_if_without_else.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' (declared on line 4) may be uninitialized here (used on line 8) because the branch where 'flag' is false (missing else on line 5) does not assign it. |
| `B06` | Cat B: Branching Flow | `catB_06_error_ifelse_then_only.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' (declared on line 4) may be uninitialized here (used on line 10) because the else branch (where 'flag' is false on line 7) does not assign it. |
| `B07` | Cat B: Branching Flow | `catB_07_error_ifelse_else_only.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' (declared on line 4) may be uninitialized here (used on line 10) because the then branch (where 'flag' is true on line 5) does not assign it. |
| `B08` | Cat B: Branching Flow | `catB_08_error_nested_if_missing_inner_else.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' (declared on line 5) may be uninitialized here (used on line 13) because the branch where 'b' is false (missing else on line 7) does not assign it. |
| `B09` | Cat B: Branching Flow | `catB_09_error_nested_else_missing_branch.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' (declared on line 5) may be uninitialized here (used on line 15) because the else branch (where 'b' is false on line 11) does not assign it. |
| `B10` | Cat B: Branching Flow | `catB_10_error_uninit_in_if_condition.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'flag' declared on line 3 is used on line 4 before any value is assigned to it. |
| `C01` | Cat C: Loop Flow | `catC_01_valid_preinit_before_while.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `C02` | Cat C: Loop Flow | `catC_02_valid_assigned_after_while.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `C03` | Cat C: Loop Flow | `catC_03_error_while_only_init.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' (declared on line 4) may be uninitialized here (used on line 9) because the while loop on line 5 may execute zero times. |
| `C04` | Cat C: Loop Flow | `catC_04_error_nested_while_init.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' (declared on line 5) may be uninitialized here (used on line 13) because the while loop on line 6 may execute zero times. |
| `C05` | Cat C: Loop Flow | `catC_05_error_uninit_in_while_cond.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'limit' declared on line 3 is used on line 5 before any value is assigned to it. |
| `C06` | Cat C: Loop Flow | `catC_06_error_read_before_assign_in_loop.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'acc' declared on line 3 is used on line 6 before any value is assigned to it. |
| `D01` | Cat D: Scoping & Shadowing | `catD_01_valid_shadowing_independent_init.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `D02` | Cat D: Scoping & Shadowing | `catD_02_valid_inner_assigns_outer_uninit.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `D03` | Cat D: Scoping & Shadowing | `catD_03_valid_deep_nested_shadowing.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `D04` | Cat D: Scoping & Shadowing | `catD_04_error_shadowed_inner_uninit.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' declared on line 5 is used on line 6 before any value is assigned to it. Scope trace: 'x' resolves to the int declared on line 5 (shadowing the int declared on line 3). |
| `D05` | Cat D: Scoping & Shadowing | `catD_05_error_shadowed_outer_still_uninit.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' declared on line 3 is used on line 8 before any value is assigned to it. Shadow escape trace: an inner declaration of 'x' on line 5 shadowed this variable, leaving the outer 'x' (declared on line 3) uninitialized. |
| `D06` | Cat D: Scoping & Shadowing | `catD_06_error_shadowed_type_mismatch_assign.txt` | **Error** (1) | Error (1) | **Error (1)** | Yes | Type mismatch in assignment: cannot assign 'int' to variable 'x' of type 'bool' (declared on line 5) Scope trace: 'x' resolves to the bool declared on line 5 (shadowing the int declared on line 3). Counterfactual check: 'int' is compatible with the shadowed 'int x' on line 3, but inner 'bool x' on line 5 masks it. |
| `D07` | Cat D: Scoping & Shadowing | `catD_07_error_shadowed_type_mismatch_expr.txt` | **Error** (1) | Error (1) | **Error (1)** | Yes | Arithmetic operator '+' requires numeric operands, got 'bool' and 'int' ('x' declared as 'bool' on line 5) Scope trace: 'x' resolves to the bool declared on line 5 (shadowing the int declared on line 3). |
| `D08` | Cat D: Scoping & Shadowing | `catD_08_error_shadowed_maybe_init_branch.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' (declared on line 6) may be uninitialized here (used on line 10) because the branch where 'flag' is false (missing else on line 7) does not assign it. Scope trace: 'x' resolves to the int declared on line 6 (shadowing the int declared on line 3). |
| `E01` | Cat E: Cascading & Complex | `catE_01_valid_complex_functions_and_branches.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `E02` | Cat E: Cascading & Complex | `catE_02_valid_recursion_and_scopes.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `E03` | Cat E: Cascading & Complex | `catE_03_valid_multi_return_paths.txt` | **Valid** (0) | Valid (0) | **Valid (0)** | N/A | (0 errors - passed) |
| `E04` | Cat E: Cascading & Complex | `catE_04_error_cascade_undeclared_reuse.txt` | **Error** (1) | Error (3) | **Error (1)** | Yes | Undeclared identifier 'missing' in assignment statement (no declaration in scope) |
| `E05` | Cat E: Cascading & Complex | `catE_05_error_cascade_uninit_multiple_reads.txt` | **Error** (1) | Valid (0) | **Error (1)** | Yes | Variable 'x' declared on line 3 is used on line 4 before any value is assigned to it. |
| `E06` | Cat E: Cascading & Complex | `catE_06_error_cascade_nested_bad_expr.txt` | **Error** (1) | Error (1) | **Error (1)** | Yes | Arithmetic operator '+' requires numeric operands, got 'bool' and 'int' ('flag' declared as 'bool' on line 3) |
| `E07` | Cat E: Cascading & Complex | `catE_07_error_bad_assign_suppresses_uninit.txt` | **Error** (1) | Error (1) | **Error (1)** | Yes | Type mismatch in assignment: cannot assign 'bool' to variable 'x' of type 'int' (declared on line 3) |
| `E08` | Cat E: Cascading & Complex | `catE_08_error_return_and_arg_mismatch.txt` | **Error** (2) | Error (2) | **Error (2)** | Yes | Argument 1 of function 'scale' (declared on line 2): type mismatch, expected 'int', got 'bool' ('badArg' declared as 'bool' on line 7) |
