#include <stdlib.h>
#include <string.h>

#include "focl.h"
#include "sys_lean.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef MEMORY_ALLOC_CHECK
extern size_t focl_malloced_;
extern size_t focl_realloced_;
#endif

#define FOCLRC_FILENAME ".foclrc"

void* Focl_realloc(void* ptr, size_t size);

int execfoclrc(Focl_Context* ctx)
{
    char* rcpath = Focl_GetHomeDirectory();
    if (rcpath == NULL)
    {
        return 0;
    }
    
    size_t lenOfPath = strlen(rcpath);
    size_t newSize = lenOfPath + 1 + sizeof(FOCLRC_FILENAME);
    rcpath = Focl_realloc(rcpath, newSize);
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
    int exitCode = 0;
    bool shouldExecFoclrc = true;
    bool shouldEnterREPL = true;
    char* fileToOpen = NULL;
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "--norc") == 0)
            {
                shouldExecFoclrc = false;
            }
            else if (strcmp(argv[i], "-c") == 0)
            {
                if (argc > i + 1)
                {
                    i++;
                    FoclObjectRelease(Focl_eval(ctx, argv[i]), ctx);
                    shouldEnterREPL = false;
                    fileToOpen = NULL;
                }
                else
                {
                    printf("Cannot get content from command line.\n");
                    shouldEnterREPL = false;
                    exitCode = 1;
                }
            }
            else
            {
                fileToOpen = argv[i];
                shouldEnterREPL = false;
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

    if (shouldEnterREPL) /* REPL */
    {
    #ifdef MEMORY_ALLOC_CHECK
        printf("Malloced:%lld Bytes, Realloced:%lld Bytes\n", focl_malloced_, focl_realloced_);
    #endif
        printf("Focl REPL\n");
        printf("Type \"exit\" to quit.\n");
        exitCode = Focl_REPL(ctx);
    }
    else if (fileToOpen != NULL) /* exec file */
    {
        exitCode = Focl_ExecFile(ctx, fileToOpen);
    }
    else
    {
        ; /* this is very ugly! */
    }
    freeFoclContext(ctx);
    return exitCode;
}