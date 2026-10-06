# Test Suite & Verification Guide

This document describes the testing methodology, test suites, coverage details, and instructions for running and adding tests.

---

## 1. Testing Methodology

The test suite is structured around two distinct verification categories:
1. **Positive Tests (`test1` - `test3`):** Valid, semantically sound programs that must compile with **0 errors** and produce exit code `0`.
2. **Negative Tests (`test4` - `test10`):** Programs containing deliberate semantic violations designed to test every check performed by the analyzer. Each test must report the expected errors and produce exit code `1`.

---

## 2. Test Suite Catalog

### 2.1 Positive Test Cases

| File | Focus Area | Description | Expected Status |
|:---|:---|:---|:---:|
| `test1_valid_basic.txt` | Core Language Syntax | Tests basic variable declarations (`int`, `float`, `bool`), compound arithmetic expressions, relational and logical operators, control flow statements (`if`-`else`, `while`), and return statements. | **PASS (0 errors)** |
| `test2_valid_scopes.txt` | Lexical Scoping & Shadowing | Tests block scopes up to 3 levels deep. Validates that inner variables correctly shadow outer variables of different types (e.g., `float x` shadowing `int x`), and outer variables regain visibility after block exit. | **PASS (0 errors)** |
| `test3_valid_functions.txt` | Functions & Promotion | Tests multi-parameter functions, `void` return functions, 1D array indexing, mutual recursion / forward references, and automatic numeric promotion (`int` widening to `float`). | **PASS (0 errors)** |

---

### 2.2 Negative Test Cases (Error Detection)

| File | Targeted Semantic Rule | Violations Triggered | Expected Status |
|:---|:---|:---|:---:|
| `test4_error_undeclared.txt` | Declaration Validation | • Undeclared variable used in assignment LHS<br>• Undeclared variable used in arithmetic RHS<br>• Calling undeclared function | **FAIL (3 errors)** |
| `test5_error_redeclared.txt` | Duplicate Declarations | • Redeclaring global variable<br>• Redeclaring local variable in same scope<br>• Duplicate function parameter names<br>• Redeclaring function name | **FAIL (4 errors)** |
| `test6_error_type_mismatch.txt` | Type Compatibility | • Assigning `float` to `int` without cast<br>• Initializing `bool` with integer<br>• Incompatible arithmetic (`int + bool`)<br>• Modulo operator with `float`<br>• Assigning `char` to `int` | **FAIL (6 errors)** |
| `test7_error_condition.txt` | Control Flow Types | • Integer expression as `if` condition<br>• Float expression as `while` condition | **FAIL (2 errors)** |
| `test8_error_function_args.txt` | Function Invocations | • Calling a non-function variable<br>• Passing too few arguments<br>• Passing too many arguments<br>• Argument type mismatch (`bool` where `int` expected) | **FAIL (4 errors)** |
| `test9_error_return_mismatch.txt` | Return Semantics | • Non-void function returning empty `return;`<br>• Return type mismatch (`bool` returned from `int` function)<br>• `void` function returning a value<br>• `return` statement at global scope | **FAIL (4 errors)** |
| `test10_error_invalid_lvalue.txt`| L-Values & Subscripts | • Assigning to function identifier<br>• Assigning directly to entire array<br>• Indexing a non-array variable<br>• Indexing array with `float` instead of `int`<br>• Assigning incompatible type to array element | **FAIL (5 errors)** |

---

## 3. Running the Tests

### Via Makefile
```bash
make test
```

### Via Test Script Directly
```bash
chmod +x run_tests.sh
./run_tests.sh
```

### Running Individual Tests
To inspect full AST, typed AST, and symbol table for an individual test:
```bash
./semantic_analyzer --all tests/test1_valid_basic.txt
```

To view error diagnostics for a negative test:
```bash
./semantic_analyzer tests/test6_error_type_mismatch.txt
```

---

## 4. Adding New Tests

1. Create a new test file under `tests/` (e.g. `tests/test11_my_check.txt`).
2. Add your test program to the file.
3. Open `run_tests.sh` and add a new entry to the appropriate section:
   ```bash
   run_test "tests/test11_my_check.txt" <expected_exit_code> "Test Description"
   ```
   - Use `0` if the test is expected to succeed without errors.
   - Use `1` if the test contains semantic errors.
4. Run `make test` to verify.
