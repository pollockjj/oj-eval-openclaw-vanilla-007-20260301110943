#include "statement.hpp"

#include <cctype>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include "exp.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "Utils/error.hpp"
#include "Utils/strlib.hpp"
#include "Utils/tokenScanner.hpp"

namespace {
const std::set<std::string> kKeywords = {
        "REM", "LET", "PRINT", "INPUT", "END", "GOTO", "IF", "THEN",
        "RUN", "LIST", "CLEAR", "QUIT", "HELP"
};

bool isValidVariableName(const std::string &name) {
    if (name.empty()) return false;
    if (kKeywords.count(toUpperCase(name)) != 0) return false;
    for (char ch: name) {
        if (!std::isalnum(static_cast<unsigned char>(ch))) return false;
    }
    return true;
}

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

int parseIntOrSyntaxError(const std::string &token) {
    if (!isStrictIntegerToken(token)) error("SYNTAX ERROR");
    try {
        return stringToInteger(token);
    } catch (ErrorException &) {
        error("SYNTAX ERROR");
    }
    return 0;
}

Expression *parseExpressionOrSyntaxError(const std::string &exprText) {
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exprText);
    try {
        return parseExp(scanner);
    } catch (ErrorException &) {
        error("SYNTAX ERROR");
    }
    return nullptr;
}

std::string readRemaining(TokenScanner &scanner) {
    std::string out;
    while (scanner.hasMoreTokens()) {
        if (!out.empty()) out += " ";
        out += scanner.nextToken();
    }
    return out;
}

} // namespace

Statement::Statement() = default;
Statement::~Statement() = default;

RemStatement::RemStatement() = default;
void RemStatement::execute(EvalState &state, Program &program) {
    (void) state;
    (void) program;
}

LetStatement::LetStatement(Expression *exp) : exp(exp) {}
LetStatement::~LetStatement() { delete exp; }
void LetStatement::execute(EvalState &state, Program &program) {
    (void) program;
    exp->eval(state);
}

PrintStatement::PrintStatement(Expression *exp) : exp(exp) {}
PrintStatement::~PrintStatement() { delete exp; }
void PrintStatement::execute(EvalState &state, Program &program) {
    (void) program;
    std::cout << exp->eval(state) << std::endl;
}

InputStatement::InputStatement(std::string var) : var(std::move(var)) {}
void InputStatement::execute(EvalState &state, Program &program) {
    (void) program;
    while (true) {
        std::cout << " ? ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            exit(0);
        }
        std::string token = trim(line);
        if (isStrictIntegerToken(token)) {
            state.setValue(var, stringToInteger(token));
            break;
        }
        std::cout << "INVALID NUMBER" << std::endl;
    }
}

EndStatement::EndStatement() = default;
void EndStatement::execute(EvalState &state, Program &program) {
    (void) state;
    program.setEndFlag(true);
}

GotoStatement::GotoStatement(int targetLine) : targetLine(targetLine) {}
void GotoStatement::execute(EvalState &state, Program &program) {
    (void) state;
    program.setJumpTarget(targetLine);
}

IfStatement::IfStatement(Expression *lhs, std::string op, Expression *rhs, int targetLine)
        : lhs(lhs), op(std::move(op)), rhs(rhs), targetLine(targetLine) {}
IfStatement::~IfStatement() {
    delete lhs;
    delete rhs;
}
void IfStatement::execute(EvalState &state, Program &program) {
    int l = lhs->eval(state);
    int r = rhs->eval(state);
    bool ok = false;
    if (op == "=") ok = (l == r);
    else if (op == "<") ok = (l < r);
    else if (op == ">") ok = (l > r);
    else error("SYNTAX ERROR");

    if (ok) program.setJumpTarget(targetLine);
}

Statement *parseStatement(const std::string &line) {
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(line);

    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");

    const std::string cmd = toUpperCase(scanner.nextToken());

    if (cmd == "REM") {
        return new RemStatement();
    }

    if (cmd == "LET") {
        const std::string exprText = readRemaining(scanner);
        if (exprText.empty()) error("SYNTAX ERROR");
        Expression *exp = parseExpressionOrSyntaxError(exprText);
        if (exp->getType() != COMPOUND ||
            static_cast<CompoundExp *>(exp)->getOp() != "=" ||
            static_cast<CompoundExp *>(exp)->getLHS()->getType() != IDENTIFIER) {
            delete exp;
            error("SYNTAX ERROR");
        }
        const std::string var = static_cast<IdentifierExp *>(static_cast<CompoundExp *>(exp)->getLHS())->getName();
        if (!isValidVariableName(var)) {
            delete exp;
            error("SYNTAX ERROR");
        }
        return new LetStatement(exp);
    }

    if (cmd == "PRINT") {
        const std::string exprText = readRemaining(scanner);
        if (exprText.empty()) error("SYNTAX ERROR");
        return new PrintStatement(parseExpressionOrSyntaxError(exprText));
    }

    if (cmd == "INPUT") {
        if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
        const std::string var = scanner.nextToken();
        if (scanner.hasMoreTokens() || !isValidVariableName(var)) error("SYNTAX ERROR");
        return new InputStatement(var);
    }

    if (cmd == "END") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        return new EndStatement();
    }

    if (cmd == "GOTO") {
        if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
        const std::string lineToken = scanner.nextToken();
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        return new GotoStatement(parseIntOrSyntaxError(lineToken));
    }

    if (cmd == "IF") {
        std::vector<std::string> tokens;
        while (scanner.hasMoreTokens()) tokens.push_back(scanner.nextToken());
        int thenPos = -1;
        for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
            if (toUpperCase(tokens[i]) == "THEN") {
                thenPos = i;
                break;
            }
        }
        if (thenPos <= 1 || thenPos >= static_cast<int>(tokens.size()) - 1) error("SYNTAX ERROR");

        int relPos = -1;
        for (int i = 0; i < thenPos; ++i) {
            if (tokens[i] == "=" || tokens[i] == "<" || tokens[i] == ">") {
                relPos = i;
                break;
            }
        }
        if (relPos <= 0 || relPos >= thenPos - 1) error("SYNTAX ERROR");

        std::string lhsText, rhsText;
        for (int i = 0; i < relPos; ++i) {
            if (!lhsText.empty()) lhsText += " ";
            lhsText += tokens[i];
        }
        for (int i = relPos + 1; i < thenPos; ++i) {
            if (!rhsText.empty()) rhsText += " ";
            rhsText += tokens[i];
        }
        std::string targetText;
        for (int i = thenPos + 1; i < static_cast<int>(tokens.size()); ++i) {
            if (!targetText.empty()) targetText += " ";
            targetText += tokens[i];
        }
        int target = parseIntOrSyntaxError(targetText);

        std::unique_ptr<Expression> lhs(parseExpressionOrSyntaxError(lhsText));
        std::unique_ptr<Expression> rhs(parseExpressionOrSyntaxError(rhsText));
        return new IfStatement(lhs.release(), tokens[relPos], rhs.release(), target);
    }

    error("SYNTAX ERROR");
    return nullptr;
}
