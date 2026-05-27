#include "parser.h"
#include "repl.h"

void startREPL() {
    InitializeModule();
    fprintf(stderr, "ready> ");
    Parser p;
    p.getNextToken();
    p.MainLoop();
    PrintModule();
}