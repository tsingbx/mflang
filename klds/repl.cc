#include "parser.h"
#include "repl.h"

void startREPL() {
    InitializeModuleAndPassManager();
    fprintf(stderr, "ready> ");
    Parser p;
    p.getNextToken();
    p.MainLoop();
    PrintModule();
}