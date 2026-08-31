#ifndef FOCL_DEV_H
#define FOCL_DEV_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>

#ifdef USE_F32_ON_64
#if SIZE_MAX != 0xFFFFFFFFFFFFFFFF
    #error "Not a 64-Bit platform. Don't define USE_F32_ON_64 macro."
#endif
#endif

#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF
    typedef uint32_t Focl_Obj_Type;
    typedef uint32_t Focl_Obj_RefCount;
#ifndef USE_F32_ON_64
    typedef double Focl_Obj_Float;
#else
    typedef float Focl_Obj_Float;
#endif
    typedef int64_t Focl_Obj_Int;
    #define FOCL_OBJ_INT_MAX ((1LL << 61) - 1)
    #define FOCL_FORMAT_INT PRId64
#ifndef USE_F32_ON_64
    #define FOCL_FORMAT_FLOAT "lf"
#else
    #define FOCL_FORMAT_FLOAT "f"
#endif
    #define FOCL_INT_TO_STR_TMP_BUFFER_SIZE 24
    #define FOCL_FLOAT_TO_STR_TMP_BUFFER_SIZE 32
    Focl_Obj_Int Focl_StrToInt(const char* str);
    Focl_Obj_Float Focl_StrToFloat(const char* str);
#elif SIZE_MAX == 0xFFFFFFFF
    typedef uint16_t Focl_Obj_Type;
    typedef uint16_t Focl_Obj_RefCount;
    typedef float Focl_Obj_Float;
    typedef int32_t Focl_Obj_Int;
    #define FOCL_OBJ_INT_MAX ((1 << 29) - 1)
    #define FOCL_FORMAT_INT PRId32
    #define FOCL_FORMAT_FLOAT "f"
    #define FOCL_INT_TO_STR_TMP_BUFFER_SIZE 12
    #define FOCL_FLOAT_TO_STR_TMP_BUFFER_SIZE 32
    Focl_Obj_Int Focl_StrToInt(const char* str);
    Focl_Obj_Float Focl_StrToFloat(const char* str);
#else
    #error "Unsupported word length platform. Though I want to see this program run in every platform. But now it couldn't run yours. Sorry. :("
#endif

#ifdef MEMORY_ALLOC_CHECK
extern size_t focl_malloced_;
extern size_t focl_realloced_;
extern size_t focl_freed_;
#endif

void* Focl_malloc(size_t size);
void* Focl_realloc(void* ptr, size_t size);
void* Focl_calloc(size_t itemCount, size_t itemSize);
void Focl_free(void* ptr);

typedef Focl_Obj_Int Focl_Obj_Bool;
#define FOCL_OBJ_TRUE 1
#define FOCL_OBJ_FALSE 0

#define FOCL_STRING_INIT_CAPACITY 64
#define FOCL_STRING_NPOS SIZE_MAX

typedef void* Focl_TypeOpCtCtx;
typedef void* Focl_TypeOpDtCtx;
typedef void* Focl_TypeOpClCtx;
typedef void (*Focl_TypeOpCtFunc)(void* ptThis, Focl_TypeOpCtCtx ctCtx_); /* Constructor */
typedef void (*Focl_TypeOpDtFunc)(void* ptThis, Focl_TypeOpDtCtx dtCtx_); /* Destructor */
typedef void (*Focl_TypeOpClFunc)(void* ptThis, Focl_TypeOpClCtx clCtx_); /* Clear function */

typedef struct Focl_TypeOpCt
{
    Focl_TypeOpCtCtx ctx;
    Focl_TypeOpCtFunc func;
}Focl_TypeOpCt;
typedef struct Focl_TypeDt
{
    Focl_TypeOpDtCtx ctx;
    Focl_TypeOpDtFunc func;
}Focl_TypeOpDt;
typedef struct Focl_TypeCl
{
    Focl_TypeOpClCtx ctx;
    Focl_TypeOpClFunc func;
}Focl_TypeOpCl;

int32_t getUtf8CodePointLength(uint8_t firstByte);

typedef struct Focl_String
{
    size_t capacity;
    size_t length;
    char* data;
}Focl_String;

int32_t FoclStrAt(size_t idx, char** start, size_t* hadSearchIdx);
int FoclStrComp(const Focl_String* str, const char* cStr);
int FoclStrCompStr(const Focl_String* str1, const Focl_String* str2);
void FoclStrClear(Focl_String* str);
void FoclStrAppend(Focl_String* str, const char* Cstr);
void FoclStrReserve(Focl_String* str, size_t newSize_);
void FoclStrAppendStr(Focl_String* dst, const Focl_String* src);
void FoclStrAssign(Focl_String* str, const char* cStr);
void FoclStrAssignStr(Focl_String* dst, const Focl_String* src);

void FoclStringOpCt(Focl_String* str, size_t iCapacity);
void FoclStringOpDt(Focl_String* str);

size_t FoclStrCharCount(const Focl_String* str);
bool StrKeyCompare(void* a, void* b);

bool Focl_isInteger(const char* str);
bool Focl_isFloat(const char* str);

typedef struct Focl_StringView
{
    size_t len;
    char* strPtr;
}Focl_StringView;

Focl_StringView FoclStringViewPeelBoth(Focl_StringView* strView);
int FoclStringViewComp(Focl_StringView* strView, const char* Cstr);
char initTempFoclStringWithView(Focl_String* tmpStr, Focl_StringView* strView); /* The tmpStr should be on stack! This function will return the savedPos char. */
void restoreFoclStringViewFromTempString(Focl_StringView* strView, char saved); /* The tmpStr should be on stack! */

void FoclStrAssignView(Focl_String* dst, const Focl_StringView* view);

#define FOCL_VECTOR_INIT_CAPACITY 16

typedef struct Focl_Vector
{
    size_t itemSize;
    size_t capacity;
    size_t size;
    void* data;
}Focl_Vector;

void FoclVectorPushBack(Focl_Vector* vec, void* data);
void FoclVectorPopBack(Focl_Vector* vec);
size_t FoclVectorGetSize(Focl_Vector* vec);
void FoclVectorAppendVector(Focl_Vector* dst, Focl_Vector* src);

typedef struct Focl_PoolBlock
{
    size_t itemCount;
    size_t itemSize;
    size_t freeTop;
    size_t* freeStack;
    void* data;
}Focl_PoolBlock;

typedef struct Focl_Pool
{
    size_t blockCount;
    size_t itemSize;
    size_t itemPerBlock;
    Focl_PoolBlock** blocks;
}Focl_Pool;

#define FOCL_STRING_POOL_ITEM_PER_BLOCK 32
#define FOCL_STRING_POOL_BLOCK_COUNT_INIT 4

typedef Focl_Pool Focl_StringPool;

Focl_String* FoclStringPoolAlloc(Focl_StringPool* strPool); /* It will alloc string and clear it. */
void FoclStringPoolFree(Focl_String* str, Focl_StringPool* strPool);

#define FOCL_VECTOR_POOL_ITEM_PER_BLOCK 32
#define FOCL_VECTOR_POOL_BLOCK_COUNT_INIT 2

typedef struct Focl_VectorPool
{
    size_t elementSize;
    Focl_Pool* pool;
}Focl_VectorPool;

#define FOCL_OBJ_POOL_ITEM_PER_BLOCK 16
#define FOCL_OBJ_POOL_BLOCK_COUNT_INIT 2

#define FOCL_OBJ_POOL_WITH_NO_STR_DEFAULT_TYPE FOCL_OBJ_TYPE_INT
#define FOCL_OBJ_POOL_WITH_STR_DEFAULT_TYPE FOCL_OBJ_TYPE_STR

typedef Focl_Pool Focl_ObjWithNoStrPool;
typedef Focl_Pool Focl_StrObjPool;
typedef Focl_Pool Focl_CmpdObjPool;

typedef Focl_Pool Focl_HashTablePool;

typedef Focl_HashTablePool Focl_ObjTablePool;
typedef Focl_HashTablePool Focl_CommandTablePool;

#define FOCL_HASH_TABLE_POOL_ITEM_PER_BLOCK 8
#define FOCL_HASH_TABLE_POOL_BLOCK_COUNT_INIT 2

typedef Focl_Pool Focl_EnvPool;

#define FOCL_ENV_POOL_ITEM_PER_BLOCK 4
#define FOCL_ENV_POOL_BLOCK_COUNT_INIT 2

typedef struct Focl_Object
{
    Focl_Obj_RefCount refCount;
    Focl_Obj_Type type;
    union
    {
        Focl_String* data;
        Focl_Vector* vec;
        Focl_Obj_Float f;
        Focl_Obj_Int i;
    }as;
}Focl_Object;

Focl_Obj_Int FoclObjectUnboxInt(Focl_Object* obj);
Focl_Obj_Float FoclObjectUnboxFloat(Focl_Object* obj);
void FoclObjectBoxInt(Focl_Object* obj, Focl_Obj_Int i_);
void FoclObjectBoxFloat(Focl_Object* obj, Focl_Obj_Float f_);
bool isFoclObjectUseString(Focl_Object* obj);
bool isFoclObjectCompound(Focl_Object* obj);
Focl_String* FoclObjectGetString(Focl_Object* obj);
Focl_Vector* FoclObjectGetVector(Focl_Object* obj);
void FoclObjectAssign(Focl_Object* dst, Focl_Object* src, Focl_StringPool* strPool, Focl_VectorPool* vecPool);

typedef size_t (*Focl_HashFunc)(void*);
typedef bool (*KeyCompareFunc)(void*, void*);

typedef Focl_TypeOpDt Focl_KeyOpDt;
typedef Focl_TypeOpDt Focl_ValueOpDt;

typedef struct Focl_HashTableUnit
{
    void* key;
    void* value;
    struct Focl_HashTableUnit* next;
}Focl_HashTableUnit;

typedef struct Focl_HashTable
{
    size_t capacity;
    size_t size;
    Focl_HashTableUnit** buckets;
    Focl_HashFunc hashFunc;
    size_t rehashLimit;
    float loadFactor;
}Focl_HashTable;

void FoclHashTableDelete(Focl_HashTable* table, void* key, KeyCompareFunc keyCompareFunc, Focl_KeyOpDt* keyOpDt, Focl_ValueOpDt* valueOpDt);
void FoclHashTableInsert(Focl_HashTable* table, void* key, void* value, KeyCompareFunc keyCompareFunc, Focl_KeyOpDt* keyOpDt, Focl_ValueOpDt* valueOpDt);

#define FOCL_OBJ_TABLE_INIT_CAPACITY 12
#define FOCL_OBJ_TABLE_LOAD_FACTOR 0.75f

typedef Focl_HashTable Focl_ObjTable;

#define FOCL_COMMAND_TABLE_INIT_CAPACITY 128
#define FOCL_COMMAND_TABLE_LOAD_FACTOR 0.85f

typedef Focl_HashTable Focl_CommandTable;

typedef struct Focl_Compound
{
    Focl_String* name;
    union
    {
        Focl_Vector* vec;
        Focl_HashTable* htable;
    }as;
    union
    {
        Focl_Object* (*vecAtFunc)(Focl_Vector* vec, Focl_Obj_Int idx);
        Focl_HashTable* (*htableAtFunc)(Focl_HashTable* hTable, Focl_String* str);
    }at;
}Focl_Compound;

typedef struct Focl_Environment
{
    size_t level;
    Focl_ObjTable* objTable;
    Focl_CommandTable* cmdTable;
    struct Focl_Environment* parent; /* if the level is 0, it will be NULL. */
    Focl_String* envNamespace; /* :: if root env. */
    Focl_Vector* namespaceVec; /* imported namespace vector */
}Focl_Environment;

#define FOCL_IOBUFFER_DEFAULT_SIZE 2048

typedef struct Focl_IOBuffer
{
    char* buf;
    FILE* fPtr;
    int used;
    int size; /* Why not use size_t? because of the snprintf()! */
}Focl_IOBuffer;

typedef struct Focl_Context
{
    Focl_StringPool* strPool;
    Focl_VectorPool* objVecPool;
    Focl_VectorPool* strVecPool;
    Focl_ObjTablePool* objTablePool;
    Focl_CommandTablePool* cmdTablePool;
    Focl_Object* returnValue;
    Focl_ObjWithNoStrPool* objWithNoStrPool;
    Focl_StrObjPool* strObjPool;
    Focl_CmpdObjPool* cmpdObjPool;
    Focl_EnvPool* envPool;
    Focl_IOBuffer* outBuffer; /* currently only have output buffer */
    Focl_Environment* globalEnv;
    Focl_Environment* curEnv;
    jmp_buf breakBuf;
    jmp_buf continueBuf;
    jmp_buf exitBuf;
    jmp_buf returnBuf;
    bool hasBreakBuf;
    bool hasContinueBuf;
    bool hasExitBuf;
    bool hasReturnBuf;
    int exitCode;
}Focl_Context;

typedef struct Focl_ExprParser
{
    Focl_Context* context;
    const char* pos;
    const char* end;
}Focl_ExprParser;

typedef struct Focl_Command
{
    Focl_Object* (*func)(Focl_Context* context, Focl_Vector* objVec, struct Focl_Command* cmd);
    Focl_String* name;
    Focl_String* proc; /* NULL if build-in */
    Focl_String* args; /* NULL if build-in */
}Focl_Command;

Focl_Command* createFoclCommand(Focl_StringPool* strPool, Focl_String* cmdName, Focl_StringView* argsView, Focl_StringView* procView);

typedef Focl_Object* (*Focl_CommandFunc)(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd);

#define FOCL_OBJECT_ERROR NULL
#define FOCL_COMMAND_ERROR NULL
 
#define FOCL_OBJ_TYPE_INT 0
#define FOCL_OBJ_TYPE_FLOAT 1
#define FOCL_OBJ_TYPE_BOOL 2
#define FOCL_OBJ_TYPE_VOID 3
#define FOCL_OBJ_TYPE_ERROR 4
#define FOCL_OBJ_TYPE_BYTECODE 5
#define FOCL_OBJ_TYPE_STR 6
#define FOCL_OBJ_TYPE_COMPOUND 7

#define FOCL_ERR_INVALID_ARG "Invalid argument"
#define FOCL_ERR_UNSUPPORTED_ARG_COUNT "Unsupported argument counts"
#define FOCL_ERR_CANNOT_FIND_OBJECT "Cannot find object"
#define FOCL_ERR_UNCLOSED_CURLY_BRACKET "Unclosed curly bracket"
#define FOCL_ERR_UNCLOSED_SQUARE_BRACKET "Unclosed square bracket"
#define FOCL_ERR_UNKNOWN_COMMAND "Unknown command"
#define FOCL_ERR_WRONG_TYPE_ASSIGNMENT "Wrong type in assignment"
#define FOCL_ERR_READ_ERR_STDIN "EOF or read error on stdin"
#define FOCl_ERR_INVALID_BLOCK "Invalid block"
#define FOCL_ERR_NO_EXEC_BLOCK "No block to execute when the if command is true"
#define FOCL_ERR_UNKNOWN_ARG "Unknown argument"
#define FOCL_ERR_MUST_BE_BLOCK "For arguments must be blocks"
#define FOCL_ERR_INDEX_NEGATIVE "Index couldn't be negative"
#define FOCL_ERR_INDEX_OOL "Index out of length"

#ifndef __FILE_NAME__
#define FOCL_FILE_NAME_ __FILE__
#else
#define FOCL_FILE_NAME_ __FILE_NAME__
#endif

#define _FOCL_STR_HELPER(x) #x
#define _FOCL_MACRO_AS_STR(x) _FOCL_STR_HELPER(x)
#define FOCL_ERR_YSNBH "You should not be here! Line: " _FOCL_MACRO_AS_STR(__LINE__) " File: " FOCL_FILE_NAME_

void FoclVectorPoolFree(Focl_Vector* vec, Focl_VectorPool* vecPool);

void LinkObjectWithName(Focl_Context* context, Focl_Object* obj, const Focl_String* str);

Focl_Object* Focl_exprBool(Focl_Context* context, const Focl_StringView* strView);

Focl_Object* Focl_parseBlock(Focl_Context* context, Focl_StringView* strView);
Focl_Object* Focl_parseCommand(Focl_Context* context, const Focl_StringView* strView);
Focl_Object* Focl_parseLine(Focl_Context* context, Focl_String* lineStr);
Focl_Command* Focl_FindCommand(Focl_Context* context, const Focl_String* target);

Focl_Object* exprParseExpression(Focl_ExprParser* p);
void exprSkipSpace(Focl_ExprParser* p);

Focl_IOBuffer* createFoclIOBuffer(FILE* fptr_, int bufferSize);
void freeFoclIOBuffer(Focl_IOBuffer* ioBuffer);
void FoclIOBufferFlushOut(Focl_IOBuffer* ioBuffer);
void FoclIOBufferPrintf(Focl_IOBuffer* ioBuffer, const char* fmt, ...);
void FoclIOBufferPutChar(Focl_IOBuffer* ioBuffer, char c);

Focl_Object* FoclObjectError(Focl_StrObjPool* strObjPool, Focl_StringPool* strPool, const char* errmsg);
Focl_Object* FoclObjectVoid(Focl_StrObjPool* strObjPool, Focl_StringPool* strPool);
Focl_Object* FoclObjectBool(Focl_ObjWithNoStrPool* objPool, Focl_Obj_Bool booleanValue);
void FoclObjectRetain(Focl_Object* obj);
void FoclObjectRelease(Focl_Object* obj, Focl_Context* context);

Focl_String* FoclObjectStringize(Focl_Object* obj, Focl_StringPool* strPool); /* free the return string! */
Focl_String* FoclCmpdObjStringize(Focl_Object* cmpdObj, Focl_StringPool* strPool); /* free the return string! */

Focl_Object* Focl_FindObject(Focl_Environment* env, Focl_StringPool* strPool, const Focl_String* target);
Focl_Object* FoclObjVecAt(Focl_Vector* objVec, size_t idx);
char* FoclStrCStr(const Focl_String* str);
void FoclStringPoolFreeOpDtVoid(void* str, void* strPool);
Focl_Object* FoclObjWithNoStringPoolAlloc(Focl_ObjWithNoStrPool* objPool, Focl_Obj_Type type_);
Focl_Object* FoclStringObjPoolAlloc(Focl_StrObjPool* strObjPool, Focl_StringPool* strPool, Focl_Obj_Type type_);
Focl_Object* FoclCmpdObjPoolAlloc(Focl_CmpdObjPool* cmpdObjPool, Focl_VectorPool* objVecPool);
Focl_Object* FoclObjPoolAllocAssign(Focl_Context* context, Focl_Object* src);
Focl_Object* FoclObjectCopy(Focl_Context* context, Focl_Object* src);

void FoclObjectPrint(Focl_Object* obj, Focl_IOBuffer* oBuffer, Focl_StringPool* strPool);
void FoclObjectGets(Focl_StringPool* strPool, Focl_Object* obj);
Focl_Object* FoclObjectScan(Focl_StrObjPool* strObjPool, Focl_StringPool* strPool, Focl_Object* obj);
Focl_Object* Focl_evalProc(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd);
void FoclRegisterCommand(Focl_Context* context, const char* cmdName, Focl_CommandFunc func);

int focl_countBraceDepth(const char* str);

void Focl_getline(FILE* fp, char** linePtr, size_t* len, size_t* capacity);

Focl_Object* Focl_evalFile(Focl_Context* ctx, const char* filename);

#define FOCL_OBJ_VEC_AT_AS_OBJ(objVec, idx, obj, dsttype, strObjPool, strPool) \
    obj = FoclObjVecAt(objVec, idx); \
    if (obj->type != dsttype) \
    { \
        return FoclObjectError(strObjPool, strPool, FOCL_ERR_INVALID_ARG); \
    } \

#define FOCL_OBJ_VEC_AT_AS_OBJ_WITH_CLEAR(objVec, idx, obj, dsttype, strObjPool, strPool, CLEAR_PROGN) \
    obj = FoclObjVecAt(objVec, idx); \
    if (obj->type != dsttype) \
    { \
        CLEAR_PROGN; \
        return FoclObjectError(strObjPool, strPool, FOCL_ERR_INVALID_ARG); \
    } \

#define FOCL_OBJ_VEC_AT_NOT_ERR(objVec, idx, obj, strObjPool, strPool) \
    obj = FoclObjVecAt(objVec, idx); \
    if (obj->type == FOCL_OBJ_TYPE_ERROR) \
    { \
        return FoclObjectError(strObjPool, strPool, FOCL_ERR_INVALID_ARG); \
    } \

#define FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, idx, obj, strObjPool, strPool) FOCL_OBJ_VEC_AT_AS_OBJ(objVec, idx, obj, FOCL_OBJ_TYPE_INT, strObjPool, strPool)
#define FOCL_OBJ_VEC_AT_AS_FLOAT_OBJ(objVec, idx, obj, strObjPool, strPool) FOCL_OBJ_VEC_AT_AS_OBJ(objVec, idx, obj, FOCL_OBJ_TYPE_FLOAT, strObjPool, strPool)
#define FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, idx, obj, strObjPool, strPool) FOCL_OBJ_VEC_AT_AS_OBJ(objVec, idx, obj, FOCL_OBJ_TYPE_STR, strObjPool, strPool)
#define FOCL_OBJ_VEC_AT_AS_COMPOUND_OBJ(objVec, idx, obj, strObjPool, strPool) FOCL_OBJ_VEC_AT_AS_OBJ(objVec, idx, obj, FOCL_OBJ_TYPE_COMPOUND, strObjPool, strPool)

#define FOCL_OBJ_VEC_AT_AS_STRING_OBJ_WITH_CLEAR(objVec, idx, obj, strObjPool, strPool, CLEAR_PROGN) FOCL_OBJ_VEC_AT_AS_OBJ_WITH_CLEAR(objVec, idx, obj, FOCL_OBJ_TYPE_STR, strObjPool, strPool, CLEAR_PROGN)

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define UNIQUE_NAME(prefix) CONCAT(prefix, __COUNTER__)

#define FOCL_OBJ_VEC_AT_AS_STRING(objVec, idx, strObj, str, strObjPool, strPool) \
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, idx, strObj, strObjPool, strPool) \
    str = FoclObjectGetString(strObj) \

#define FOCL_OBJ_VEC_AT_AS_STRING_WITH_CLEAR(objVec, idx, strObj, str, strObjPool, strPool, CLEAR_PROGN) \
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ_WITH_CLEAR(objVec, idx, strObj, strObjPool, strPool, CLEAR_PROGN) \
    str = FoclObjectGetString(strObj) \

#define FOCL_OBJ_VEC_AT_AS_STRING_VIEW(objVec, idx, strObj, str, strView, strObjPool, strPool) \
    FOCL_OBJ_VEC_AT_AS_STRING(objVec, idx, strObj, str, strObjPool, strPool); \
    strView = (Focl_StringView){str->length, str->data} \

#define FOCL_OBJ_VEC_AT_AS_STRING_VIEW_WITH_CLEAR(objVec, idx, strObj, str, strView, strObjPool, strPool, CLEAR_PROGN) \
    FOCL_OBJ_VEC_AT_AS_STRING_WITH_CLEAR(objVec, idx, strObj, str, strObjPool, strPool, CLEAR_PROGN); \
    strView = (Focl_StringView){str->length, str->data} \

#define FOCL_ERROBJ_ALLOC(errobj, ctx, content) \
    errobj = FoclStringObjPoolAlloc(ctx->strObjPool, ctx->strPool, FOCL_OBJ_TYPE_ERROR); \
    FoclStrAssign(FoclObjectGetString(errobj), content) \

#define FOCL_STROBJ_ALLOC(strobj, ctx, content) \
    strobj = FoclStringObjPoolAlloc(ctx->strObjPool, ctx->strPool, FOCL_OBJ_TYPE_STR); \
    FoclStrAssign(FoclObjectGetString(strobj), content) \

#define FOCL_STROBJ_CSTR(obj) FoclStrCStr(FoclObjectGetString(obj))

#ifdef __cplusplus
}
#endif

#endif