#include "ast.h"
#include "parser.h"

Parser::Parser()
{
    _lexer = std::make_unique<Lexer>();
}

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
int Parser::getTokPrecedence()
{
    if (!isascii(_curTok))
    {
        return -1;
    }

    // Make sure it's a declared binop
    switch (_curTok)
    {
    case '<':
    case '>':
        return 10;
    case '+':
    case '-':
        return 20;
    case '*':
    case '/':
        return 40;
    default:
        return -1;
    }
}
int Parser::getNextToken()
{
    return _curTok = _lexer->gettok();
}

std::unique_ptr<ExprAST> Parser::ParseNumberExpr()
{
    auto result = std::make_unique<NumberExprAST>(_lexer->numVal());
    // eat the number
    getNextToken();
    return std::move(result);
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> Parser::ParseParenExpr()
{
    // eat (
    getNextToken();
    auto v = ParseExpression();
    if (!v)
    {
        return nullptr;
    }
    if (_curTok != ')')
    {
        return LogError("expected ')'");
    }
    // eat )
    getNextToken();
    return v;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
std::unique_ptr<ExprAST> Parser::ParseIdentifierExpr()
{
    std::string idName = _lexer->identifierStr();
    // eat identifier
    getNextToken();

    if (_curTok != '(')
    {
        // simple variable ref
        return std::make_unique<VariableExprAST>(idName);
    }

    // eat (
    getNextToken();
    // call
    std::vector<std::unique_ptr<ExprAST>> args;
    if (_curTok != ')')
    {
        while (true)
        {
            if (auto arg = ParseExpression())
            {
                args.push_back(std::move(arg));
            }
            else
            {
                return nullptr;
            }

            if (_curTok == ')')
            {
                break;
            }

            if (_curTok != ',')
            {
                return LogError("Expected ')' or ',' in argument list");
            }

            getNextToken();
        }
    }

    // eat ')'
    getNextToken();

    return std::make_unique<CallExprAST>(idName, std::move(args));
}

std::unique_ptr<ExprAST> Parser::ParsePrimary()
{
    switch (_curTok)
    {
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    default:
        return LogError("unknown token when expecting an expression");
    }
}

std::unique_ptr<ExprAST> Parser::ParseBinOpRHS(int exprPrec, std::unique_ptr<ExprAST> lhs)
{
    // If this is a binop, find its precedence.
    while (true)
    {
        int tokPrec = getTokPrecedence();
        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (tokPrec < exprPrec)
        {
            return lhs;
        }
        // Okay, we know this is a binop.
        int binop = _curTok;
        // eat binop
        getNextToken();
        // Parse the primary expression after the binary operator.
        auto rhs = ParsePrimary();
        if (!rhs)
        {
            return nullptr;
        }
        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int nextPrec = getTokPrecedence();
        if (tokPrec < nextPrec)
        {
            rhs = ParseBinOpRHS(tokPrec + 1, std::move(rhs));
            if (!rhs)
            {
                return nullptr;
            }
        }
        // Merge LHS/RHS.
        lhs = std::make_unique<BinaryExprAST>(
            binop, std::move(lhs), std::move(rhs));
    } // loop around to the top of the while loop.
}

std::unique_ptr<ExprAST> Parser::ParseExpression()
{
    auto lhs = ParsePrimary();
    if (!lhs)
    {
        return nullptr;
    }
    return ParseBinOpRHS(0, std::move(lhs));
}

/// prototype
///   ::= id '(' id* ')'
std::unique_ptr<PrototypeAST> Parser::ParsePrototype()
{
    if (_curTok != tok_identifier)
    {
        return LogErrorP("Expected function name in prototype");
    }

    std::string fnName = _lexer->identifierStr();
    getNextToken();

    if (_curTok != '(')
    {
        return LogErrorP("Expected '(' in prototype");
    }

    // Read the list of argument names
    std::vector<std::string> argNames;
    while (getNextToken() == tok_identifier)
    {
        argNames.push_back(_lexer->identifierStr());
    }
    if (_curTok != ')')
    {
        return LogErrorP("Expected ')' in prototype");
    }

    // success
    getNextToken(); // eat ')'

    return std::make_unique<PrototypeAST>(fnName, std::move(argNames));
}

/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> Parser::ParseDefinition()
{
    getNextToken(); // eat def
    auto proto = ParsePrototype();
    if (!proto)
    {
        return nullptr;
    }
    if (auto e = ParseExpression())
    {
        return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
    }
    return nullptr;
}

/// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> Parser::ParseTopLevelExpr()
{
    if (auto e = ParseExpression())
    {
        // Make an anonymous proto
        auto proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
    }
    return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::ParseExtern()
{
    getNextToken(); // eat extern
    return ParsePrototype();
}

/// top ::= definition | external | expression | ';'
void Parser::MainLoop()
{
    while (true)
    {
        if (_curTok != tok_eof) {
            fprintf(stderr, "ready> ");
        }
        switch (_curTok)
        {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons
            getNextToken();
            break;
        case tok_def:
            if (auto fnAST = ParseDefinition())
            {
                if (auto *fnIR = fnAST->codegen())
                {
                    fprintf(stderr, "Read function definition:\n");
                    fnIR->print(errs());
                    fprintf(stderr, "\n");
                }
            }
            else
            {
                // Skip token for error recovery.
                getNextToken();
            }
            break;
        case tok_extern:
            if (auto protoAST = ParseExtern())
            {
                if (auto *fnIR = protoAST->codegen())
                {
                    fprintf(stderr, "Read extern: ");
                    fnIR->print(errs());
                    fprintf(stderr, "\n");
                }
            }
            else
            {
                // Skip token for error recovery.
                getNextToken();
            }
            break;
        default:
            // Evaluate a top-level expression into an anonymous function.
            if (auto fnAST = ParseTopLevelExpr())
            {
                if (auto *fnIR = fnAST->codegen())
                {
                    fprintf(stderr, "Read top-level expression:");
                    fnIR->print(errs());
                    fprintf(stderr, "\n");

                    // Remove the anonymous expression.
                    fnIR->eraseFromParent();
                }
            }
            else
            {
                // Skip token for error recovery.
                getNextToken();
            }
            break;
        }
    }
}