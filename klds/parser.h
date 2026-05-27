#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
#include "ast.h"

class Parser {
    int _curTok;
    std::unique_ptr<Lexer> _lexer;
private:
    int getTokPrecedence();
public:
    Parser();
    int getNextToken();
    std::unique_ptr<ExprAST> ParseNumberExpr();
    std::unique_ptr<ExprAST> ParseParenExpr();
    std::unique_ptr<ExprAST> ParseIdentifierExpr();
    std::unique_ptr<ExprAST> ParsePrimary();
    std::unique_ptr<ExprAST> ParseExpression();
    std::unique_ptr<ExprAST> ParseBinOpRHS(int exprPrec, std::unique_ptr<ExprAST> lhs);
    std::unique_ptr<PrototypeAST> ParsePrototype();
    std::unique_ptr<FunctionAST> ParseDefinition();
    std::unique_ptr<PrototypeAST> ParseExtern();
    std::unique_ptr<FunctionAST> ParseTopLevelExpr();
    void MainLoop();
};

#endif