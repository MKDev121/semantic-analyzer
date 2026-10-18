#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/build"
mkdir -p "${OUT_DIR}"

echo "[1/3] Building 25% Milestone (build/review_1.exe)..."
g++ -std=c++17 -Wall -Wextra -Werror \
    -I "${ROOT_DIR}/src" \
    "${ROOT_DIR}/src/main_1.cpp" \
    "${ROOT_DIR}/src/Lexer.cpp" \
    -o "${OUT_DIR}/review_1.exe"

echo "[2/3] Building 75% Baseline Milestone (build/semantic_demo.exe)..."
g++ -std=c++17 -Wall -Wextra -Werror \
    -I "${ROOT_DIR}/src" -I "${ROOT_DIR}/shared" \
    "${ROOT_DIR}/src/main_2.cpp" \
    "${ROOT_DIR}/src/Lexer.cpp" \
    "${ROOT_DIR}/src/LexicalValidation.cpp" \
    "${ROOT_DIR}/src/Parser.cpp" \
    "${ROOT_DIR}/src/AST.cpp" \
    "${ROOT_DIR}/src/Type.cpp" \
    "${ROOT_DIR}/src/SemanticAnalyzer.cpp" \
    "${ROOT_DIR}/shared/SymbolTable.cpp" \
    -o "${OUT_DIR}/semantic_demo.exe"

echo "[3/3] Building 100% Final Explainable Analyzer (build/semantic_final.exe)..."
g++ -std=c++17 -Wall -Wextra -Werror \
    -I "${ROOT_DIR}/later/src" -I "${ROOT_DIR}/shared" -I "${ROOT_DIR}/src" \
    "${ROOT_DIR}/later/src/main_3.cpp" \
    "${ROOT_DIR}/src/Lexer.cpp" \
    "${ROOT_DIR}/later/src/LexicalValidation.cpp" \
    "${ROOT_DIR}/later/src/Parser.cpp" \
    "${ROOT_DIR}/later/src/AST.cpp" \
    "${ROOT_DIR}/later/src/Type.cpp" \
    "${ROOT_DIR}/later/src/SemanticAnalyzer.cpp" \
    "${ROOT_DIR}/shared/SymbolTable.cpp" \
    -o "${OUT_DIR}/semantic_final.exe"

echo "All milestones built successfully in ${OUT_DIR}/"
