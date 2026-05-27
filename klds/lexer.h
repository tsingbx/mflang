#ifndef LEXER_H
#define LEXER_H
#include <string>

enum Token {
    tok_eof = -1,
    
    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5,
};

class Lexer {
    int _lastChar;
    // Filled in if tok_identifier
    std::string _identifierStr;
    // Filled in if tok_number
    double _numVal; 
private:
public:
    Lexer();
    int gettok();
    const std::string& identifierStr() const {
        return _identifierStr;
    }
    const double numVal() const {
        return _numVal;
    }
};

#endif