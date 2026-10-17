#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

enum class DiagnosticPhase { Lexical, Syntax, Semantic };

struct Diagnostic {
    DiagnosticPhase phase;
    int line;
    int column;
    std::string message;
};

// Both parser and semantic analysis use one source-aware diagnostic collector.
class ErrorReporter {
public:
    void setSource(const std::string& source) {
        sourceLines.clear();
        std::istringstream input(source);
        std::string line;
        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            sourceLines.push_back(std::move(line));
        }
        // Keep a line available for EOF diagnostics, including empty input.
        if (sourceLines.empty() || (!source.empty() && source.back() == '\n')) {
            sourceLines.emplace_back();
        }
    }

    void setPhase(DiagnosticPhase value) { phase = value; }

    void reportError(int line, int column, const std::string& message) {
        diagnostics.push_back({phase, line, column, message});
    }

    bool hasErrors() const { return errorCount() != 0; }

    int errorCount() const { return static_cast<int>(diagnostics.size()); }

    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics; }

    void printReport(std::ostream& output = std::cerr) const {
        for (const auto& diagnostic : diagnostics) {
            output << '\n' << '[' << phaseLabel(diagnostic.phase)
                   << " ERROR] line " << diagnostic.line << ", col " << diagnostic.column << '\n';
            if (diagnostic.line > 0 && static_cast<std::size_t>(diagnostic.line) <= sourceLines.size()) {
                const auto& line = sourceLines[static_cast<std::size_t>(diagnostic.line - 1)];
                output << "    " << line << "\n    ";
                for (int column = 1; column < diagnostic.column; ++column) {
                    const auto index = static_cast<std::size_t>(column - 1);
                    output << (index < line.size() && line[index] == '\t' ? '\t' : ' ');
                }
                output << "^\n";
            }
            output << "Reason: " << diagnostic.message << '\n';
        }
        output << '\n' << analysisLabel(phase)
               << (hasErrors() ? " failed with " : " passed successfully with ")
               << errorCount() << " error(s).\n";
    }

    void clear() { diagnostics.clear(); }

private:
    DiagnosticPhase phase = DiagnosticPhase::Semantic;
    std::vector<Diagnostic> diagnostics;
    std::vector<std::string> sourceLines;

    static const char* phaseLabel(DiagnosticPhase value) {
        switch (value) {
            case DiagnosticPhase::Lexical: return "LEXICAL";
            case DiagnosticPhase::Syntax: return "SYNTAX";
            case DiagnosticPhase::Semantic: return "SEMANTIC";
        }
        return "SEMANTIC";
    }

    static const char* analysisLabel(DiagnosticPhase value) {
        switch (value) {
            case DiagnosticPhase::Lexical: return "Lexical analysis";
            case DiagnosticPhase::Syntax: return "Syntax analysis";
            case DiagnosticPhase::Semantic: return "Semantic analysis";
        }
        return "Semantic analysis";
    }

};
