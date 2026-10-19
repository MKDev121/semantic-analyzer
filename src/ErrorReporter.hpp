#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

enum class DiagnosticPhase { Lexical, Syntax, Semantic };

struct Diagnostic {
    enum class Severity { Error, Warning };
    Severity severity;
    DiagnosticPhase phase;
    int line;
    int column;
    std::string message;
    std::string correction;
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

    void reportError(int line, int column, const std::string& message,
                     const std::string& correction = "") {
        diagnostics.push_back({Diagnostic::Severity::Error, phase, line, column, message, correction});
    }

    void reportWarning(int line, int column, const std::string& message,
                       const std::string& correction = "") {
        diagnostics.push_back({Diagnostic::Severity::Warning, phase, line, column, message, correction});
    }

    bool hasErrors() const { return errorCount() != 0; }

    int errorCount() const {
        int count = 0;
        for (const auto& diagnostic : diagnostics) {
            if (diagnostic.severity == Diagnostic::Severity::Error) ++count;
        }
        return count;
    }

    int warningCount() const {
        int count = 0;
        for (const auto& diagnostic : diagnostics) {
            if (diagnostic.severity == Diagnostic::Severity::Warning) ++count;
        }
        return count;
    }

    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics; }

    void printReport(std::ostream& output = std::cerr) const {
        for (const auto& diagnostic : diagnostics) {
            output << '\n' << '[' << phaseLabel(diagnostic.phase) << ' '
                   << (diagnostic.severity == Diagnostic::Severity::Error ? "ERROR" : "WARNING")
                   << "] line " << diagnostic.line << ", col " << diagnostic.column << '\n';
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
            output << "Possible correction: "
                   << (diagnostic.correction.empty() ? defaultCorrection(diagnostic.phase) : diagnostic.correction)
                   << '\n';
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

    static const char* defaultCorrection(DiagnosticPhase value) {
        switch (value) {
            case DiagnosticPhase::Lexical: return "Use a supported token or correctly terminated literal/comment.";
            case DiagnosticPhase::Syntax: return "Correct the indicated statement to follow the language grammar.";
            case DiagnosticPhase::Semantic: return "Use declarations and operand types compatible with the rule described above.";
        }
        return "Correct the indicated source code and run the analyzer again.";
    }
};
