#include "ErrorReporter.hpp"
#include "Lexer.hpp"
#include "LexicalValidation.hpp"
#include "Parser.hpp"
#include "SemanticAnalyzer.hpp"
#include "SymbolTable.hpp"

#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options] [source-file]\n"
              << "Explainable Semantic Analyzer with Scope and Definite-Assignment Checking.\n"
              << "Omit source-file or use '-' to read from standard input.\n\n"
              << "Options:\n"
              << "  --dump-tokens     Show token categories and source positions\n"
              << "  --dump-ast        Show the AST before semantic analysis\n"
              << "  --dump-typed-ast  Show the AST with inferred expression types\n"
              << "  --dump-symbols    Show declarations in their lexical scopes\n"
              << "  --all             Show tokens, basic AST, typed AST, and symbols\n"
              << "  -h, --help        Show this help\n"
              << "  --                Treat following arguments as file names\n";
}

void printTokens(const std::vector<Token>& tokens) {
    std::cout << "\n=============================== TOKENS ===============================\n"
              << std::left << std::setw(8) << "Line" << std::setw(8) << "Column"
              << std::setw(20) << "Token" << "Lexeme\n";
    for (const auto& token : tokens) {
        std::cout << std::left << std::setw(8) << token.line << std::setw(8) << token.column
                  << std::setw(20) << Token::typeName(token.type)
                  << (token.type == TokenType::TOK_EOF ? "<end>" : token.text) << '\n';
    }
    std::cout << "Tokens (excluding EOF): " << (tokens.empty() ? 0 : tokens.size() - 1) << '\n';
}

void reportUnknownTokens(const std::vector<Token>& tokens, ErrorReporter& reporter) {
    for (const auto& token : tokens) {
        if (token.type != TokenType::TOK_UNKNOWN) continue;
        std::string correction = "Remove '" + token.text + "' if accidental, or replace it with a supported token.";
        if (token.text == "&") correction = "Use '&&' if you intended logical AND.";
        else if (token.text == "|") correction = "Use '||' if you intended logical OR.";
        reporter.reportError(token.line, token.column,
                             "Unrecognized character '" + token.text + "' is not a supported token in this language",
                             correction);
    }
}
} // namespace

int main(int argc, char* argv[]) {
    bool dumpTokens = false;
    bool dumpAST = false;
    bool dumpTypedAST = false;
    bool dumpSymbols = false;
    bool dumpAll = false;
    bool optionsEnded = false;
    bool haveInput = false;
    std::string filename;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (!optionsEnded && (argument == "--help" || argument == "-h")) {
            printUsage(argv[0]);
            return 0;
        }
        if (!optionsEnded && argument == "--") { optionsEnded = true; continue; }
        if (!optionsEnded && argument == "--dump-tokens") { dumpTokens = true; continue; }
        if (!optionsEnded && argument == "--dump-ast") { dumpAST = true; continue; }
        if (!optionsEnded && argument == "--dump-typed-ast") { dumpTypedAST = true; continue; }
        if (!optionsEnded && argument == "--dump-symbols") { dumpSymbols = true; continue; }
        if (!optionsEnded && argument == "--all") {
            dumpAll = true;
            continue;
        }
        if (!optionsEnded && argument.size() > 1 && argument.front() == '-') {
            std::cerr << "Unknown option: " << argument << '\n';
            printUsage(argv[0]);
            return 1;
        }
        if (haveInput) {
            std::cerr << "Provide exactly one source file per run.\n";
            return 1;
        }
        haveInput = true;
        filename = argument;
    }

    if (dumpAll) {
        dumpTokens = dumpAST = dumpTypedAST = dumpSymbols = true;
    }

    std::string source;
    if (!haveInput || filename == "-") {
        std::ostringstream input;
        input << std::cin.rdbuf();
        if (std::cin.bad()) {
            std::cerr << "Could not read source from standard input.\n";
            return 1;
        }
        source = input.str();
        filename = "<stdin>";
    } else {
        std::ifstream input(filename, std::ios::binary);
        if (!input) {
            std::cerr << "Could not open file: " << filename << '\n';
            return 1;
        }
        std::ostringstream buffer;
        buffer << input.rdbuf();
        if (input.bad()) {
            std::cerr << "Could not completely read file: " << filename << '\n';
            return 1;
        }
        source = buffer.str();
    }
    // A UTF-8 BOM is file metadata rather than part of a source token.
    if (source.compare(0, 3, "\xEF\xBB\xBF") == 0) source.erase(0, 3);

    std::cout << "Explainable Semantic Analyzer\n"
              << "Input: " << filename << '\n';
    ErrorReporter reporter;
    reporter.setSource(source);
    reporter.setPhase(DiagnosticPhase::Lexical);

    try {
        if (!validateLexicalStructure(source, reporter)) {
            reporter.printReport(std::cout);
            return 1;
        }
        Lexer lexer(source);
        const auto tokens = lexer.tokenize();
        if (dumpTokens) printTokens(tokens);
        reportUnknownTokens(tokens, reporter);
        if (reporter.hasErrors()) {
            reporter.printReport(std::cout);
            return 1;
        }

        reporter.setPhase(DiagnosticPhase::Syntax);
        Parser parser(tokens, reporter);
        auto ast = parser.parseProgram();
        if (reporter.hasErrors()) {
            reporter.printReport(std::cout);
            return 1;
        }
        if (tokens.size() == 1) {
            std::cout << "NOTICE => Empty compilation unit: no declarations to analyze.\n";
        }
        if (dumpAST) {
            std::cout << "\n============================== INITIAL AST ==============================\n";
            ast->print(std::cout);
        }

        reporter.setPhase(DiagnosticPhase::Semantic);
        SymbolTable symbols;
        SemanticAnalyzer analyzer(symbols, reporter);
        analyzer.analyze(ast.get());
        if (dumpTypedAST) {
            std::cout << "\n=============================== TYPED AST ===============================\n";
            ast->print(std::cout);
        }
        if (dumpSymbols) {
            std::cout << '\n';
            symbols.printHierarchy(std::cout);
        }
        reporter.printReport(std::cout);
        return reporter.hasErrors() ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "Analyzer could not complete: " << error.what() << '\n';
        return 2;
    }
}
