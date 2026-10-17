# Research basis and proposed contribution

Status: initial literature survey and research proposal, 17 October 2026.
The proposed extension is not implemented or evaluated by this document.

## What can currently be claimed

The existing project implements semantic checks for a small, statically typed,
C-like language. Its final variant already includes correction guidance and a
typed AST. Uninitialized-variable analysis is listed as unfinished in the project
README and is absent from the final analyzer's variable-read visitor.

Adding definite-assignment analysis would extend this project. It would not, by
itself, establish a new compiler algorithm. Explanations that identify problematic
branches also exist in established compilers. A claim of research originality
therefore remains unproven.

## Initial literature survey

The summaries below cover verified publication metadata and the relevant
published ideas. Proposed adaptations are our design choices. They are not claims
that the papers lack these capabilities or that our implementation improves on
their results.

### 1. Definite-assignment analysis

Nicu G. Fruja. 2004. *The Correctness of the Definite Assignment Analysis in C#*.
Journal of Object Technology, 3(9), 29-52.
[Publisher record](https://www.jot.fm/issues/issue_2004_10/article2/index.html)
and [paper](https://www.jot.fm/issues/issue_2004_10/article2.pdf).

The paper formalizes C# definite assignment with data-flow equations and proves
correctness. It tracks assignment information before and after statements and
accounts for control flow. Its treatment also covers constructs outside our
language, such as goto and ref/out parameters.

Relevance: this is the direct algorithmic foundation for requiring initialization
on every incoming path before a read. We can adapt the applicable rules to our
smaller grammar, distinguish declarations by scope, and handle branches that
return before reaching a later read. This adaptation inherits established ideas;
the paper's proof does not automatically prove our implementation correct.

### 2. Connecting diagnostics to relevant source locations

Christian Haack and J. B. Wells. 2004. *Type error slicing in implicitly typed
higher-order languages*. Science of Computer Programming, 50(1-3), 189-224.
DOI: [10.1016/j.scico.2004.01.004](https://doi.org/10.1016/j.scico.2004.01.004).
[Publisher abstract](https://www.sciencedirect.com/science/article/pii/S016764230400005X).

The published abstract describes identifying a type error through a set of
relevant program points, with completeness and minimality criteria, rather than
blaming only one location.

Relevance: this motivates connecting an error to its relevant declaration and
other contributing locations. Our proposed branch explanations are not an
implementation of the paper's minimal type-error-slicing algorithm. The paper
addresses implicitly typed higher-order languages, while our language uses
explicit scalar declarations. This entry is based on the journal abstract; a
detailed algorithm comparison remains pending.

### 3. Evaluating enhanced messages for beginners

Brett A. Becker et al. 2016. *Effective compiler error message enhancement for
novice programming students*. Computer Science Education, 26(2-3), 148-175.
DOI: [10.1080/08993408.2016.1225464](https://doi.org/10.1080/08993408.2016.1225464).
[Author manuscript in the UCD repository](https://researchrepository.ucd.ie/server/api/core/bitstreams/7fb3b47b-769e-4616-8809-ba4e72a654ae/content).

The authors evaluate enhanced Java compiler messages through a control/intervention
study. They report reductions in overall errors, errors per student, and several
repeated-error measures in the intervention group.

Relevance: this provides an empirical model for evaluating feedback instead of
assuming longer messages help. Its findings concern its Java teaching context;
they do not establish that our proposed messages improve learning. Such a claim
would require a suitable study of our own system.

### 4. Survey of programming error-message research

Brett A. Becker et al. 2019. *Compiler Error Messages Considered Unhelpful: The
Landscape of Text-Based Programming Error Message Research*. 2019 ITiCSE Working
Group Reports.
DOI: [10.1145/3344429.3372508](https://doi.org/10.1145/3344429.3372508).
[Author copy](https://www.brettbecker.com/wp-content/uploads/2019/12/becker2019compiler.pdf).

This survey synthesizes research on programming error messages and presents
design guidelines classified by the evidence supporting them. It also provides
a bibliography for further investigation.

Relevance: use it to choose message-design principles and find additional related
work. It supports an evidence-based approach to diagnostic design, not a claim
that our combination of features is absent from prior work. This initial entry
uses the verified abstract and publication metadata; a guideline-by-guideline
review remains pending.

### 5. Explaining errors through data flow

Ishan Bhanuka, Lionel Parreaux, David Binder, and Jonathan Immanuel Brachthaeuser.
2023. *Getting into the Flow: Towards Better Type Error Messages for
Constraint-Based Type Inference*. Proceedings of the ACM on Programming
Languages, 7, OOPSLA2, Article 237.
DOI: [10.1145/3622812](https://doi.org/10.1145/3622812).
[Author paper](https://binderdavid.github.io/publications/GettingIntoTheFlow.pdf).

The work explains type errors as faulty data flows and presents sequences of
relevant program locations. Its user study did not establish quantitative
improvements in localization or understanding, although qualitative feedback
indicated interest in flow-based explanations.

Relevance: this motivates explanatory traces and comparison against simpler
messages. The paper concerns constraint-based type inference; our proposed
extension concerns initialization facts in an explicitly typed language. Better
explanations remain a hypothesis to test, not an established outcome for our tool.

## Existing-system comparison that limits the novelty claim

Clang's documented UninitUse representation already distinguishes possibly
uninitialized use from use that is uninitialized when a particular branch is
taken, and retains associated branch information.
[Clang UninitUse documentation](https://clang.llvm.org/doxygen/classclang_1_1UninitUse.html).

Consequently, neither detecting uninitialized reads nor identifying a responsible
branch is sufficient evidence of research novelty. Clang's broader diagnostic
features are also relevant prior art:
[Expressive Diagnostics](https://clang.llvm.org/diagnostics.html).

## Proposed project contribution

Working title: **Explainable Semantic Analysis with Scope and Definite-Assignment
Checking**.

Develop a structured teaching trace that connects:

1. The variable read and its source location.
2. The exact declaration selected by lexical scope lookup.
3. The relevant initialization facts at incoming branches or a loop exit.
4. The semantic rule that cannot be established.
5. A suggested action supported by those facts.

Use declaration identity, rather than the variable name alone, so shadowed names
retain separate initialization states. For initial scope, support scalar locals,
parameters, assignments, blocks, if/else, while, and return. Specify the treatment
of globals and arrays separately before implementation.

A conservative analysis may identify a syntactic path it cannot rule out. The
trace must not present such a path as a proven executable counterexample.
Correction suggestions must also avoid assuming a suitable initialization value.

This is a candidate design and evaluation contribution for the course project.
A structured trace format or feature combination is not automatically novel
research. More detailed prior-art comparison is necessary before claiming an
original method or a publication-level advance.

## Evaluation plan

Separate detection coverage from explanation quality.

### Detection coverage

Compare the current analyzer with the extended analyzer using a proposed set of
48 labeled programs: six cases in each of eight categories, balanced between
valid and invalid inputs where applicable.

- Straight-line initialization and self-initialization.
- One-sided conditional assignment.
- Assignment in both branches.
- Nested conditionals.
- Shadowing and nested scopes.
- Loop bodies that may never execute.
- Branches that return before a later read.
- Interactions between invalid assignments and initialization facts.

Report expected versus actual classifications, false positives, false negatives,
and regression results for the existing type and scope checks. State the chosen
language rules and conservative approximations before labeling the examples.

### Explanation comparison

Hold the extended analysis constant and compare three output modes:

1. Error location and short message.
2. Error plus rule explanation and declaration location.
3. Structured scope and initialization trace.

Check that every reported declaration, branch, and rule matches the labeled
example. Record trace size and analysis overhead. These checks evaluate technical
correctness and cost; they do not measure student understanding. A separate
task-based study would be needed to assess identification accuracy or repair time.

### Research questions

- RQ1: Does the extension classify the specified initialization cases correctly?
- RQ2: Are the extra source locations and rule explanations accurate and relevant?
- RQ3: What additional processing cost does trace generation introduce?
- Optional RQ4: Do students identify or repair errors more effectively with the
  structured trace than with simpler diagnostics?

## Suggested wording for the report

> Building on established definite-assignment analysis and compiler-diagnostic
> research, this project proposes a structured explanation mechanism for a small
> C-like teaching language. The mechanism links variable uses to resolved
> declarations and initialization facts across control flow. We plan to evaluate
> detection correctness, explanation accuracy, and processing overhead.

Replace proposed work with completed-work statements only after implementation
and evaluation. Add measured results only after collecting them.

## Remaining research work

- Read the complete slicing paper and diagnostic survey before making detailed
  comparisons with their algorithms or guidelines.
- Compare the proposed trace design directly with Clang and other relevant
  teaching-language tools.
- Identify a specific supported difference, or retain the honest framing of an
  implementation and evaluation project if no research gap is established.
- Select a new algorithmic question only if the faculty's novelty requirement
  demands one; the present evidence does not establish such an algorithm.
