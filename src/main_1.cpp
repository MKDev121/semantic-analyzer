#include "Lexer.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

void explainLexicalError(const Token& token, const vector<string>& lines) {
    cout << "\nERROR => line " << token.line << ", col " << token.column << ": unrecognized character '" << token.text << "'\n"; //Error msg.

    if (token.line > 0 && static_cast<size_t>(token.line) <= lines.size()) {    //Check source line exists.
        const auto& line = lines[token.line - 1];   //Since line numbers start from 1. But we subtract to make it consistent with vector indices.

        cout << "    " << line << "\n    ";     //Go to new line.

        for (int col = 1; col < token.column; ++col) {
            const size_t index = static_cast<size_t>(col - 1);
            cout << (index < line.size() && line[index] == '\t' ? '\t' : ' ');      //The next loop prints spacing up to the error’s column.
        }

        cout << "^\n";  //Error arrow indication.
    }

    cout << "Reason: '" << token.text << "' is not a supported token in this language.\n";

    if (token.text == "&") {
        cout << "Possible correction: Use '&&' if you intended logical AND.\n"; //Reasons defined for various symbolic errors.
    } 
    else if (token.text == "|") {
        cout << "Possible correction: Use '||' if you intended logical OR.\n";  
    } 
    else {
        cout << "Possible correction: Remove '" << token.text << "' if accidental, or replace it with the intended supported token.\n";
    }
}

int main(int argc, char* argv[]) {       //argc is the number of command-line arguments. It includes the program’s name. argv contains those arguments as text. Each char* points to a C-style string.
    /*
    
        If you run:
        .\lexer_demo.exe .\tests\test1_declarations.txt
        Then:
        argc    = 2
        argv[0] = ".\lexer_demo.exe"
        argv[1] = ".\tests\test1_declarations.txt"
    
    */
    
    
    if (argc == 2 && (string(argv[1]) == "--help" || string(argv[1]) == "-h")) {
        cout << "Usage: lexer_demo <source-file>\n";        //Usage help.
        return 0;
    }

    if (argc != 2) {
        cerr << "Usage: lexer_demo <source-file>\n";    //Error if arguments not given properly.
        return 1;
    }

    ifstream file(argv[1], ios::binary | ios::ate); //input file stream with 1st argument being the file path supplied. Reads bytes and from the end of file.

    if (!file) {
        cerr << "Could not open file: " << argv[1] << '\n'; //Check whether opening succeeded.
        return 1;
    }

    const auto fileSize = file.tellg(); //Current reading position.
    if (fileSize < 0) {
        cerr << "Could not determine file size: " << argv[1] << '\n'; //Error position.
        return 1;
    }

    file.seekg(0, ios::beg);        //Move to beginning.

    if (!file) {
        cerr << "Could not seek to the beginning of file: " << argv[1] << '\n'; //Error.
        return 1;
    }

    string sourceText(static_cast<size_t>(fileSize), '\0'); //Create string large enough to hold content of file. By default create an null string of length equal to file size. 

    if (!sourceText.empty() && !file.read(sourceText.data(), static_cast<streamsize>(sourceText.size()))) {
        cerr << "Could not read file: " << argv[1] << '\n';            //Error reading file.
        cerr << "Expected " << sourceText.size() << " bytes, read " << file.gcount() << ".\n";
        return 1;
    }

    vector<string> lines;   //Empty lines of strings list.
    istringstream sourceLines(sourceText);  //Creates an input stream that reads from a string rather than a file. It lets us use reading functions on sourceText.
    string line;        //Temp line for current line.

    while (getline(sourceLines, line)) {    //Inputstream reading.
        if (!line.empty() && line.back() == '\r') { //Handles window line endings.
            line.pop_back();
        }

        lines.push_back(line);  //Appends until reading fails.
    }   

    Lexer lexer(sourceText);                //Creates a lexer with complete sourcetext.
    const auto tokens = lexer.tokenize();   //Collects all tokens.

    cout << "Lexical analysis\n";           //Basic info.
    cout << "Input: " << argv[1] << '\n';
    cout << "Bytes read: " << sourceText.size() << '\n';

    if (sourceText.empty()) { 
        cout << "NOTICE => The input file is empty; only EOF will be shown.\n";
        cout << "Save source code in this file, then run the command again.\n"; //If the file is empty, it prints an empty-file notice.
    } else if (tokens.size() == 1) {
        cout << "NOTICE => No tokens were produced after skipping whitespace and comments.\n";   //- If the file contains only spaces or comments, the lexer returns just one token: EOF.
    }

    cout << left << setw(8) << "Line" << setw(8) << "Column" << setw(20) << "Token" << "Lexeme\n";  //Token Table.

    int errors = 0;

    for (const auto& token : tokens) {
        cout << left << setw(8) << token.line << setw(8) << token.column << setw(20) << Token::typeName(token.type) << (token.type == TokenType::TOK_EOF ? "<end>" : token.text) << '\n';   //Print each token.
    }

    for (const auto& token : tokens) {
        if (token.type == TokenType::TOK_UNKNOWN) { //Error.
            ++errors;
            explainLexicalError(token, lines);      //Explain error function call.  
        }
    }

    cout << "\nTokens (excluding EOF): " << tokens.size() - 1 << '\n';  //Numer of tokens.
    cout << "Lexical errors: " << errors << '\n';       //Errors.
    cout << "Parsing and semantic analysis are deferred to later milestones.\n"; 

    return errors == 0 ? 0 : 1; //Successful / Unsuccessful exit.
}
