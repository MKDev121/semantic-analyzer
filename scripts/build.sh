#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/build"
mkdir -p "${OUT_DIR}"

echo "Building Explainable Semantic Analyzer (build/semantic_final.exe)..."
g++ -std=c++17 -Wall -Wextra -Werror \
    -I "${ROOT_DIR}/src" \
    "${ROOT_DIR}/src/main.cpp" \
    "${ROOT_DIR}/src/Lexer.cpp" \
    "${ROOT_DIR}/src/LexicalValidation.cpp" \
    "${ROOT_DIR}/src/Parser.cpp" \
    "${ROOT_DIR}/src/AST.cpp" \
    "${ROOT_DIR}/src/Type.cpp" \
    "${ROOT_DIR}/src/SemanticAnalyzer.cpp" \
    "${ROOT_DIR}/src/SymbolTable.cpp" \
    -o "${OUT_DIR}/semantic_final.exe"

echo "Build completed successfully: ${OUT_DIR}/semantic_final.exe"
