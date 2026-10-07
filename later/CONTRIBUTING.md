# Contributing Guidelines

Thank you for your interest in contributing to the **Semantic Analyzer** project! Contributions from the community, whether bug reports, improvements, documentation, or new features, are welcomed.

---

## Code of Conduct

Please maintain a respectful, constructive, and collaborative tone in all project communications and contributions.

---

## Development Setup

### Prerequisites
- Modern C++ compiler supporting **C++17** (e.g. `g++ >= 9.0` or `clang++ >= 9.0`).
- GNU `make`
- Bash shell (for running test scripts)

### Building the Project
```bash
make clean
make
```

### Running the Test Suite
Before submitting any changes, verify that all test cases pass:
```bash
make test
```
Or run directly:
```bash
./run_tests.sh
```

---

## Project Structure & Architecture

- `src/Token.hpp`: Token definitions, lexical categories, position info.
- `src/Lexer.hpp`, `src/Lexer.cpp`: Deterministic lexical scanner.
- `src/AST.hpp`, `src/AST.cpp`: Abstract syntax tree node definitions and Visitor interface.
- `src/Parser.hpp`, `src/Parser.cpp`: Recursive-descent parser building the AST.
- `src/Type.hpp`, `src/Type.cpp`: Type representation, type coercion/widening, and compatibility logic.
- `src/SymbolTable.hpp`, `src/SymbolTable.cpp`: Lexical scoping tree and identifier resolution.
- `src/SemanticAnalyzer.hpp`, `src/SemanticAnalyzer.cpp`: Two-pass semantic validator and AST annotator.
- `src/ErrorReporter.hpp`: Diagnostics collector with formatted console reports.
- `src/main.cpp`: CLI entry point with options for dumping AST, Typed AST, and Symbol Tables.

---

## Submitting Pull Requests

1. **Fork the repository** on GitHub.
2. **Create a topic branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Commit your changes**:
   - Write clear, concise commit messages following standard conventions.
   - Keep commits focused on a single change or fix.
4. **Ensure tests pass**:
   - If adding a new feature or fixing a bug, add a corresponding test in `tests/`.
   - Update `run_tests.sh` and documentation if applicable.
5. **Push and open a PR**:
   - Provide a clear summary of what was changed and why.
