# Technical & Algorithmic Novelty of the Project

This document details the novel contributions implemented in our **Explainable Semantic Analyzer with Scope and Definite-Assignment Checking** (`src/`) and explains how they advance beyond conventional teaching compilers and standard production diagnostics (`gcc -Wuninitialized`, `clang::UninitUse`, and `javac` JLS Chapter 16).

---

## 1. Summary of Novel Contributions

While standard compilers treat **lexical scope resolution**, **bidirectional type checking**, and **data-flow definite-assignment analysis** as isolated passes that emit single-point error messages, our analyzer unifies them into a **Cross-Domain Provenance & Counterfactual Diagnostic Engine** featuring four technical innovations:

| # | Novel Mechanism | Where Implemented | What Standard Compilers Do | What Our Analyzer Does Differently |
| :---: | :--- | :--- | :--- | :--- |
| **1** | **Bidirectional Lexical-Shadowing Provenance (Upward Masking & Downward Shadow-Escape Tracking)** | `SymbolTable::lookupShadowed()`, `Symbol::shadowedByLine` (`src/SymbolTable.hpp`, `src/SemanticAnalyzer.cpp`) | Pop and discard inner scope tables on `}` exit; an outer uninitialized read after a nested block has zero memory that an inner declaration of the same name shadowed it. | Tracks shadowing **both upward** (inner declaration masks outer declaration) **and downward across exited scopes** (`shadowedByLine`), diagnosing when an inner declaration (`int x = 50;`) accidentally shadowed an uninitialized outer `x` instead of assigning to it. |
| **2** | **Counterfactual Scope-Compatibility Verification** | `SemanticAnalyzer::visit(AssignStmtNode*)` (`src/SemanticAnalyzer.cpp`) | Report only that the assigned expression conflicts with the innermost variable's type (`cannot assign 'int' to 'bool'`). | Automatically tests whether the failed assignment or expression **would have been type-valid under the shadowed outer declaration** ($\tau_{\text{rhs}} \preceq \tau_{\text{outer}}$) and emits a `Counterfactual check:` proving the error was caused by lexical masking. |
| **3** | **Provenance-Carrying 4-Point Definite-Assignment Lattice ($\mathcal{L}$)** | `InitState`, `captureInitEnv()`, `restoreInitEnv()`, `guaranteesReturn()` (`src/SemanticAnalyzer.cpp`) | `javac` (JLS Ch. 16) tracks boolean definite assignment and states only `variable x might not have been initialized` without naming the responsible branch or loop. | Embeds structured control-flow path provenance $\rho = (\ell_{\text{branch}}, \text{path\_desc})$ directly into the `MAYBE_INITIALIZED` lattice element, pinpointing whether the `then` branch, `else` branch, missing `else`, or zero-trip `while` loop omitted the assignment while pruning early-return ($\top$) paths. |
| **4** | **Four-Layer Coupled Cascade Suppression** | `Type::getError()`, `poisonedIdentifiers`, `reportedUninitRead`, post-error init promotion (`src/SemanticAnalyzer.cpp`) | Emit cascading secondary type errors and repeated uninitialized-read warnings after a single failed declaration or assignment. | Couples bidirectional `TYPE_ERROR` propagation with scope-chain identifier poisoning, basic-block read deduplication, and post-error initialization promotion, achieving **0 cascading errors** across all 40 benchmark programs. |

---

## 2. Deep Dive into Each Novel Contribution

### 2.1 Bidirectional Lexical-Shadowing Provenance (Upward & Downward Shadow-Escape Traces)

In block-structured imperative languages, variable shadowing interacts subtly with definite assignment in two opposite directions:

1. **Upward Shadow Masking (`catD_04_error_shadowed_inner_uninit.txt`)**:
   An outer variable `int x = 100;` is initialized, but an inner block declares an uninitialized `int x;` and reads it (`int y = x + 1;`).
   - Using `SymbolTable::lookupShadowed(name)`, the analyzer walks past the innermost scope match to locate the shadowed outer declaration and emits:
     ```text
     Reason: Variable 'x' declared on line 5 is used on line 6 before any value is assigned to it. Scope trace: 'x' resolves to the int declared on line 5 (shadowing the int declared on line 3).
     ```

2. **Downward Shadow-Escape Trap (`catD_05_error_shadowed_outer_still_uninit.txt`)**:
   A programmer declares `int x;` in an outer scope (line 3), enters a nested block, and accidentally writes `int x = 50;` (line 5) instead of `x = 50;`. When the nested block exits, the inner `x` is destroyed, leaving the outer `x` uninitialized when read at `return x;` (line 8).
   - **Why standard compilers fail**: Conventional symbol tables destroy or unlink the inner scope upon exiting `}`, losing all record that an inner `x` was declared and initialized on line 5.
   - **Our Solution (`shadowedByLine`)**: Whenever a local variable in an inner scope is initialized or assigned while shadowing an enclosing variable whose state is not `DEFINITELY_INITIALIZED`, our analyzer records `outer->shadowedByLine = innerLine`. When the outer variable is later read while still `UNINITIALIZED`, the analyzer emits a **Shadow Escape Trace** and synthesizes a targeted repair:
     ```text
     Reason: Variable 'x' declared on line 3 is used on line 8 before any value is assigned to it. Shadow escape trace: an inner declaration of 'x' on line 5 shadowed this variable, leaving the outer 'x' (declared on line 3) uninitialized.
     Possible correction: Remove the inner type keyword on line 5 if you intended to assign to the outer 'x' declared on line 3, or initialize 'x' before line 8.
     ```

---

### 2.2 Counterfactual Scope-Compatibility Verification

When a programmer writes an assignment to a shadowed variable that fails type checking (`catD_06_error_shadowed_type_mismatch_assign.txt`):

```c
int main() {
    int x = 10;       // Line 3: outer 'int x'
    {
        bool x = true; // Line 5: inner 'bool x' shadows outer 'int x'
        x = 25;        // Line 6: type error!
    }
    return x;
}
```

Instead of merely reporting that `'int'` cannot be assigned to `'bool'` (or even merely noting that `x` shadows line 3), our analyzer performs a **Counterfactual Type-Compatibility Check**:
- Given the inner binding $\sigma_{\text{in}}(x) : \tau_{\text{in}}$ where $\tau_{\text{rhs}} \not\preceq \tau_{\text{in}}$, the analyzer queries $\sigma_{\text{out}}(x) = \text{lookupShadowed}(x)$ and evaluates the counterfactual predicate:
  $$\text{CounterfactualValid}(x, \tau_{\text{rhs}}) \iff \tau_{\text{rhs}} \preceq \sigma_{\text{out}}(x).\text{type}$$
- Because `int` $\preceq$ `int` holds for the shadowed declaration on line 3, the diagnostic proves that the statement would have succeeded in the enclosing scope:
  ```text
  Reason: Type mismatch in assignment: cannot assign 'int' to variable 'x' of type 'bool' (declared on line 5) Scope trace: 'x' resolves to the bool declared on line 5 (shadowing the int declared on line 3). Counterfactual check: 'int' is compatible with the shadowed 'int x' on line 3, but inner 'bool x' on line 5 masks it.
  ```

---

### 2.3 Provenance-Carrying 4-Point Definite-Assignment Lattice ($\mathcal{L}$)

We extend standard boolean definite-assignment analysis to a 4-element lattice:
$$\mathcal{L} = \{\top \text{ (Early Return)}, \;\text{Init}, \;\text{MayUninit}(\rho), \;\text{Uninit}\}$$
keyed by lexical declaration identity `SymbolKey = (scope->id, sym.name)` so shadowed variables never collide during control-flow merges:
- **Early-Return Pruning ($\top$)**: Using structural return analysis (`guaranteesReturn`), if one branch of an `if-else` returns early and the other initializes `x`, the join operation $\top \sqcap \text{Init} = \text{Init}$ recognizes `x` as `DEFINITELY_INITIALIZED` after the conditional (`catB_03`, `catE_03`), avoiding false positives.
- **Exact Branch & Zero-Trip Loop Provenance ($\rho$)**: When $\text{Init} \sqcap \text{Uninit} = \text{MayUninit}(\rho)$, $\rho$ records the exact condition expression text and source line of the missing `else` branch (`catB_05`), the non-assigning `then`/`else` branch (`catB_06`, `catB_07`), or the zero-trip `while` loop (`catC_03`, `catC_04`).

---

### 2.4 Empirical Validation of Novelty (`evaluation/`)

All novel mechanisms are verified automatically by `./scripts/test.sh` (`python3 evaluation/run_eval.py`) across the 40-program corpus (`evaluation/test_corpus/`):
- **100.0% Recall (25/25 faulty programs detected)** vs. 32.0% (8/25) in a standard type-only baseline.
- **100.0% Precision (0 false positives across 15 valid programs)**, eliminating 11 false alarms that occur under a naive declaration-initializer check.
- **0 Cascading Diagnostics** across all 40 programs.
- **100.0% Explanation Completeness (25/25)** citing declaration lines, control-flow branch/loop provenance, bidirectional shadowing traces, and counterfactual checks.
