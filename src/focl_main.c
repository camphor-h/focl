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

#ifdef NEED_PROFILE
#define FOCLRC_FILENAME ".foclrc"
#endif

void* Focl_realloc(void* ptr, size_t size);
void Focl_free(void* ptr);

#ifdef NEED_PROFILE
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
    Focl_free(rcpath);
    return foclrcExecCode;
}
#endif

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    
    int exitCode = 0;
#ifdef NEED_PROFILE
    bool shouldExecFoclrc = true;
#endif

    bool shouldEnterREPL = true;
    char* fileToOpen = NULL;
    int filePtrIdx = -1;
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            if (fileToOpen == NULL)
            {
                if (strcmp(argv[i], "--norc") == 0)
                {
                #ifdef NEED_PROFILE
                    shouldExecFoclrc = false;
                #endif
                }
                else if (strcmp(argv[i], "-c") == 0)
                {
                    if (argc > i + 1)
                    {
                        i++;
                        Focl_Context* ctx = createFoclContext(stdout, argc - i, argv + i);
                        Focl_RegisterBuiltinCommands(ctx);
                        /* no free function, because it will exit soon. */
                        return Focl_evalWithExitCode(ctx, argv[i]);;
                    }
                    else
                    {
                        printf("Cannot get content from command line.\n");
                        shouldEnterREPL = false;
                        exitCode = 1;
                    }
                }
                else if (strcmp(argv[i], "-v") == 0)
                {
                    shouldEnterREPL = false;
                    fileToOpen = NULL;
                    printf("Fast Optimized Command Language\nBuild time:" __DATE__", "__TIME__ "\n");
                    exitCode = 0;
                }
                else
                {
                    fileToOpen = argv[i];
                    shouldEnterREPL = false;
                    filePtrIdx = i;
                }
            }
        }
    }
    int startOfArgv = (filePtrIdx > 0) ? filePtrIdx : 1;
    Focl_Context* ctx = createFoclContext(stdout, argc - startOfArgv, argv + startOfArgv);
    Focl_RegisterBuiltinCommands(ctx);
#ifdef NEED_PROFILE
    if (shouldExecFoclrc)
    {
        exitCode = execfoclrc(ctx);
        if (exitCode != 0)
        {
            freeFoclContext(ctx);
            return exitCode;
        }
    }
#endif

    if (shouldEnterREPL) /* REPL */
    {
    #ifdef MEMORY_ALLOC_CHECK
        printf("Malloced:%zu Bytes, Realloced:%zu Bytes\n", focl_malloced_, focl_realloced_);
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