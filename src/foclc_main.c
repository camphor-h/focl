#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include "sys_lean.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
int64_t Focl_getpid()
{
    return GetCurrentProcessId();
}
#else
#include <sys/types.h>
#include <unistd.h>
int64_t Focl_getpid()
{
    return getpid();
}
#endif

#define COMPILE_DEBUG_OPTION_DEF "-g"
#define COMPILE_RELEASE_OPTION_DEF "-O2 -flto"

#define HEAD_OF_COMPILE_CONTENT_DEF \
    "#include <stdio.h> \n" \
    "typedef struct Focl_Object Focl_Object;\n" \
    "typedef struct Focl_Context Focl_Context;\n" \
    "Focl_Context* createFoclContext(FILE* outpotfPtr);\n" \
    "void Focl_RegisterBuiltinCommands(Focl_Context* context);\n" \
    "void freeFoclContext(Focl_Context* context);\n" \
    "void FoclObjectRelease(Focl_Object* obj, Focl_Context* context);\n" \
    "Focl_Object* Focl_eval(Focl_Context* context, const char* Cstr);\n" \
    "int main(int argc, char* argv[])\n" \
    "{\n" \
    "   Focl_Context* ctx = createFoclContext(stdout);\n" \
    "   Focl_RegisterBuiltinCommands(ctx);\n" \
    "   FoclObjectRelease(Focl_eval(ctx, \""

#define TAIL_OF_COMPILE_CONTENT_DEF \
    "\"), ctx);\n" \
    "   freeFoclContext(ctx);\n" \
    "   return 0;\n" \
    "}"

const char COMPILE_DEBUG_OPTION[] = COMPILE_DEBUG_OPTION_DEF;
const char COMPILE_RELEASE_OPTION[] = COMPILE_RELEASE_OPTION_DEF;
const char HEAD_OF_COMPILE_CONTENT[] = HEAD_OF_COMPILE_CONTENT_DEF;
const char TAIL_OF_COMPILE_CONTENT[] = TAIL_OF_COMPILE_CONTENT_DEF;

char* Focl_strdup(const char* src);

int lenOfNum(int64_t n)
{
    int len = 0;
    do
    {
        len++;
        n /= 10;
    }
    while (n);
    return len;
}

char* escapeCString(const char* src, size_t len)
{
    char* dst = (char*)malloc(len * 6 + 1);
    if (!dst)
    {
        return NULL;
    }

    char* p = dst;
    for (size_t i = 0; i < len; i++)
    {
        unsigned char c = src[i];
        switch (c)
        {
            case '\n':
                *p++ = '\\';
                *p++ = 'n';
                break;
            case '\r':
                *p++ = '\\';
                *p++ = 'r';
                break;
            case '\t':
                *p++ = '\\';
                *p++ = 't';
                break;
            case '"':
                *p++ = '\\';
                *p++ = '"';
                break;
            case '\\':
                *p++ = '\\';
                *p++ = '\\';
                break;
            case '\b':
                *p++ = '\\';
                *p++ = 'b';
                break;
            case '\f':
                *p++ = '\\';
                *p++ = 'f';
                break;
            default:
                if (c < 0x20 || c >= 0x7F)
                {
                    sprintf(p, "\\%03o", c);
                    p += 4;
                }
                else
                {
                    *p++ = c;
                }
                break;
        }
    }
    *p = '\0';
    return dst;
}

char* createTempFileName()
{
#ifdef _WIN32
    char tempName[MAX_PATH];
    char* tempDir = ".";
    UINT unique = GetTempFileName(tempDir, "focl", 0, tempName);
    if (unique == 0)
    {
        return NULL;
    }
    return Focl_strdup(tempName);
#else
    char template[] = "/tmp/foclc_XXXXXX";
    int fd = mkstemp(template);
    if (fd == -1)
    {
        return NULL;
    }
    close(fd);
    return Focl_strdup(template);
#endif
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    char* compileContent = NULL;
    char* fileToCompile = NULL;
    char* outputFileName = NULL;
    bool isOutputFileNameAlloced = false;
    const char* compileOption = COMPILE_RELEASE_OPTION;
    bool keepSourceOnly = false;

    if (argc < 2)
    {
        printf("Usage: %s <source.focl> [-o output] [-d|-r] [-c]\n", argv[0]);
        printf("  -d: debug mode (%s)\n", COMPILE_DEBUG_OPTION);
        printf("  -r: release mode (%s)\n", COMPILE_RELEASE_OPTION);
        printf("  -c: generate C source file only (do not compile)\n");
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0)
        {
            if (i + 1 < argc)
            {
                i++;
                outputFileName = argv[i];
            }
            else
            {
                printf("Error: Missing output file name after -o\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            compileOption = COMPILE_DEBUG_OPTION;
        }
        else if (strcmp(argv[i], "-r") == 0)
        {
            compileOption = COMPILE_RELEASE_OPTION;
        }
        else if (strcmp(argv[i], "-c") == 0)
        {
            keepSourceOnly = true;
        }
        else
        {
            if (fileToCompile == NULL)
            {
                fileToCompile = argv[i];
            }
            else
            {
                printf("Error: Unknown argument: %s\n", argv[i]);
                return 1;
            }
        }
    }

    if (fileToCompile == NULL)
    {
        printf("Error: No source file specified\n");
        return 1;
    }

    if (!keepSourceOnly)
    {
        if (Focl_isFileExist("libfocl.a") == false)
        {
            printf("Error: \"libfocl.a\" must be in the working directory.\n");
            return 1;
        }
    }

    if (outputFileName == NULL)
    {
        outputFileName = Focl_GetFileNameWithoutExt(fileToCompile);
        if (outputFileName == NULL)
        {
            printf("Error: Cannot generate output file name automatically.\n");
            return 1;
        }
        isOutputFileNameAlloced = true;
    }

    if (strcmp(fileToCompile, outputFileName) == 0)
    {
        printf("Error: Source file and output file cannot have the same name.\n");
        if (isOutputFileNameAlloced) free(outputFileName);
        return 1;
    }

    FILE* srcFile = fopen(fileToCompile, "rb");
    int retValue = 0;

    if (srcFile == NULL)
    {
        printf("Error: Cannot open source file: %s\n", fileToCompile);
        retValue = 1;
    }
    else
    {
        fseek(srcFile, 0, SEEK_END);
        long fileSize = ftell(srcFile);
        fseek(srcFile, 0, SEEK_SET);

        if (fileSize < 0)
        {
            printf("Error: Cannot get source file size.\n");
            retValue = 1;
        }
        else
        {
            char* sourceContent = (char*)malloc(fileSize + 1);
            if (sourceContent == NULL)
            {
                printf("Error: Memory allocation failed for source content.\n");
                retValue = 1;
            }
            else
            {
                size_t readSize = fread(sourceContent, 1, fileSize, srcFile);
                sourceContent[readSize] = '\0';

                char* escapedContent = escapeCString(sourceContent, readSize);
                free(sourceContent);

                if (escapedContent == NULL)
                {
                    printf("Error: Failed to escape source content.\n");
                    retValue = 1;
                }
                else
                {
                    size_t headLen = strlen(HEAD_OF_COMPILE_CONTENT);
                    size_t tailLen = strlen(TAIL_OF_COMPILE_CONTENT);
                    size_t escapedLen = strlen(escapedContent);
                    size_t totalLen = headLen + escapedLen + tailLen + 1;

                    compileContent = (char*)malloc(totalLen);
                    if (compileContent == NULL)
                    {
                        printf("Error: Memory allocation failed.\n");
                        retValue = 1;
                    }
                    else
                    {
                        char* ptr = compileContent;

                        memcpy(ptr, HEAD_OF_COMPILE_CONTENT, headLen);
                        ptr += headLen;

                        memcpy(ptr, escapedContent, escapedLen);
                        ptr += escapedLen;

                        memcpy(ptr, TAIL_OF_COMPILE_CONTENT, tailLen);
                        ptr += tailLen;
                        *ptr = '\0';

                        char* tempFile = createTempFileName();
                        if (tempFile == NULL)
                        {
                            printf("Error: Cannot create temporary file.\n");
                            retValue = 1;
                        }
                        else
                        {
                            if (keepSourceOnly)
                            {
                                FILE* tempFp = fopen(tempFile, "w");
                                if (tempFp == NULL)
                                {
                                    printf("Error: Cannot open temporary file: %s\n", tempFile);
                                    free(tempFile);
                                    retValue = 1;
                                }
                                else
                                {
                                    fwrite(compileContent, 1, strlen(compileContent), tempFp);
                                    fclose(tempFp);
                                    char outputCFile[4096];
                                    snprintf(outputCFile, sizeof(outputCFile), "%s.c", outputFileName);
                                    tempFp = fopen(tempFile, "r");
                                    if (tempFp == NULL)
                                    {
                                        printf("Error: Cannot open temporary file for reading: %s\n", tempFile);
                                        free(tempFile);
                                        retValue = 1;
                                    }
                                    else
                                    {
                                        FILE* outFp = fopen(outputCFile, "w");
                                        if (outFp == NULL)
                                        {
                                            printf("Error: Cannot create output C file: %s\n", outputCFile);
                                            fclose(tempFp);
                                            free(tempFile);
                                            retValue = 1;
                                        }
                                        else
                                        {
                                            char buffer[8192];
                                            size_t bytes;
                                            while ((bytes = fread(buffer, 1, sizeof(buffer), tempFp)) > 0)
                                            {
                                                fwrite(buffer, 1, bytes, outFp);
                                            }
                                            fclose(outFp);
                                            fclose(tempFp);
                                            
                                            printf("Generated C source file: %s\n", outputCFile);
                                        #ifdef _WIN32
                                            char delCmd[4096];
                                            snprintf(delCmd, sizeof(delCmd), "del \"%s\"", tempFile);
                                            system(delCmd);
                                        #else
                                            remove(tempFile);
                                        #endif
                                        }
                                    }
                                    free(tempFile);
                                }
                            }
                            else
                            {
                                // 写入临时文件
                                FILE* tempFp = fopen(tempFile, "w");
                                if (tempFp == NULL)
                                {
                                    printf("Error: Cannot open temporary file: %s\n", tempFile);
                                    free(tempFile);
                                    retValue = 1;
                                }
                                else
                                {
                                    fwrite(compileContent, 1, strlen(compileContent), tempFp);
                                    fclose(tempFp);

                                    // 编译
                                    char cmdBuffer[4096];
                                    snprintf(cmdBuffer, sizeof(cmdBuffer),
#ifdef _WIN32
                                        "cc -x c %s \"%s\" -o \"%s\" -L. -lfocl && del \"%s\"",
#else
                                        "cc -x c %s \"%s\" -o \"%s\" -L. -lfocl && rm -f \"%s\"",
#endif
                                        compileOption, tempFile, outputFileName, tempFile);

                                    printf("Compiling...\n");
                                    printf("Command: %s\n", cmdBuffer);

                                    int systemRet = system(cmdBuffer);
                                    if (systemRet == 0)
                                    {
                                        printf("Compilation successful! Output: %s\n", outputFileName);
                                    }
                                    else
                                    {
                                        printf("Compilation failed with error code: %d\n", systemRet);
                                        retValue = 1;
                                    }

                                    free(tempFile);
                                }
                            }
                        }

                        free(compileContent);
                    }

                    free(escapedContent);
                }
            }
        }

        fclose(srcFile);
    }

    if (isOutputFileNameAlloced)
    {
        free(outputFileName);
    }

    return retValue;
}