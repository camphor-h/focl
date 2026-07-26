/* Currently I have no motivation to implement readline on my system.
 * Let's just use linenoise. I'm sorry Salvatore Sanfilippo. :( */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "focl_dev.h"
#include "linenoise.h"

int Focl_REPL(Focl_Context* ctx)
{
    Focl_String buffer;
    FoclStringOpCt(&buffer, FOCL_STRING_INIT_CAPACITY);
    int depth = 0;
    ctx->hasExitBuf = true;
    if (setjmp(ctx->exitBuf) != 0)
    {
        FoclStringOpDt(&buffer);
        ctx->hasExitBuf = false;
        return ctx->exitCode;
    }

    while (1)
    {
        if (depth > 0)
        {
            for (int d = depth; d > 0; d--)
            {
                putchar('.');
            }
            putchar(' ');
        }
        else
        {
            printf("> ");
        }
        fflush(stdout);

        size_t lineLen = 0;
    #ifdef _WIN32
        char* input = Focl_getline(stdin, &lineLen);
    #else
        char* input = linenoise("");
    #endif
        if (input == NULL)
        {
            printf("\n");
            break;
        }
        if (lineLen == 0 && depth == 0)
        {
        #ifdef _WIN32
            free(input);
        #else
            linenoiseHistoryAdd(input);
            linenoiseFree(input);
        #endif
            continue;
        }
        if (buffer.length > 0)
        {
            FoclStrAppend(&buffer, "\n");
        }
        FoclStrAppend(&buffer, input);
        depth += focl_countBraceDepth(input);
        if (depth < 0)
        {
            depth = 0;
        }
    #ifdef _WIN32
        free(input);
    #else
        linenoiseHistoryAdd(input);
        linenoiseFree(input);
    #endif

        if (depth > 0)
        {
            continue;
        }
        Focl_Object* result = Focl_parseLine(ctx, &buffer);

        if (result->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclIOBufferPrintf(ctx->outBuffer, "Error: %s\n", FoclStrCStr(result->as.data));
        }
        else if (result->type != FOCL_OBJ_TYPE_VOID)
        {
            FoclObjectPrint(result, ctx->outBuffer);
            FoclIOBufferPutChar(ctx->outBuffer, '\n');
        }

        FoclObjectRelease(result, ctx);
        FoclStrClear(&buffer);
        depth = 0;
    }

    ctx->hasExitBuf = false;
    FoclStringOpDt(&buffer);
    return ctx->exitCode;
}