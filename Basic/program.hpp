/*
 * File: program.h
 * ---------------
 * This interface exports a Program class for storing a BASIC
 * program.
 */

#ifndef _program_h
#define _program_h

#include <map>
#include <string>
#include "Utils/error.hpp"

class Statement;

class Program {
public:
    Program();
    ~Program();

    void clear();
    void addSourceLine(int lineNumber, const std::string &line);
    void removeSourceLine(int lineNumber);
    std::string getSourceLine(int lineNumber);

    void setParsedStatement(int lineNumber, Statement *stmt);
    Statement *getParsedStatement(int lineNumber);

    int getFirstLineNumber();
    int getNextLineNumber(int lineNumber);

    bool containsLine(int lineNumber) const;

    // Runtime control for RUN
    void resetExecutionState();
    void setJumpTarget(int lineNumber);
    int getJumpTarget() const;
    void setEndFlag(bool ended);
    bool getEndFlag() const;

private:
    struct ProgramLine {
        std::string source;
        Statement *stmt = nullptr;
    };

    std::map<int, ProgramLine> lines;
    int jumpTarget = -1;
    bool endFlag = false;
};

#endif
