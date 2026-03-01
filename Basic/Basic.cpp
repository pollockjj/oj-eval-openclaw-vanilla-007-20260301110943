#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>

#include "evalstate.hpp"
#include "program.hpp"
#include "statement.hpp"
#include "Utils/error.hpp"
#include "Utils/strlib.hpp"
#include "Utils/tokenScanner.hpp"

namespace {
bool isStrictIntegerToken(const std::string &token) {
    if (token.empty()) return false;
    int i = 0;
    if (token[0] == '-') {
        if (token.size() == 1) return false;
        i = 1;
    }
    for (; i < static_cast<int>(token.size()); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(token[i]))) return false;
    }
    return true;
}

void runProgram(Program &program, EvalState &state) {
    (void) state;
    program.resetExecutionState();

    int lineNumber = program.getFirstLineNumber();
    while (lineNumber != -1) {
        Statement *stmt = program.getParsedStatement(lineNumber);
        if (stmt == nullptr) {
            lineNumber = program.getNextLineNumber(lineNumber);
            continue;
        }

        program.setJumpTarget(-1);
        stmt->execute(state, program);

        if (program.getEndFlag()) break;

        int jumpLine = program.getJumpTarget();
        if (jumpLine != -1) {
            if (!program.containsLine(jumpLine)) error("LINE NUMBER ERROR");
            lineNumber = jumpLine;
        } else {
            lineNumber = program.getNextLineNumber(lineNumber);
        }
    }
}

void listProgram(Program &program) {
    int line = program.getFirstLineNumber();
    while (line != -1) {
        std::cout << program.getSourceLine(line) << std::endl;
        line = program.getNextLineNumber(line);
    }
}

void processLine(std::string line, Program &program, EvalState &state) {
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(line);

    if (!scanner.hasMoreTokens()) return;

    std::string first = scanner.nextToken();

    // Program line (line number first)
    if (isStrictIntegerToken(first) && !first.empty() && std::isdigit(static_cast<unsigned char>(first[0]))) {
        int lineNumber = stringToInteger(first);
        if (!scanner.hasMoreTokens()) {
            program.removeSourceLine(lineNumber);
            return;
        }

        std::string stmtText;
        while (scanner.hasMoreTokens()) {
            if (!stmtText.empty()) stmtText += " ";
            stmtText += scanner.nextToken();
        }

        Statement *stmt = parseStatement(stmtText);
        program.addSourceLine(lineNumber, line);
        program.setParsedStatement(lineNumber, stmt);
        return;
    }

    scanner.saveToken(first);
    std::string cmd = toUpperCase(scanner.nextToken());

    if (cmd == "RUN") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        runProgram(program, state);
        return;
    }
    if (cmd == "LIST") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        listProgram(program);
        return;
    }
    if (cmd == "CLEAR") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        program.clear();
        state.Clear();
        return;
    }
    if (cmd == "QUIT") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        std::exit(0);
    }
    if (cmd == "HELP") {
        return;
    }

    std::unique_ptr<Statement> stmt(parseStatement(line));
    program.setEndFlag(false);
    program.setJumpTarget(-1);
    stmt->execute(state, program);
}
} // namespace

int main() {
    EvalState state;
    Program program;

    while (true) {
        try {
            std::string input;
            if (!std::getline(std::cin, input)) break;
            if (input.empty()) continue;
            processLine(input, program, state);
        } catch (ErrorException &ex) {
            std::cout << ex.getMessage() << std::endl;
        }
    }
    return 0;
}
