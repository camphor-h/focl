#include "focl.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    Focl_Context* ctx = createFoclContext(stdout);
    Focl_RegisterBuiltinCommands(ctx);
    int exitCode;
    if (argc > 1)
    {
        exitCode = Focl_ExecFile(ctx, argv[1]);
    }
    else
    {
        printf("Focl REPL\n");
        printf("Type \"exit\" to quit.\n");
        exitCode = Focl_REPL(ctx);
    }
    freeFoclContext(ctx);
    return exitCode;
}