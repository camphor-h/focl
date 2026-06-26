#include "focl.h"
int main(int argc, char* argv[])
{
    Focl_Context* ctx = createFoclContext();
    Focl_RegisterBuiltinCommands(ctx);
    int exitCode;
    if (argc > 1)
    {
        exitCode = Focl_ExecFile(ctx, argv[1]);
    }
    else
    {
        exitCode = Focl_REPL(ctx);
    }
    freeFoclContext(ctx);
    return exitCode;
}