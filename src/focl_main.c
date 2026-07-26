#include <stdlib.h>
#include <string.h>

#include "focl.h"
#include "sys_lean.h"

#ifdef _WIN32
#include <windows.h>
#endif

#define FOCLRC_FILENAME ".foclrc"

int execfoclrc(Focl_Context* ctx)
{
    char* rcpath = Focl_GetHomeDirectory();
    if (rcpath == NULL)
    {
        return 0;
    }
    
    size_t lenOfPath = strlen(rcpath);
    size_t newSize = lenOfPath + 1 + sizeof(FOCLRC_FILENAME);
    rcpath = realloc(rcpath, newSize);
    if (rcpath == NULL)
    {
        return 0;
    }
    
#ifdef _WIN32
    rcpath[lenOfPath] = '\\';
#else
    rcpath[lenOfPath] = '/';
#endif
    memcpy(rcpath + lenOfPath + 1, FOCLRC_FILENAME, sizeof(FOCLRC_FILENAME));
    
    int foclrcExecCode = 0;
    if (Focl_isFileExist(rcpath))
    {
        foclrcExecCode = Focl_ExecFile(ctx, rcpath);
    }
    free(rcpath);
    return foclrcExecCode;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    Focl_Context* ctx = createFoclContext(stdout);
    Focl_RegisterBuiltinCommands(ctx);
    int exitCode;
    bool shouldExecFoclrc = true;
    char* fileToOpen = NULL; /* if it's NULL it will open REPL */
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "--norc") == 0)
            {
                shouldExecFoclrc = false;
            }
            else
            {
                fileToOpen = argv[i];
            }
        }
    }
    
    if (shouldExecFoclrc)
    {
        exitCode = execfoclrc(ctx);
        if (exitCode != 0)
        {
            freeFoclContext(ctx);
            return exitCode;
        }
    }

    if (fileToOpen == NULL) /* REPL */
    {
        printf("Focl REPL\n");
        printf("Type \"exit\" to quit.\n");
        exitCode = Focl_REPL(ctx);
    }
    else /* exec file */
    {
        exitCode = Focl_ExecFile(ctx, fileToOpen);
    }
    freeFoclContext(ctx);
    return exitCode;
}