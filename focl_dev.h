#ifndef FOCL_DEV_H
#define FOCL_DEV_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdbool.h>

/* 
 * The aim of the file is to create a interface for outer program who wants
 * to embed Focl. The focl runtime don't lean on it
 */

#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF
    typedef uint32_t Focl_Obj_Type;
    typedef uint32_t Focl_Obj_RefCount;
    typedef double Focl_Obj_Float;
    typedef int64_t Focl_Obj_Int;
    #define FOCL_OBJ_INT_MAX ((1LL << 61) - 1)
    #define FOCL_FORMAT_INT PRId64
    #define FOCL_FORMAT_FLOAT "lf"
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

typedef struct Focl_String
{
    size_t capacity;
    size_t length;
    char* data;
}Focl_String;

typedef struct Focl_StringView
{
    size_t len;
    char* strPtr;
}Focl_StringView;

#define FOCL_VECTOR_INIT_CAPACITY 32

typedef struct Focl_Vector
{
    size_t itemSize;
    size_t capacity;
    size_t size;
    void* data;
}Focl_Vector;

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

#define FOCL_VECTOR_POOL_ITEM_PER_BLOCK 8
#define FOCL_VECTOR_POOL_BLOCK_COUNT_INIT 2

typedef Focl_Pool Focl_VectorPool;

#define FOCL_OBJ_POOL_ITEM_PER_BLOCK 16
#define FOCL_OBJ_POOL_BLOCK_COUNT_INIT 2

#define FOCL_OBJ_POOL_WITH_NO_STR_DEFAULT_TYPE FOCL_OBJ_TYPE_INT
#define FOCL_OBJ_POOL_WITH_STR_DEFAULT_TYPE FOCL_OBJ_TYPE_STR

typedef Focl_Pool Focl_ObjWithNoStrPool;
typedef Focl_Pool Focl_StrObjPool;

typedef struct Focl_Object
{
    Focl_Obj_RefCount refCount;
    Focl_Obj_Type type;
    union
    {
        Focl_String* data;
        Focl_Obj_Float f;
        Focl_Obj_Int i;
    }as;
}Focl_Object;

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

typedef Focl_HashTable Focl_ObjTable;
typedef Focl_HashTable Focl_CommandTable;

typedef struct Focl_Context Focl_Context;

typedef struct Focl_Command
{
    Focl_Object* (*func)(Focl_Context* context, Focl_Vector* objVec, struct Focl_Command* cmd);
    Focl_String* proc; /* NULL if build-in */
    Focl_String* args; /* NULL if build-in */
}Focl_Command;

typedef Focl_Object* (*Focl_CommandFunc)(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd);

typedef struct Focl_Environment
{
    size_t level;
    Focl_ObjTable* objTable;
    Focl_CommandTable* cmdTable;
    struct Focl_Environment* parent; /* if the level is 0, it will be NULL. */
}Focl_Environment;

typedef struct Focl_Context
{
    Focl_Environment* globalEnv;
    Focl_Environment* curEnv;
    Focl_StringPool* strPool;
    Focl_VectorPool* vecPool;
    Focl_Object* returnValue;
    Focl_ObjWithNoStrPool* objWithNoStrPool;
    Focl_StrObjPool* strObjPool;
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

void FoclObjectRetain(Focl_Object* obj);
void FoclObjectRelease(Focl_Object* obj, Focl_Context* context);

#define FOCL_OBJECT_ERROR NULL

#define FOCL_OBJ_TYPE_INT 0
#define FOCL_OBJ_TYPE_FLOAT 1
#define FOCL_OBJ_TYPE_BOOL 2
#define FOCL_OBJ_TYPE_VOID 3
#define FOCL_OBJ_TYPE_ERROR 4
#define FOCL_OBJ_TYPE_BYTECODE 5
#define FOCL_OBJ_TYPE_STR 6
#define FOCL_OBJ_TYPE_COMPLEX 7

#define FOCL_ERR_INVALID_ARG "Invalid argument"
#define FOCL_ERR_UNSUPPORTED_ARG_COUNT "Unsupported argument counts"
#define FOCL_ERR_CANNOT_FIND_OBJECT "Cannot find object"
#define FOCL_ERR_UNCLOSED_CURLY_BRACKET "Unclosed curly bracket"
#define FOCL_ERR_UNCLOSED_SQUARE_BRACKET "Unclosed square bracket"
#define FOCL_ERR_UNKNOWN_COMMAND "Unknown command"
#define FOCL_ERR_WRONG_TYPE_ASSIGNMENT "Wrong type in assignment"
#define _FOCL_STR_HELPER(x) #x
#define _FOCL_MACRO_AS_STR(x) _FOCL_STR_HELPER(x)
#define FOCL_ERR_YSNBH "You should not be here! Line: " _FOCL_MACRO_AS_STR(__LINE__)

Focl_Object* FoclObjectError(Focl_StrObjPool* objPool, Focl_StringPool* strPool, const char* errmsg);
size_t FoclVectorGetSize(Focl_Vector* vec);

Focl_Object* FoclObjVecAt(Focl_Vector* objVec, size_t idx);
Focl_String* FoclObjVecAtAsString(Focl_Vector* objVec, size_t idx);
Focl_StringView FoclObjVecAtAsStringToView(Focl_Vector* objVec, size_t idx);
char* FoclStrCStr(const Focl_String* str);
Focl_Object* FoclObjectVoid(Focl_StrObjPool* strObjPool, Focl_StringPool* strPool);
Focl_Object* FoclObjWithNoStringPoolAlloc(Focl_ObjWithNoStrPool* objPool, Focl_Obj_Type type_);
Focl_Object* FoclStringObjPoolAlloc(Focl_StrObjPool* objPool, Focl_StringPool* strPool, Focl_Obj_Type type_);
Focl_Object* FoclObjectBool(Focl_ObjWithNoStrPool* objPool, Focl_Obj_Bool booleanValue);

#define FOCL_OBJ_VEC_AT_AS_OBJ(objVec, idx, obj, dsttype, strObjPool, strPool) \
    obj = FoclObjVecAt(objVec, idx); \
    if (obj->type != dsttype) \
    { \
        return FoclObjectError(strObjPool, strPool, FOCL_ERR_INVALID_ARG);\
    } \

#define FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, idx, obj, strObjPool, strPool) FOCL_OBJ_VEC_AT_AS_OBJ(objVec, idx, obj, FOCL_OBJ_TYPE_INT, strObjPool, strPool)
#define FOCL_OBJ_VEC_AT_AS_FLOAT_OBJ(objVec, idx, obj, strObjPool, strPool) FOCL_OBJ_VEC_AT_AS_OBJ(objVec, idx, obj, FOCL_OBJ_TYPE_FLOAT, strObjPool, strPool)
#define FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, idx, obj, strObjPool, strPool) FOCL_OBJ_VEC_AT_AS_OBJ(objVec, idx, obj, FOCL_OBJ_TYPE_STR, strObjPool, strPool)

#endif