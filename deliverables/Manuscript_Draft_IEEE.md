# Explainable Semantic Analysis with Scope-Shadowing Traces and Flow-Aware Definite-Assignment Checking for Novice Programming Languages

**Authors**: Compiler Design Lab Project Team  
**Affiliation**: Department of Computer Science and Engineering  
**Date**: October 2026  

---

## Abstract
Conventional compiler front-ends designed for introductory programming courses validate lexical scope and expression type compatibility but frequently omit flow-sensitive definite-assignment analysis or report cryptic, single-site error messages when an uninitialized or shadowed variable is referenced. Consequently, novice programmers face either undetected runtime bugs from uninitialized reads along partial control-flow paths or terse diagnostics that fail to explain *which* branch, loop, or lexical scope caused the violation. In this paper, we present a formal model and C++17 implementation of an **Explainable Semantic Analyzer** for a statically typed C-like language that unifies bidirectional type checking, lexical scope-shadowing resolution, and lattice-based flow-aware definite-assignment analysis. Our analyzer tracks per-symbol initialization states across a four-element lattice $\mathcal{L} = \{\top, \text{Init}, \text{MayUninit}, \text{Uninit}\}$ augmented with control-flow provenance metadata and scope-chain shadowing lookup (`lookupShadowed`). When a semantic invariant fails, the analyzer synthesizes a multi-point diagnostic linking the read/use site, the exact declaration line, the responsible conditional branch or zero-trip loop path, any shadowed outer declaration, and a concrete repair hint, while suppressing cascading secondary errors via poison types and per-block deduplication. Empirical evaluation across a benchmark corpus of 40 labeled programs across five categories demonstrates that our enhanced analyzer improves defect detection recall from **32.0% to 100.0%** (F1-score from **48.5% to 100.0%**), eliminates **11 false positives** that would arise under a naive syntax-only initialization check, achieves **100.0% explanation completeness** (25/25 faulty programs), and suppresses **100% of cascading secondary diagnostics** with negligible runtime overhead (~1.62 ms per compilation unit).

**Keywords** — *Semantic analysis, definite-assignment analysis, compiler diagnostics, lexical scoping, variable shadowing, bidirectional typing, computer science education.*

---

## I. Introduction

Static semantic analysis is the bridge between syntax tree construction and intermediate code generation in a compiler front-end. While lexical and syntax analyzers verify whether an input string belongs to a context-free grammar, the semantic analyzer enforces context-sensitive invariants: declaration-before-use, scope visibility, type compatibility across assignments and operators, function call arity/types, and control-flow completeness.

In educational compiler projects and novice programming environments, two recurring challenges degrade the effectiveness of semantic feedback:
1. **Control-Flow Blindness in Initialization Checking**: A symbol table that only records static declaration attributes cannot distinguish between a variable read before any assignment (`int x; return x;`), a variable assigned on only one branch of a conditional (`if (flag) { x = 1; } return x;`), a variable assigned inside a `while` loop whose body may execute zero times (`while (cond) { x = 1; } return x;`), and a variable safely initialized across both branches of an `if-else` statement (`if (flag) { x = 1; } else { x = 2; } return x;`). Ignoring initialization yields *false negatives* (missed bugs), whereas requiring inline initializers at declaration time yields *false positives* (rejecting valid multi-branch initialization).
2. **Lack of Provenance in Shadowing and Flow Diagnostics**: Even production compilers that detect uninitialized reads (e.g., `gcc -Wuninitialized` or `javac` enforcing JLS Chapter 16 [3]) typically emit localized messages such as `variable 'x' might not have been initialized` at the use site without stating *which branch or loop on which line failed to assign `x`*. Furthermore, when a student declares an inner variable `x` that shadows an enclosing variable `x` of a different type or initialization state, standard diagnostics report only the inner type mismatch or uninitialized read without exposing the lexical shadowing relationship.

### Contributions
This paper makes the following contributions:
- **Provenance-Carrying Definite-Assignment Lattice (Section III)**: We formulate syntax-directed data-flow transfer equations over $\mathcal{L} = \{\top, \text{Init}, \text{MayUninit}(\rho), \text{Uninit}\}$ where the `MayUninit` state carries structured provenance $\rho = (\ell_{\text{branch}}, \text{path\_desc})$ identifying the exact conditional branch or zero-trip loop responsible for potential uninitialization, alongside scope-shadowing resolution (`lookupShadowed`).
- **Four-Layer Cascading Suppression & Explainable C++17 Architecture (Section IV)**: We implement environment cloning (`captureInitEnv`/`restoreInitEnv`), early-return path pruning (`guaranteesReturn`), scope-chain shadowing traces, and four-layer cascading error suppression in a modular C++17 compiler front-end (`later/src/`).
- **Empirical Evaluation on 40 Labeled Benchmark Programs (Section V)**: We evaluate our Enhanced Analyzer against the 75% Baseline Analyzer across 40 test programs spanning five categories, measuring True Positives, False Negatives, False Positives, Cascading Error Reduction, Explanation Completeness, and Execution Time.

---

## II. Related Work

**Bidirectional Type Checking and Error Localization**: Dunfield and Krishnaswami [1] surveyed bidirectional typing, separating *type synthesis* ($\Gamma \vdash e \Rightarrow \tau$) from *type checking* ($\Gamma \vdash e \Leftarrow \tau$). Haack and Wells [6] introduced type error slicing to link all program points contributing to a type conflict, while Bhanuka et al. [9] modeled type errors as faulty data flows across source locations. Unlike constraint-based type inference in functional languages, our work targets an explicitly typed imperative language where confusion arises from the interaction of *mutable initialization flow* and *nested lexical shadowing*.

**Definite-Assignment Analysis**: Chapter 16 of the Java Language Specification (JLS) [3] and Fruja's formalization for C# [5] define flow-sensitive rules ensuring every local variable is definitely assigned before use. Similarly, TypeScript's control-flow narrowing [4] refines symbol states across branches and early returns. While `javac` and C# enforce definite assignment as a boolean pass/fail property, our lattice attaches human-readable branch and loop provenance to `MAYBE_INITIALIZED` transitions so the resulting diagnostic explains *why* definite assignment failed.

**Expressive Compiler Diagnostics for Novices**: Clang [2] popularized source excerpts, column carets, and Fix-It hints, and provides `clang::UninitUse` to classify uninitialized reads. In computing education, Becker et al. [7], [8] demonstrated empirically that enhanced compiler messages that state the violated rule and connect use sites to declaration context reduce student error repetition. Our work operationalizes these guidelines for scope shadowing and flow-sensitive initialization.

---

## III. Formal Model of Flow-Aware Explainable Diagnostics

### A. Scoped Symbol Environment and Shadowing Lookup
Let $\Sigma$ be a tree of lexical scopes where each scope $\sigma \in \Sigma$ has a unique identifier $\text{id}(\sigma) \in \mathbb{N}$ and parent pointer $\text{parent}(\sigma)$. Each declared variable is uniquely addressed by key $k = (\text{id}(\sigma), x)$.
- **Primary Resolution**: $\text{lookup}(x, \sigma)$ returns the symbol entry $\sigma_i(x)$ from the nearest enclosing scope $\sigma_i$ (starting at $\sigma$) such that $x \in \text{dom}(\sigma_i)$.
- **Shadowing Resolution**: Given $\sigma_i$ where $\text{lookup}(x, \sigma)$ succeeded, we define:
  $$\text{lookupShadowed}(x, \sigma) = \text{lookup}(x, \text{parent}(\sigma_i))$$
  If $\text{lookupShadowed}(x, \sigma) = \sigma_o(x) \neq \bot$, identifier $x$ at $(\sigma_i, \ell_{\text{in}}, \tau_{\text{in}})$ shadows the outer declaration at $(\sigma_o, \ell_{\text{out}}, \tau_{\text{out}})$, generating the scope trace annotation:
  $$\mathcal{T}_{\text{shadow}}(x) = \text{``'x' resolves to the }\tau_{\text{in}}\text{ declared on line }\ell_{\text{in}}\text{ (shadowing the }\tau_{\text{out}}\text{ declared on line }\ell_{\text{out}}\text{).''}$$

### B. Provenance-Augmented Initialization Lattice
Each variable key $k = (\text{id}(\sigma), x)$ is mapped by an initialization environment $E : K \to \mathcal{L}$ to a state in the lattice:
$$\mathcal{L} = \{\top, \;\text{Init}, \;\text{MayUninit}(\rho), \;\text{Uninit}\}, \quad \text{Uninit} \sqsubseteq \text{MayUninit}(\rho) \sqsubseteq \text{Init} \sqsubseteq \top$$
where $\top$ denotes an unreachable control-flow path (after a guaranteed `return` statement), and $\rho$ records the source line and control-flow branch or loop description where initialization was missed.

### C. Syntax-Directed Transfer Functions
For any statement $S$ with incoming environment $E_{\text{in}}$:
1. **Declaration (`T x;` or `T x = e;` at line $\ell$)**:
   $x$ is registered in the current scope $\sigma_c$ with $E_0 = E_{\text{in}}[(\text{id}(\sigma_c), x) \mapsto \text{Uninit}]$. If initializer $e$ is present, $e$ is checked under $E_0$ (detecting self-referential reads such as `int x = x + 1;`), after which $E_{\text{out}} = E_0[(\text{id}(\sigma_c), x) \mapsto \text{Init}]$.
2. **Assignment (`x = e;` at line $\ell$)**:
   $e$ is checked under $E_{\text{in}}$ first (detecting uninitialized reads in `x = x + 10;`), and then the resolved key $k_x$ is updated to $E_{\text{out}} = E_{\text{in}}[k_x \mapsto \text{Init}]$.
3. **Conditional (`if (c) S_1 else S_2` at line $\ell$)**:
   After checking condition $c$ under $E_{\text{in}}$, $S_1$ is analyzed under a clone of $E_{\text{in}}$ yielding $E_1$, and $S_2$ is analyzed under a fresh clone of $E_{\text{in}}$ yielding $E_2$ (with $E_2 = E_{\text{in}}$ if `else` is omitted). Let $R(S) \in \{\text{true}, \text{false}\}$ denote structural return guarantee (`guaranteesReturn`). For each key $k \in \text{dom}(E_{\text{in}})$:
   $$E_{\text{out}}(k) = \begin{cases}
   \text{Init} & \text{if } E_{\text{in}}(k) = \text{Init} \\
   \text{Init} & \text{if } (R(S_1) \land \neg R(S_2) \land E_2(k) = \text{Init}) \lor (R(S_2) \land \neg R(S_1) \land E_1(k) = \text{Init}) \\
   \text{Init} & \text{if } E_1(k) = \text{Init} \land E_2(k) = \text{Init} \\
   \text{Uninit} & \text{if } E_1(k) = \text{Uninit} \land E_2(k) = \text{Uninit} \\
   \text{MayUninit}(\rho_{\text{cond}}) & \text{otherwise}
   \end{cases}$$
   where $\rho_{\text{cond}}$ distinguishes between:
   - Missing `else`: `"the branch where '<c>' is false (missing else on line <l>) does not assign it"`
   - Unassigned `else`: `"the else branch (where '<c>' is false on line <l_else>) does not assign it"`
   - Unassigned `then`: `"the then branch (where '<c>' is true on line <l>) does not assign it"`
   - Nested `MayUninit`: propagates the inner branch provenance $\rho_1$ or $\rho_2$.
4. **While Loop (`while (c) S_b` at line $\ell$)**:
   After checking $c$ and analyzing $S_b$ to obtain $E_b$, any variable $k$ with $E_{\text{in}}(k) = \text{Uninit}$ and $E_b(k) \in \{\text{Init}, \text{MayUninit}\}$ transitions to:
   $$E_{\text{out}}(k) = \text{MayUninit}(\text{``the while loop on line }\ell\text{ may execute zero times''})$$

---

## IV. Implementation Architecture

The compiler front-end is implemented in C++17 across three modular directories: `src/` (25% lexer and 75% baseline semantic analyzer), `shared/` (`SymbolTable.hpp/.cpp`), and `later/src/` (100% enhanced analyzer):
1. **Two-Pass Top-Level Registration (`registerTopLevelDeclarations`)**: Registers all global variables and function signatures prior to body traversal, enabling forward function calls and mutual recursion.
2. **Environment Snapshot & Restoration (`captureInitEnv` / `restoreInitEnv`)**: Traverses active `Scope` pointers from `currentScope` to `rootScope`, snapshotting and restoring `VarInitSnapshot` states in $O(|V_{\text{active}}|)$ time per branch or loop without deep-cloning the AST or symbol-table tree.
3. **Four-Layer Cascading Error Suppression**:
   - *Expression Poisoning (`Type::getError()`)*: Suppresses parent operator and initializer type errors when an operand is ill-typed.
   - *Scope-Level Undeclared Identifier Poisoning (`poisonedIdentifiers`)*: Suppresses repeated "undeclared identifier" complaints when an undeclared variable or function is referenced multiple times within the same scope.
   - *Basic-Block Uninitialized Read Deduplication (`reportedUninitRead`)*: Suppresses duplicate uninitialized diagnostics when an uninitialized variable is read multiple times in the same basic block.
   - *Post-Error Initialization Promotion*: Transitions variables to `DEFINITELY_INITIALIZED` even after an ill-typed initializer or assignment (`int x = true;`) so subsequent reads of `x` do not trigger false secondary uninitialized-read errors.

---

## V. Experimental Evaluation on 40 Benchmark Programs

### A. Benchmark Corpus and Methodology
We constructed an automated evaluation suite (`evaluation/run_eval.py`) comprising **40 labeled test programs** (`evaluation/test_corpus/`, 15 valid and 25 faulty) partitioned into five categories:
- **Category A (8 programs: 3V, 5F)**: Straight-line declarations, type checking, self-initialization (`int x = x + 1;`), and self-assignment (`x = x + 10;`).
- **Category B (10 programs: 4V, 6F)**: Branching control flow (`if` without `else`, one-sided `if-else`, two-sided `if-else`, early-return branch, nested `if-else`, uninitialized condition).
- **Category C (6 programs: 2V, 4F)**: Loop control flow (`while` zero-trip assignment, nested `while`, uninitialized loop condition, read-before-assign inside loop).
- **Category D (8 programs: 3V, 5F)**: Lexical scoping and variable shadowing interacting with definite assignment and expression/assignment types.
- **Category E (8 programs: 3V, 5F)**: Cascading error suppression and multi-function control-flow combinations.

We executed both the **Baseline Analyzer** (`build/semantic_demo.exe`) and the **Enhanced Analyzer** (`build/semantic_final.exe`) on all 40 programs and recorded empirical metrics in `evaluation/RESULTS.md`.

### B. Empirical Results and Comparison

| Evaluation Metric | Baseline Analyzer (`semantic_demo.exe`) | Enhanced Analyzer (`semantic_final.exe`) | Improvement |
| :--- | :---: | :---: | :---: |
| **True Positives (TP)** *(out of 25 faulty programs)* | 8 / 25 | **25 / 25** | **+17 (+68.0% recall)** |
| **False Negatives (FN)** *(missed faulty programs)* | 17 | **0** | **-17 (100% eliminated)** |
| **True Negatives (TN)** *(out of 15 valid programs)* | 15 / 15 | **15 / 15** | 100% valid code accepted |
| **False Positives (FP)** *(naive syntax check = 11)* | 0 | **0** | **0 false alarms** |
| **Precision / Recall / F1-Score (%)** | 100.0% / 32.0% / 48.5% | **100.0% / 100.0% / 100.0%** | **+51.5% F1-score** |
| **Cascading / Redundant Diagnostics** | 2 | **0** | **100% cascades suppressed** |
| **Explanation Completeness Score (%)** | 0.0% (0/25) | **100.0% (25/25)** | **+100.0 percentage points** |
| **Total Runtime across 40 Programs (ms)** | 64.93 ms | **64.90 ms** | ~1.62 ms/program |

### C. Category-Level Performance

| Category | Tests (V/F) | Baseline Recall | Enhanced Recall | Naive Check FPs Avoided | Enhanced Cascading Errors | Enhanced Explanation Completeness |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Cat A: Straight-Line & Types** | 8 (3V / 5F) | 40.0% (2/5) | **100.0% (5/5)** | 2 | 0 | **100% (5/5)** |
| **Cat B: Branching Flow** | 10 (4V / 6F) | 0.0% (0/6) | **100.0% (6/6)** | 4 | 0 | **100% (6/6)** |
| **Cat C: Loop Flow** | 6 (2V / 4F) | 0.0% (0/4) | **100.0% (4/4)** | 1 | 0 | **100% (4/4)** |
| **Cat D: Scoping & Shadowing** | 8 (3V / 5F) | 40.0% (2/5) | **100.0% (5/5)** | 1 | 0 | **100% (5/5)** |
| **Cat E: Cascading & Complex** | 8 (3V / 5F) | 80.0% (4/5) | **100.0% (5/5)** | 3 | 0 | **100% (5/5)** |

### D. Discussion of Key Findings
1. **Elimination of False Negatives across Control Flow**: The Baseline Analyzer missed all 17 initialization-related bugs (3 in Category A, 6 in Category B, 4 in Category C, 3 in Category D, and 1 in Category E). By tracking $\mathcal{L}$-states across branches and loops, the Enhanced Analyzer detected all 25/25 faulty programs.
2. **Elimination of Naive False Positives**: A syntax-only analyzer requiring an initializer at every declaration (`Type id = expr;`) would falsely reject 11 of the 15 valid benchmark programs (73.3% FP rate), including programs that initialize variables across both arms of `if-else` (`B01`, `B02`, `E01`, `E02`) or after an early-return guard (`B03`, `E03`). Our lattice merge operator achieves **0 false positives**.
3. **Diagnostic Quality and Cascade Suppression**: On test `E04` (multiple uses of an undeclared identifier), the Baseline Analyzer emitted 3 errors (2 redundant cascades), whereas the Enhanced Analyzer's scope-level identifier poisoning emitted exactly 1 error. Similarly, on `E05` (three reads of an uninitialized variable in one basic block), `reportedUninitRead` suppressed the 2 duplicate warnings. Across all 25 faulty programs, 100% of Enhanced diagnostics accurately cited the declaration line, violated rule, branch/loop or shadowing trace, and repair suggestion.

---

## VI. Conclusion & Future Work

We have presented an **Explainable Semantic Analyzer with Scope and Definite-Assignment Checking** that combines bidirectional type checking, lexical shadowing traces, and provenance-carrying flow-sensitive definite-assignment analysis. Empirical evaluation on 40 benchmark programs confirms 100% precision and recall, zero cascading error noise, and 100% explanation completeness at negligible compile-time cost. Future work includes extending the data-flow lattice with constant-condition folding, per-element array bounds and initialization tracking, and conducting a controlled classroom user study measuring student bug-repair time.

---

## References
[1] J. Dunfield and N. R. Krishnaswami, "Bidirectional typing," *ACM Comput. Surv.*, vol. 54, no. 5, Art. no. 98, pp. 1–38, May 2021.  
[2] LLVM Project, "Clang Expressive Diagnostics and `clang::UninitUse`," 2026. [Online]. Available: https://clang.llvm.org/diagnostics.html  
[3] J. Gosling et al., "Chapter 16: Definite Assignment," in *The Java Language Specification, Java SE 21 Edition*, Oracle, 2023.  
[4] Microsoft Corporation, "Narrowing," in *The TypeScript Handbook*, 2026. [Online]. Available: https://www.typescriptlang.org/docs/handbook/2/narrowing.html  
[5] N. G. Fruja, "The correctness of the definite assignment analysis in C#," *J. Object Technol.*, vol. 3, no. 9, pp. 29–52, Oct. 2004.  
[6] C. Haack and J. B. Wells, "Type error slicing in implicitly typed higher-order languages," *Sci. Comput. Program.*, vol. 50, no. 1–3, pp. 189–224, Mar. 2004.  
[7] B. A. Becker et al., "Effective compiler error message enhancement for novice programming students," *Comput. Sci. Educ.*, vol. 26, no. 2–3, pp. 148–175, 2016.  
[8] B. A. Becker et al., "Compiler error messages considered unhelpful: The landscape of text-based programming error message research," in *Proc. ITiCSE Working Group Reports*, 2019, pp. 177–210.  
[9] I. Bhanuka, L. Parreaux, D. Binder, and J. I. Brachthäuser, "Getting into the flow: Towards better type error messages for constraint-based type inference," *Proc. ACM Program. Lang.*, vol. 7, no. OOPSLA2, Art. no. 237, pp. 419–447, Oct. 2023.  
