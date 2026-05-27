#include "lexer.h"

Lexer::Lexer() : _identifierStr(""), _numVal(0), _lastChar(' ') {
}

int Lexer::gettok() {
    
    // Skip any whitespace.
    while (isspace(_lastChar))
    {
        _lastChar = getchar();
    }

    // identifier: [a-zA-Z][a-zA-Z0-9]*
    if (isalpha(_lastChar)) {

        _identifierStr = _lastChar;

        while (isalnum(_lastChar = getchar()))
        {
            _identifierStr += _lastChar;
        }

        if (_identifierStr == "def") {
            return tok_def;
        }

        if (_identifierStr == "extern") {
            return tok_extern;
        }

        return tok_identifier;
    }
    
    // Number: [0-9.]+
    if (isdigit(_lastChar) || _lastChar == '.') {
        std::string numStr;
        do {
            numStr += _lastChar;
            _lastChar = getchar();
        } while (isdigit(_lastChar) || _lastChar == '.');

        _numVal = strtod(numStr.c_str(), 0);
        return tok_number;
    }

    if (_lastChar == '#') {
        // Comment until end of line.
        do {
            _lastChar = getchar();
        } while(_lastChar != EOF && _lastChar != '\n' && _lastChar != '\r');

        if (_lastChar != EOF) {
            return gettok();
        }
    }

    // Check for end of file.  Don't eat the EOF.
    if (_lastChar == EOF) {
        return tok_eof;
    }

    // Otherwise, just return the character as its ascii value.
    int thisChar = _lastChar;
    _lastChar = getchar();
    return thisChar;
}