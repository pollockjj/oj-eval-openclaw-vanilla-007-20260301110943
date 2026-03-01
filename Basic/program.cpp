#include "program.hpp"

#include "statement.hpp"

Program::Program() = default;

Program::~Program() {
    clear();
}

void Program::clear() {
    for (auto &entry: lines) {
        delete entry.second.stmt;
        entry.second.stmt = nullptr;
    }
    lines.clear();
    resetExecutionState();
}

void Program::addSourceLine(int lineNumber, const std::string &line) {
    auto it = lines.find(lineNumber);
    if (it != lines.end()) {
        delete it->second.stmt;
        it->second.stmt = nullptr;
        it->second.source = line;
        return;
    }
    lines[lineNumber] = ProgramLine{line, nullptr};
}

void Program::removeSourceLine(int lineNumber) {
    auto it = lines.find(lineNumber);
    if (it == lines.end()) return;
    delete it->second.stmt;
    lines.erase(it);
}

std::string Program::getSourceLine(int lineNumber) {
    auto it = lines.find(lineNumber);
    if (it == lines.end()) return "";
    return it->second.source;
}

void Program::setParsedStatement(int lineNumber, Statement *stmt) {
    auto it = lines.find(lineNumber);
    if (it == lines.end()) {
        delete stmt;
        error("LINE NUMBER ERROR");
    }
    delete it->second.stmt;
    it->second.stmt = stmt;
}

Statement *Program::getParsedStatement(int lineNumber) {
    auto it = lines.find(lineNumber);
    if (it == lines.end()) return nullptr;
    return it->second.stmt;
}

int Program::getFirstLineNumber() {
    if (lines.empty()) return -1;
    return lines.begin()->first;
}

int Program::getNextLineNumber(int lineNumber) {
    auto it = lines.upper_bound(lineNumber);
    if (it == lines.end()) return -1;
    return it->first;
}

bool Program::containsLine(int lineNumber) const {
    return lines.find(lineNumber) != lines.end();
}

void Program::resetExecutionState() {
    jumpTarget = -1;
    endFlag = false;
}

void Program::setJumpTarget(int lineNumber) {
    jumpTarget = lineNumber;
}

int Program::getJumpTarget() const {
    return jumpTarget;
}

void Program::setEndFlag(bool ended) {
    endFlag = ended;
}

bool Program::getEndFlag() const {
    return endFlag;
}
