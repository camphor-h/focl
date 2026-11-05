/* Currently I have no motivation to implement readline on my system.
 * Let's just use linenoise. I'm sorry Salvatore Sanfilippo. :( */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "focl_dev.h"
#include "linenoise.h"

#define RED_CTLPMT "\033[1;31m"
#define CLEAR_CTLPMT "\033[0m"
#define YELLOW_CTLPMT "\033[93m"

int Focl_REPL(Focl_Context* ctx)
{
    Focl_String* prompt = FoclStringPoolAlloc(ctx->strPool);
    Focl_String* buffer = FoclStringPoolAlloc(ctx->strPool);
    int depth = 0;
    int exitCode = 0;

#ifdef _WIN32
    char* input = NULL;
    size_t capacity = 0;
#endif

    while (1)
    {
        FoclStrClear(prompt);
        if (depth > 0)
        {
            for (int d = depth; d > 0; d--)
            {
                FoclStrAppend(prompt, ".", sizeof(".") - 1);
            }
            FoclStrAppend(prompt, " ", sizeof(" ") - 1);
        }
        else
        {
            FoclStrAppend(prompt, "> ", sizeof("> ") - 1);
        }

        size_t lineLen = 0;
    #ifdef _WIN32
        printf(FoclStrCStr(prompt));
        fflush(stdout);
        Focl_getline(stdin, &input, &lineLen, &capacity);
    #else
        char* input = linenoise(FoclStrCStr(prompt));
        lineLen = strlen(input);
    #endif
        if (input == NULL)
        {
            printf("\n");
            break;
        }
        if (lineLen == 0 && depth == 0)
        {
        #ifndef _WIN32
            linenoiseHistoryAdd(input);
            linenoiseFree(input);
        #endif
            continue;
        }
        if (buffer->length > 0)
        {
            FoclStrAppend(buffer, "\n", sizeof("\n") - 1);
        }
        FoclStrAppend(buffer, input, lineLen);
        depth += focl_countBraceDepth(input);
        if (depth < 0)
        {
            depth = 0;
        }
    #ifndef _WIN32
        linenoiseHistoryAdd(input);
        linenoiseFree(input);
    #endif

        if (depth > 0)
        {
            continue;
        }
        Focl_Result result = Focl_parseLine(ctx, buffer);

        if (result.flow == FOCL_FLOW_EXIT)
        {
            exitCode = result.exitCode;
            FoclResultRelease(&result, ctx);
            FoclStrClear(buffer);
            break;
        }

        const char* flowMsg = NULL;
        if (result.flow == FOCL_FLOW_BREAK)
        {
            flowMsg = "break outside loop";
        }
        else if (result.flow == FOCL_FLOW_CONTINUE)
        {
            flowMsg = "continue outside loop";
        }
        else if (result.flow == FOCL_FLOW_RETURN)
        {
            flowMsg = "return outside proc";
        }
        if (flowMsg != NULL)
        {
            FoclResultRelease(&result, ctx);
            result = FoclResultError(FoclObjectError(ctx->strObjPool, ctx->strPool, flowMsg));
        }

        if (result.flow == FOCL_FLOW_ERROR)
        {
            FoclIOBufferPrintf(ctx->outBuffer, RED_CTLPMT"Error:"CLEAR_CTLPMT" %s\n", FoclStrCStr(FoclObjectGetString(result.value)));
            FoclIOBufferPrintf(ctx->outBuffer, YELLOW_CTLPMT"Line:"CLEAR_CTLPMT" %s\n", FoclStrCStr(buffer));
        }
        else if (result.value->type != FOCL_OBJ_TYPE_VOID)
        {
            FoclObjectPrint(result.value, ctx->outBuffer, ctx->strPool);
            FoclIOBufferPutChar(ctx->outBuffer, '\n');
        }

        FoclResultRelease(&result, ctx);
        FoclStrClear(buffer);
        depth = 0;
    }
#ifdef _WIN32
    Focl_free(input);
#endif

    FoclStringPoolFree(prompt, ctx->strPool);
    return exitCode;
}