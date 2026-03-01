/*
 * File: statement.h
 * -----------------
 * Statement hierarchy for BASIC commands.
 */

#ifndef _statement_h
#define _statement_h

#include <string>

#include "evalstate.hpp"

class Program;
class Expression;

class Statement {
public:
    Statement();
    virtual ~Statement();

    virtual void execute(EvalState &state, Program &program) = 0;
};

class RemStatement final : public Statement {
public:
    RemStatement();
    void execute(EvalState &state, Program &program) override;
};

class LetStatement final : public Statement {
public:
    explicit LetStatement(Expression *exp);
    ~LetStatement() override;
    void execute(EvalState &state, Program &program) override;

private:
    Expression *exp;
};

class PrintStatement final : public Statement {
public:
    explicit PrintStatement(Expression *exp);
    ~PrintStatement() override;
    void execute(EvalState &state, Program &program) override;

private:
    Expression *exp;
};

class InputStatement final : public Statement {
public:
    explicit InputStatement(std::string var);
    void execute(EvalState &state, Program &program) override;

private:
    std::string var;
};

class EndStatement final : public Statement {
public:
    EndStatement();
    void execute(EvalState &state, Program &program) override;
};

class GotoStatement final : public Statement {
public:
    explicit GotoStatement(int targetLine);
    void execute(EvalState &state, Program &program) override;

private:
    int targetLine;
};

class IfStatement final : public Statement {
public:
    IfStatement(Expression *lhs, std::string op, Expression *rhs, int targetLine);
    ~IfStatement() override;
    void execute(EvalState &state, Program &program) override;

private:
    Expression *lhs;
    std::string op;
    Expression *rhs;
    int targetLine;
};

Statement *parseStatement(const std::string &line);

#endif
