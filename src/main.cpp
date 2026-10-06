#include "Lexer.hpp"
#include "Parser.hpp"
#include "SymbolTable.hpp"
#include "SemanticAnalyzer.hpp"
#include "ErrorReporter.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [options] <source-file>\n"
              << "Options:\n"
              << "  --dump-ast        Print the abstract syntax tree\n"
              << "  --dump-typed-ast  Print the AST annotated with inferred types\n"
              << "  --dump-symbols    Print the hierarchical symbol table\n"
              << "  --all             Dump AST, Typed AST, and Symbol Table\n"
              << "  -h, --help        Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string filename = "";
    bool dumpAST = false;
    bool dumpTypedAST = false;
    bool dumpSymbols = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-ast") {
            dumpAST = true;
        } else if (arg == "--dump-typed-ast") {
            dumpTypedAST = true;
        } else if (arg == "--dump-symbols") {
            dumpSymbols = true;
        } else if (arg == "--all") {
            dumpAST = true;
            dumpTypedAST = true;
            dumpSymbols = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        } else {
            filename = arg;
        }
    }

    std::string source;
    if (filename.empty()) {
        std::cerr << "Reading from standard input (press Ctrl+D to finish):\n";
        std::string line;
        while (std::getline(std::cin, line)) {
            source += line + "\n";
        }
    } else {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file '" << filename << "'\n";
            return 1;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        source = ss.str();
    }

    ErrorReporter reporter;

    // 1. Lexical Analysis
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    // 2. Syntax Analysis (Parsing)
    Parser parser(tokens, reporter);
    auto ast = parser.parseProgram();

    if (reporter.hasErrors()) {
        std::cerr << "Syntax errors encountered during parsing:\n";
        reporter.printReport(std::cerr);
        return 1;
    }

    if (dumpAST && ast) {
        std::cout << "\n============================== INITIAL AST ==============================\n";
        ast->print(std::cout);
        std::cout << "=========================================================================\n\n";
    }

    // 3. Semantic Analysis
    SymbolTable symbolTable;
    SemanticAnalyzer analyzer(symbolTable, reporter);
    if (ast) {
        analyzer.analyze(ast.get());
    }

    if (dumpTypedAST && ast) {
        std::cout << "\n=============================== TYPED AST ===============================\n";
        ast->print(std::cout);
        std::cout << "=========================================================================\n\n";
    }

    if (dumpSymbols) {
        symbolTable.printHierarchy(std::cout);
    }

    // 4. Output diagnostics
    reporter.printReport(std::cout);

    return reporter.hasErrors() ? 1 : 0;
}
