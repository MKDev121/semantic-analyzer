#pragma once

#include "ErrorReporter.hpp"
#include <string>

// Final-demo validation keeps the original shared Lexer files unchanged.
bool validateLexicalStructure(const std::string& source, ErrorReporter& reporter);
