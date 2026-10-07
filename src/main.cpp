#include "Lexer.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

void explainLexicalError(const Token& token, const vector<string>& lines) {
    cout << "\n[ERROR] line " << token.line << ", col " << token.column << ": unrecognized character '" << token.text << "'\n";

    if (token.line > 0 && static_cast<size_t>(token.line) <= lines.size()) {
        const auto& line = lines[token.line - 1];
        cout << "    " << line << "\n    ";

        for (int col = 1; col < token.column; ++col) {
            const size_t index = static_cast<size_t>(col - 1);
            cout << (index < line.size() && line[index] == '\t' ? '\t' : ' ');
        }

        cout << "^\n";
    }

    cout << "Reason: '" << token.text << "' is not a supported token in this language.\n";

    if (token.text == "&") {
        cout << "Possible correction: Use '&&' if you intended logical AND.\n";
    } else if (token.text == "|") {
        cout << "Possible correction: Use '||' if you intended logical OR.\n";
    } else {
        cout << "Possible correction: Remove '" << token.text << "' if accidental, or replace it with the intended supported token.\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc == 2 && (string(argv[1]) == "--help" || string(argv[1]) == "-h")) {
        cout << "Usage: lexer_demo <source-file>\n";
        cout << "Milestone 1: print tokens and their source positions.\n";
        return 0;
    }

    if (argc != 2) {
        cerr << "Usage: lexer_demo <source-file>\n";
        return 1;
    }

    ifstream file(argv[1], ios::binary | ios::ate);

    if (!file) {
        cerr << "Could not open file: " << argv[1] << '\n';
        return 1;
    }

    const auto fileSize = file.tellg();

    if (fileSize < 0) {
        cerr << "Could not determine file size: " << argv[1] << '\n';
        return 1;
    }

    file.seekg(0, ios::beg);

    if (!file) {
        cerr << "Could not seek to the beginning of file: " << argv[1] << '\n';
        return 1;
    }

    string sourceText(static_cast<size_t>(fileSize), '\0');

    if (!sourceText.empty() && !file.read(sourceText.data(), static_cast<streamsize>(sourceText.size()))) {
        cerr << "Could not read file: " << argv[1] << '\n';
        cerr << "Expected " << sourceText.size() << " bytes, read " << file.gcount() << ".\n";
        return 1;
    }

    vector<string> lines;
    istringstream sourceLines(sourceText);
    string line;

    while (getline(sourceLines, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        lines.push_back(line);
    }

    Lexer lexer(sourceText);
    const auto tokens = lexer.tokenize();

    cout << "Milestone 1: Lexical analysis\n";
    cout << "Input: " << argv[1] << '\n';
    cout << "Bytes read: " << sourceText.size() << '\n';

    if (sourceText.empty()) {
        cout << "[NOTICE] The input file is empty; only EOF will be shown.\n";
        cout << "Save source code in this file, then run the command again.\n";
    } else if (tokens.size() == 1) {
        cout << "[NOTICE] No tokens were produced after skipping whitespace and comments.\n";
    }

    cout << left << setw(8) << "Line" << setw(8) << "Column" << setw(20) << "Token" << "Lexeme\n";

    int errors = 0;

    for (const auto& token : tokens) {
        cout << left << setw(8) << token.line << setw(8) << token.column << setw(20) << Token::typeName(token.type) << (token.type == TokenType::TOK_EOF ? "<end>" : token.text) << '\n';
    }

    for (const auto& token : tokens) {
        if (token.type == TokenType::TOK_UNKNOWN) {
            ++errors;
            explainLexicalError(token, lines);
        }
    }

    cout << "\nTokens (excluding EOF): " << tokens.size() - 1 << '\n';
    cout << "Lexical errors: " << errors << '\n';
    cout << "Parsing and semantic analysis are deferred to later milestones.\n";

    return errors == 0 ? 0 : 1;
}
