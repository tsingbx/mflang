#ifndef AST_H
#define AST_H
#include <type_traits>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <llvm/IR/Value.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/Module.h"

using namespace llvm;

extern void InitializeModule();
extern void PrintModule();

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual Value* codegen() = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
    double _val;

public:
    NumberExprAST(double val) : _val(val) {}
    Value *codegen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
    std::string _name;

public:
    VariableExprAST(const std::string &name) : _name(name) {}
    Value *codegen() override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    char _op;
    std::unique_ptr<ExprAST> _lhs, _rhs;

public:
  BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs,
                std::unique_ptr<ExprAST> rhs);
  Value *codegen() override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
    std::string _callee;
    std::vector<std::unique_ptr<ExprAST>> _args;

public:
    CallExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST>> args)
    : _callee(callee), _args(std::move(args)) {}
    Value *codegen() override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
    std::string _name;
    std::vector<std::string> _args;

public:
    PrototypeAST(const std::string &name, std::vector<std::string> args)
    : _name(name), _args(std::move(args)) {}

    const std::string &getName() const { return _name; }
    Function* codegen();
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
    std::unique_ptr<PrototypeAST> _proto;
    std::unique_ptr<ExprAST> _body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::unique_ptr<ExprAST> body)
    : _proto(std::move(proto)), _body(std::move(body)) {}
    Function *codegen();
};

extern std::unique_ptr<ExprAST> LogError(const char *str);

extern std::unique_ptr<PrototypeAST> LogErrorP(const char* str);

extern Value *LogErrorV(const char *Str);

#endif