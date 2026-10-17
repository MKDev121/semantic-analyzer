# Demonstrating 25%, 75%, and the final-review features

Run these commands from `semantic-analyzer` in PowerShell:

```powershell
.\scripts\build.ps1 -Milestone All
.\scripts\test.ps1 -SkipBuild
```

Executables are generated in `build/`. Some original presentation screenshots
may show older run locations; use the canonical commands in this guide for
the current repository layout.

## Show the 25% milestone

```powershell
.\build\review_1.exe .\tests\review_1_tests\test1_declarations.txt
.\build\review_1.exe .\tests\review_1_tests\test2_operators.txt
.\build\review_1.exe .\tests\review_1_tests\test3_unknown_character.txt
```

Explain keyword/identifier/literal classification, operator recognition,
comment skipping, and token line/column positions. The last example shows an
unknown-character error. This milestone does not check grammar, declarations,
or expression types.

## Show the core 75% milestone

```powershell
.\build\semantic_demo.exe --all .\tests\review_2_tests\test3_valid_functions.txt
.\build\semantic_demo.exe --dump-symbols .\tests\review_2_tests\test2_valid_scopes.txt
.\build\semantic_demo.exe .\tests\review_2_tests\test4_error_undeclared.txt
.\build\semantic_demo.exe .\tests\review_2_tests\test5_error_redeclared.txt
.\build\semantic_demo.exe .\tests\review_2_tests\test6_error_type_mismatch.txt
.\build\semantic_demo.exe .\tests\review_2_tests\test7_error_function_args.txt
.\build\semantic_demo.exe .\tests\review_2_tests\test8_error_return_mismatch.txt
```

The first example shows tokens, the basic AST, and the symbol hierarchy.
Explain scalar operator types, `int` to `float` widening, function argument
checks, and explicit return-type checks. Functions are defined before callers;
direct self-recursion is allowed. The scope example shows nested blocks and
name shadowing.

The error examples demonstrate undeclared names, duplicate declarations,
scalar type/operator errors, wrong function arguments, and wrong explicit
return values. Show the phase, line/column, source caret, and reason. A program
with detected errors returns exit code 1.

## Show the distinction between phases

```powershell
'int main() { bool flag = true & false; return 0; }' | .\build\semantic_demo.exe
'int main() { int value = ; return 0; }' | .\build\semantic_demo.exe
'int main() { bool flag = 7; return 0; }' | .\build\semantic_demo.exe
```

These produce lexical, syntax, and semantic errors respectively. The missing
expression stops parsing; the integer initializer fails the Boolean assignment
rule. Lexical and syntax failures prevent semantic analysis.

## Reserve these demonstrations for the final review

```powershell
.\build\semantic_final.exe --all .\later\tests\semantic\test3_valid_functions.txt
.\build\semantic_final.exe .\later\tests\test1_valid_forward_and_mutual_recursion.txt
.\build\semantic_final.exe .\later\tests\test3_error_missing_return.txt
```

The separate `later/` build preserves the existing extended analyzer: arrays and element checks,
forward calls/mutual recursion, missing-return path analysis, typed AST display,
and possible correction guidance. Its `--all` includes a typed AST alongside
tokens, the basic AST, and symbols. It also accepts `--dump-typed-ast` directly.

The array example and the forward-call example exceed the active 75% scope.
The last example receives missing-return diagnostics in the final implementation;
the 75% analyzer still checks the types of return statements that are present.

## Explain the remaining work

The project analyzes a compiler front-end; it does not execute the samples.
The advanced features above are already implemented and reserved for the
final demonstration. Uninitialized-variable, unused-variable, unreachable-code,
and array-bounds checks remain future improvements, along with final report
and presentation work. Read `LANGUAGE_SPEC.md` for the implementation-specific rules.
