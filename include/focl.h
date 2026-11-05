#ifndef FOCL_H
#define FOCL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdbool.h>
#include "focl_version.h"

/* 
 * The aim of the file is to create a interface for outer program who wants
 * to embed Focl. The focl runtime don't lean on it
 */

typedef struct Focl_Vector Focl_Vector;
typedef struct Focl_Object Focl_Object;
typedef struct Focl_Context Focl_Context;
typedef struct Focl_Command Focl_Command;
typedef struct Focl_Chunk Focl_Chunk;

#ifndef FOCL_CONTROL_TYPES_DEFINED
#define FOCL_CONTROL_TYPES_DEFINED

/* Control-flow status carried on the normal return path. */
typedef enum
{
    FOCL_FLOW_OK = 0,
    FOCL_FLOW_ERROR,
    FOCL_FLOW_BREAK,
    FOCL_FLOW_CONTINUE,
    FOCL_FLOW_RETURN,
    FOCL_FLOW_EXIT
}Focl_Flow;

typedef struct Focl_Result
{
    Focl_Flow flow;
    Focl_Object* value;
    int exitCode;
}Focl_Result;

typedef struct Focl_Bytecode
{
    unsigned char* data;
    size_t size;
}Focl_Bytecode;

static inline Focl_Result FoclResultOk(Focl_Object* value)
{
    Focl_Result result = {FOCL_FLOW_OK, value, 0};
    return result;
}
static inline Focl_Result FoclResultError(Focl_Object* value)
{
    Focl_Result result = {FOCL_FLOW_ERROR, value, 0};
    return result;
}
static inline Focl_Result FoclResultFlow(Focl_Flow flow)
{
    Focl_Result result = {flow, NULL, 0};
    return result;
}
static inline Focl_Result FoclResultReturn(Focl_Object* value)
{
    Focl_Result result = {FOCL_FLOW_RETURN, value, 0};
    return result;
}
static inline Focl_Result FoclResultExit(int exitCode)
{
    Focl_Result result = {FOCL_FLOW_EXIT, NULL, exitCode};
    return result;
}
static inline bool FoclResultIsNormal(Focl_Result result)
{
    return (result.flow == FOCL_FLOW_OK);
}

#endif /* FOCL_CONTROL_TYPES_DEFINED */

typedef Focl_Result (*Focl_CommandFunc)(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd);

Focl_Context* createFoclContext(FILE* outpotfPtr, int argc, char** argv);
void freeFoclContext(Focl_Context* context);

/* Returns a static, null-terminated version string. Do not free it. */
char* Focl_GetVersionStr(void);
int Focl_REPL(Focl_Context* ctx);
int Focl_ExecFile(Focl_Context* ctx, const char* filename);
void FoclRegisterCommand(Focl_Context* context, const char* cmdName, Focl_CommandFunc func);
void Focl_RegisterBuiltinCommands(Focl_Context* context);
Focl_Object* Focl_eval(Focl_Context* context, const char* Cstr);
int Focl_evalWithExitCode(Focl_Context* context, const char* Cstr);
void FoclObjectRelease(Focl_Object* obj, Focl_Context* context);

/* Force-compile source into a reusable chunk. Useful for foclc and embedding. */
Focl_Chunk* Focl_CompileSource(Focl_Context* context, const char* source);
Focl_Result Focl_VMExecute(Focl_Context* context, Focl_Chunk* chunk);
void Focl_ChunkFree(Focl_Chunk* chunk);

/* Serialize a compiled chunk to a self-contained bytecode buffer. */
Focl_Bytecode Focl_ChunkSerialize(Focl_Chunk* chunk);
void Focl_BytecodeFree(Focl_Bytecode* bytecode);
Focl_Chunk* Focl_ChunkDeserialize(Focl_Context* context, const void* data, size_t size);

/* Compile, execute, release the chunk, and return the last value. */
Focl_Object* Focl_evalCompiled(Focl_Context* context, const char* source);

/* Deserialize and execute an embedded bytecode buffer. */
Focl_Object* Focl_evalBytecode(Focl_Context* context, const void* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif