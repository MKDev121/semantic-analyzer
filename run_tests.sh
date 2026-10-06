#!/bin/bash
# Semantic Analyzer Test Suite Runner

EXECUTABLE="./semantic_analyzer"
PASSED=0
FAILED=0

if [ ! -f "$EXECUTABLE" ]; then
    echo "Executable $EXECUTABLE not found. Building now..."
    make
fi

echo "=================================================="
echo "    RUNNING SEMANTIC ANALYZER TEST SUITE"
echo "=================================================="

run_test() {
    local file=$1
    local expected_exit=$2
    local description=$3

    echo -n "Running $file ($description)... "
    $EXECUTABLE "$file" > /dev/null 2>&1
    local exit_code=$?

    if [ $exit_code -eq $expected_exit ]; then
        echo -e "\033[0;32m[PASS]\033[0m"
        ((PASSED++))
    else
        echo -e "\033[0;31m[FAIL]\033[0m (expected exit $expected_exit, got $exit_code)"
        ((FAILED++))
    fi
}

# Positive tests (should pass semantic analysis with exit code 0)
run_test "tests/test1_valid_basic.txt" 0 "Basic Declarations & Expressions"
run_test "tests/test2_valid_scopes.txt" 0 "Lexical Scoping & Shadowing"
run_test "tests/test3_valid_functions.txt" 0 "Functions, Arrays, & Promotions"

# Negative tests (should fail semantic analysis with exit code 1)
run_test "tests/test4_error_undeclared.txt" 1 "Undeclared Variables & Functions"
run_test "tests/test5_error_redeclared.txt" 1 "Duplicate & Redeclared Identifiers"
run_test "tests/test6_error_type_mismatch.txt" 1 "Type Mismatches & Incompatible Ops"
run_test "tests/test7_error_condition.txt" 1 "Non-Boolean Conditions"
run_test "tests/test8_error_function_args.txt" 1 "Function Call Argument Mismatches"
run_test "tests/test9_error_return_mismatch.txt" 1 "Return Type Violations"
run_test "tests/test10_error_invalid_lvalue.txt" 1 "Invalid L-Values & Array Indices"

echo "=================================================="
echo -e "Total Tests: $((PASSED + FAILED)) | \033[0;32mPassed: $PASSED\033[0m | \033[0;31mFailed: $FAILED\033[0m"
echo "=================================================="

if [ $FAILED -eq 0 ]; then
    echo "All tests passed successfully!"
    exit 0
else
    echo "Some tests failed."
    exit 1
fi
