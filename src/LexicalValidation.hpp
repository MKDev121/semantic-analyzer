#pragma once

#include "ErrorReporter.hpp"
#include <string>

// Additional 75% checks keep the original 25% Lexer files unchanged.
bool validateLexicalStructure(const std::string& source, ErrorReporter& reporter);
