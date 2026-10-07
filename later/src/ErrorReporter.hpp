#pragma once

#include <string>
#include <vector>
#include <iostream>

struct Diagnostic {
    enum class Severity { Error, Warning };
    Severity severity;
    int line;
    int column;
    std::string message;
};

class ErrorReporter {
public:
    void reportError(int line, int col, const std::string& message) {
        diagnostics.push_back({Diagnostic::Severity::Error, line, col, message});
    }

    void reportWarning(int line, int col, const std::string& message) {
        diagnostics.push_back({Diagnostic::Severity::Warning, line, col, message});
    }

    bool hasErrors() const {
        for (const auto& d : diagnostics) {
            if (d.severity == Diagnostic::Severity::Error) return true;
        }
        return false;
    }

    int errorCount() const {
        int count = 0;
        for (const auto& d : diagnostics) {
            if (d.severity == Diagnostic::Severity::Error) count++;
        }
        return count;
    }

    int warningCount() const {
        int count = 0;
        for (const auto& d : diagnostics) {
            if (d.severity == Diagnostic::Severity::Warning) count++;
        }
        return count;
    }

    const std::vector<Diagnostic>& getDiagnostics() const {
        return diagnostics;
    }

    void printReport(std::ostream& out = std::cerr) const {
        for (const auto& d : diagnostics) {
            if (d.severity == Diagnostic::Severity::Error) {
                out << "\033[1;31m[ERROR]\033[0m line " << d.line << ", col " << d.column << ": " << d.message << "\n";
            } else {
                out << "\033[1;33m[WARNING]\033[0m line " << d.line << ", col " << d.column << ": " << d.message << "\n";
            }
        }
        if (hasErrors()) {
            out << "\nSemantic analysis failed with " << errorCount() << " error(s).\n";
        } else {
            out << "\nSemantic analysis passed successfully with 0 errors.\n";
        }
    }

    void clear() {
        diagnostics.clear();
    }

private:
    std::vector<Diagnostic> diagnostics;
};
