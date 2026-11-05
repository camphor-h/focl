#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include "sys_lean.h"
#include "focl.h"

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
#define COMPILE_RELEASE_OPTION_DEF "-O2 -s"

#define HEAD_OF_COMPILE_CONTENT_DEF \
    "#include <stdio.h>\n" \
    "#include <stddef.h>\n" \
    "#ifdef _WIN32\n" \
    "#include <windows.h>\n" \
    "#endif\n" \
    "typedef struct Focl_Object Focl_Object;\n" \
    "typedef struct Focl_Context Focl_Context;\n" \
    "Focl_Context* createFoclContext(FILE* outpotfPtr, int argc, char** argv);\n" \
    "void Focl_RegisterBuiltinCommands(Focl_Context* context);\n" \
    "void freeFoclContext(Focl_Context* context);\n" \
    "void FoclObjectRelease(Focl_Object* obj, Focl_Context* context);\n" \
    "Focl_Object* Focl_evalBytecode(Focl_Context* context, const void* data, size_t size);\n" \
    "static const unsigned char foclBytecode[] = {"

#define TAIL_OF_COMPILE_CONTENT_DEF \
    "};\n" \
    "int main(int argc, char* argv[])\n" \
    "{\n" \
    "#ifdef _WIN32\n" \
    "   SetConsoleCP(65001);\n" \
    "   SetConsoleOutputCP(65001);\n" \
    "#endif\n"\
    "   Focl_Context* ctx = createFoclContext(stdout, argc, argv);\n" \
    "   Focl_RegisterBuiltinCommands(ctx);\n" \
    "   FoclObjectRelease(Focl_evalBytecode(ctx, foclBytecode, sizeof(foclBytecode)), ctx);\n" \
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

char* bytecodeToCArray(const unsigned char* data, size_t size)
{
    size_t capacity = size * 6 + size / 16 + 4;
    char* dst = (char*)malloc(capacity);
    if (!dst)
    {
        return NULL;
    }

    size_t pos = 0;
    for (size_t i = 0; i < size; i++)
    {
        if (i % 16 == 0)
        {
            dst[pos++] = '\n';
        }
        int written = snprintf(dst + pos, capacity - pos, "0x%02x,", data[i]);
        if (written < 0 || (size_t)written >= capacity - pos)
        {
            free(dst);
            return NULL;
        }
        pos += (size_t)written;
    }
    dst[pos++] = '\n';
    dst[pos] = '\0';
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
    char libDir[PATH_MAX] = {0};

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
        else if (strcmp(argv[i], "-v") == 0)
        {
            printf("foclc %s\n", Focl_GetVersionStr());
            return 0;
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
        bool libFound = false;
        char libPath[PATH_MAX];
        Focl_GetCurrentExecFilePath(libDir, PATH_MAX);
        snprintf(libPath, sizeof(libPath), "%slibfocl.a", libDir);
        if (Focl_isFileExist(libPath))
        {
            libFound = true;
        }
    #ifndef _WIN32
        else if (Focl_isFileExist("/usr/lib/focl/libfocl.a"))
        {
            snprintf(libDir, sizeof(libDir), "/usr/lib/focl/");
            libFound = true;
        }
        else if (Focl_isFileExist("/usr/local/lib/focl/libfocl.a"))
        {
            snprintf(libDir, sizeof(libDir), "/usr/local/lib/focl/");
            libFound = true;
        }
    #endif
        if (!libFound)
        {
            printf("Error: \"libfocl.a\" must be in the searching directory.\n");
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

                Focl_Context* compileCtx = createFoclContext(stdout, 0, NULL);
                Focl_Chunk* chunk = Focl_CompileSource(compileCtx, sourceContent);
                Focl_Bytecode bytecode = Focl_ChunkSerialize(chunk);
                Focl_ChunkFree(chunk);
                freeFoclContext(compileCtx);
                free(sourceContent);

                char* bytecodeText = NULL;
                if (bytecode.data != NULL)
                {
                    bytecodeText = bytecodeToCArray(bytecode.data, bytecode.size);
                }
                Focl_BytecodeFree(&bytecode);

                if (bytecodeText == NULL)
                {
                    printf("Error: Failed to compile source to bytecode.\n");
                    retValue = 1;
                }
                else
                {
                    size_t headLen = strlen(HEAD_OF_COMPILE_CONTENT);
                    size_t tailLen = strlen(TAIL_OF_COMPILE_CONTENT);
                    size_t bytecodeLen = strlen(bytecodeText);
                    size_t totalLen = headLen + bytecodeLen + tailLen + 1;

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

                        memcpy(ptr, bytecodeText, bytecodeLen);
                        ptr += bytecodeLen;

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
                                    remove(tempFile);
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
                                        remove(tempFile);
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
                                            remove(tempFile);
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
                                            remove(tempFile);
                                        }
                                    }
                                    free(tempFile);
                                }
                            }
                            else
                            {
                                FILE* tempFp = fopen(tempFile, "w");
                                if (tempFp == NULL)
                                {
                                    printf("Error: Cannot open temporary file: %s\n", tempFile);
                                    remove(tempFile);
                                    free(tempFile);
                                    retValue = 1;
                                }
                                else
                                {
                                    fwrite(compileContent, 1, strlen(compileContent), tempFp);
                                    fclose(tempFp);

                                    char cmdBuffer[4096];
                                    snprintf(cmdBuffer, sizeof(cmdBuffer),
                                        "cc -x c %s \"%s\" -o \"%s\" -x none \"%slibfocl.a\"",
                                        compileOption, tempFile, outputFileName, libDir);

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

                                    remove(tempFile);
                                    free(tempFile);
                                }
                            }
                        }

                        free(compileContent);
                    }

                    free(bytecodeText);
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