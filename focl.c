#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>

/*
 * NOTICE:
 *   No matter who you are, human or LLM, when you read the code 
 * And found almost no any NULL pointer check in it. You may feel
 * uncomfortable and think what I have done is a miss. But I have
 * to tell you that it's delibrately. If the pointer won't point to
 * NULL, there's no need to check it again, or you want to get some
 * performance loss, which is a shame for any C user. So please, don't
 * try to add any NULL pointer check code unless is a must. We cannot
 * let the modern language with optional pointer user laugh on us anymore.
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
    static Focl_Obj_Int Focl_StrToInt(const char* str)
    {
        return strtoll(str, NULL, 0);
    }
    static Focl_Obj_Float Focl_StrToFloat(const char* str)
    {
        return strtod(str, NULL);
    }
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
    static Focl_Obj_Int Focl_StrToInt(const char* str)
    {
        return strtol(str, NULL, 0);
    }
    static Focl_Obj_Float Focl_StrToFloat(const char* str)
    {
        return strtod(str, NULL);
    }
#else
    #error "Unsupported word length platform. Though I want to see this program run in every platform. But now it couldn't run yours. Sorry. :("
#endif

typedef Focl_Obj_Int Focl_Obj_Bool;
#define FOCL_OBJ_TRUE 1
#define FOCL_OBJ_FALSE 0

#define FOCL_STRING_INIT_CAPACITY 64
#define FOCL_STRING_NPOS SIZE_MAX

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

typedef void (*VectorItemFreeFunc)(void*);

typedef struct Focl_Vector
{
    size_t itemSize;
    size_t capacity;
    size_t size;
    void* data;
}Focl_Vector;

typedef void (*PoolItemInitFunc)(void*);
typedef void (*PoolItemAllocResetFunc)(void*);
typedef void (*PoolItemFreeFunc)(void*);

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

static void FoclObjectRetain(Focl_Object* obj);
static void FoclObjectRelease(Focl_Object* obj, Focl_StringPool* strPool);

typedef size_t (*Focl_HashFunc)(void*);
typedef bool (*KeyCompareFunc)(void*, void*);

typedef void (*KeyFreeFunc)(void*);
typedef void (*ValueFreeFunc)(void*);

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

typedef Focl_Object* (*Focl_CommandFunc)(Focl_Context* context, Focl_Vector* objVec);

typedef struct Focl_Command
{
    Focl_CommandFunc func;
}Focl_Command;

typedef struct Focl_Environment
{
    size_t level;
    Focl_ObjTable* objTable;
    Focl_CommandTable* cmdTable;
    struct Focl_Environment* parent; /* if the level is 0, it will be NULL. */
}Focl_Environment;

#define FOCL_REGISTERS_INIT_SIZE 32

typedef Focl_Object* Focl_Register;

typedef struct Focl_Registers
{
    size_t regCount;
    Focl_Register* r;
}Focl_Registers;

typedef struct Focl_Context
{
    Focl_Environment* globalEnv;
    Focl_Environment* curEnv;
    Focl_Registers* regs;
    Focl_StringPool* strPool;
    Focl_VectorPool* vecPool;
}Focl_Context;

void FoclStrExpansion(Focl_Context* context, Focl_String* dst, const Focl_StringView* src);
Focl_Object* FindObjectInContext(Focl_Context* context, Focl_StringView* str);

Focl_Object* Focl_parseCommand(Focl_Context* context, const Focl_StringView* strView);
Focl_Object* Focl_parseCommandSequence(Focl_Context* context, Focl_StringView* strView);

char* Focl_getline(FILE* fp, size_t* len);

/* STRING */

int32_t getUtf8CodePointLength(uint8_t firstByte)
{
    if ((firstByte & 0x80) == 0x00)
    {
        return 1;
    }
    else if ((firstByte & 0xE0) == 0xC0)
    {
        return 2;
    }
    else if ((firstByte & 0xF0) == 0xE0)
    {
        return 3;
    }
    else if ((firstByte & 0xF8) == 0xF0)
    {
        return 4;
    }
    else
    {
        return 1;
    }
}
int32_t getUtf8CodePoint(const char* bytes)
{
    uint8_t first = bytes[0];
    if ((first & 0x80) == 0)
    {
        return first;
    }
    if ((first & 0xE0) == 0xC0)
    {
        return ((first & 0x1F) << 6) | (bytes[1] & 0x3F);
    }
    if ((first & 0xF0) == 0xE0)
    {
        return ((first & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
    }
    return ((first & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) | ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
}


Focl_String* createFoclString(size_t iCapacity)
{
    Focl_String* str = (Focl_String*)malloc(sizeof(Focl_String));
    str->data = malloc(sizeof(char) * iCapacity);
    str->capacity = iCapacity;
    str->length = 0;
    str->data[iCapacity - 1] = '\0';
    return str;
}
Focl_String* createFoclStringWithCStr(const char* Cstr)
{
    size_t len = strlen(Cstr);
    size_t capacity = len + 1;
    Focl_String* str = (Focl_String*)malloc(sizeof(Focl_String));
    str->data = malloc(sizeof(char) * (capacity));
    strcpy(str->data, Cstr);
    str->length = len;
    str->capacity = capacity;
    return str;
}
Focl_String* createFoclStringWithView(const Focl_StringView* strView)
{
    size_t len = strView->len;
    size_t capacity = strView->len + 1;
    Focl_String* str = (Focl_String*)malloc(sizeof(Focl_String));
    str->data = malloc(sizeof(char) * (capacity));
    memcpy(str->data, strView->strPtr, len);
    str->data[len] = '\0';
    str->length = len;
    str->capacity = capacity;
    return str;
}
char initTempFoclStringWithView(Focl_String* tmpStr, Focl_StringView* strView) /* The tmpStr should be on stack! This function will return the savedPos char. */
{
    size_t sLen = strView->len;
    tmpStr->capacity = sLen + 1;
    tmpStr->length = sLen;
    tmpStr->data = strView->strPtr;
    char* savedPtr = strView->strPtr + sLen;
    char saved = *savedPtr;
    *savedPtr = '\0';
    return saved;
}
static void restoreFoclStringViewFromTempString(Focl_StringView* strView, char saved) /* The tmpStr should be on stack! */
{
    strView->strPtr[strView->len] = saved;
}
void initFoclString(Focl_String* str, size_t iCapacity)
{
    str->data = malloc(sizeof(char) * iCapacity);
    str->capacity = iCapacity;
    str->length = 0;
    str->data[iCapacity - 1] = '\0';
}
void FoclStrAssign(Focl_String* str, const char* cStr)
{
    size_t newLen = strlen(cStr);
    if (newLen >= str->capacity)
    {
        str->capacity = newLen + 1;
        str->data = (char*)realloc(str->data, str->capacity);
        strcpy(str->data, cStr);
        str->length = newLen;
    }
    else
    {
        strcpy(str->data, cStr);
        str->length = newLen;
    }
}
static char* FoclStrCStr(const Focl_String* str)
{
    return (char*)str->data;
}
static void FoclStrAssignStr(Focl_String* dst, const Focl_String* src)
{
    FoclStrAssign(dst, FoclStrCStr(src));
}
static void FoclStrReserve(Focl_String* str, size_t newSize_)
{
    if (str->capacity >= newSize_)
    {
        return;
    }
    str->data = (char*)realloc(str->data, sizeof(char) * newSize_);
    str->capacity = newSize_;
}
static void FoclStrReserveWithoutCheck(Focl_String* str, size_t newSize_)
{
    str->data = (char*)realloc(str->data, sizeof(char) * newSize_);
    str->capacity = newSize_;
}
void FoclStrAssignView(Focl_String* dst, const Focl_StringView* view)
{
    if (view->len > dst->length)
    {
        FoclStrReserve(dst, view->len + 1);
    }
    memcpy(dst->data, view->strPtr, sizeof(char) * view->len);
    dst->data[view->len] = '\0';
    dst->length = view->len;
}
static void FoclStrDoubleReserve(Focl_String* str)
{
    FoclStrReserve(str, str->capacity * 2);
}
void FoclStrAppend(Focl_String* str, const char* Cstr)
{
    size_t len = strlen(Cstr);
    if (str->length + len < str->capacity)
    {
        memcpy(str->data + str->length, Cstr, len);
    }
    else
    {
        FoclStrReserveWithoutCheck(str, str->length + len + 1);
        memcpy(str->data + str->length, Cstr, len);    
    }
    str->length += len;
    str->data[str->length] = '\0';
}
static void FoclStrAppendStr(Focl_String* dst, Focl_String* src)
{
    FoclStrAppend(dst, FoclStrCStr(src));
}
int32_t FoclStrAt(size_t idx, char** start, size_t* hadSearchIdx)
{
    size_t searchIdx = *hadSearchIdx;
    size_t tmp;
    char* ptr = *start;
    while (*ptr)
    {
        tmp = getUtf8CodePointLength((uint8_t)*ptr);
        if (searchIdx == idx)
        {
            *hadSearchIdx = searchIdx;
            return getUtf8CodePoint(ptr);
        }
        ptr += tmp;
        searchIdx++;
    }
    return 0;
}
int32_t FoclStrAtPace(char** searchStart)
{
    if (**searchStart == 0)
    {
        return 0;
    }
    int32_t cp = getUtf8CodePoint(*searchStart);
    size_t tmp = getUtf8CodePointLength((uint8_t)**searchStart);
    *searchStart += tmp;
    return cp;
}
static int FoclStrComp(const Focl_String* str, const char* cStr)
{
    return strcmp(str->data, cStr);
}
static int FoclStrCompStr(const Focl_String* str1, const Focl_String* str2)
{
    return strcmp(str1->data, str2->data);
}
static int FoclStrCompStrView(const Focl_String* str, const Focl_StringView* strView)
{
    if (str->length != strView->len)
    {
        return (str->length < strView->len) ? -1 : 1;
    }
    return memcmp(str->data, strView->strPtr, strView->len);
}
static void FoclStrClear(Focl_String* str)
{
    str->length = 0;
    str->data[0] = '\0';
}
void freeFoclString(Focl_String* str)
{
    free(str->data);
    free(str);
}
static void freeFoclStringData(Focl_String* str)
{
    free(str->data);
}
static void freeFoclStringDataAsVoid(void* str)
{
    freeFoclStringData((Focl_String*)str);
}

static Focl_StringView FoclStringViewPeelFront(Focl_StringView* strView)
{
    Focl_StringView peelView = {strView->len - 1, strView->strPtr + 1};
    return peelView;
}
static Focl_StringView FoclStringViewPeelBoth(Focl_StringView* strView)
{
    Focl_StringView peelView = {strView->len - 2, strView->strPtr + 1};
    return peelView;
}

static int FoclStringViewComp(Focl_StringView* strView, const char* Cstr)
{
    size_t cStrLen = strlen(Cstr);
    if (cStrLen != strView->len)
    {
        return (strView->len < cStrLen) ? -1 : 1;
    }
    return memcmp(strView->strPtr, Cstr, cStrLen);
}

static bool isStringViewEnd(const Focl_StringView* strView, const char* pos)
{
    return (pos >= strView->strPtr + strView->len);
}

/* Additional String Type Function */

bool Focl_isInteger(const char* str)
{
    if (*str == '-' || *str == '+')
    {
        str++;
    }
    if (*str == '\0')
    {
        return false;
    }
    if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X'))
    {
        str += 2;
        if (*str == '\0')
        {
            return false;
        }

        bool hasDigit = false;
        while (*str)
        {
            if ((*str >= '0' && *str <= '9') ||
                (*str >= 'a' && *str <= 'f') ||
                (*str >= 'A' && *str <= 'F'))
            {
                hasDigit = true;
            }
            else
            {
                return false;
            }
            str++;
        }
        return hasDigit;
    }
    bool hasDigit = false;
    if (*str == '0')
    {
        if (*(str + 1) == '\0')
        {
            return true;
        }
        while (*str)
        {
            if (*str >= '0' && *str <= '7')
            {
                hasDigit = true;
            }
            else
            {
                return false;
            }
            str++;
        }
        return hasDigit;
    }
    while (*str)
    {
        if (*str >= '0' && *str <= '9')
        {
            hasDigit = true;
        }
        else
        {
            return false;
        }
        str++;
    }
    return hasDigit;
}
bool Focl_isInteger_View(const Focl_StringView* strView)
{
    char* str = strView->strPtr;
    if (*str == '-' || *str == '+')
    {
        str++;
    }
    if (isStringViewEnd(strView, str))
    {
        return false;
    }
    if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X'))
    {
        str += 2;
        if (isStringViewEnd(strView, str))
        {
            return false;
        }

        bool hasDigit = false;
        while (*str)
        {
            if ((*str >= '0' && *str <= '9') ||
                (*str >= 'a' && *str <= 'f') ||
                (*str >= 'A' && *str <= 'F'))
            {
                hasDigit = true;
            }
            else
            {
                return false;
            }
            str++;
        }
        return hasDigit;
    }
    bool hasDigit = false;
    if (*str == '0')
    {
        if (isStringViewEnd(strView, str))
        {
            return true;
        }
        while (!isStringViewEnd(strView, str))
        {
            if (*str >= '0' && *str <= '7')
            {
                hasDigit = true;
            }
            else
            {
                return false;
            }
            str++;
        }
        return hasDigit;
    }
    while (!isStringViewEnd(strView, str))
    {
        if (*str >= '0' && *str <= '9')
        {
            hasDigit = true;
        }
        else
        {
            return false;
        }
        str++;
    }
    return hasDigit;
}
bool Focl_isDecimal(const char* str)
{
    if (*str == '-' || *str == '+')
    {
        str++;
    }

    bool hasDot = false;
    bool hasDigit = false;

    while (*str)
    {
        if (*str >= '0' && *str <= '9')
        {
            hasDigit = true;
        }
        else if (*str == '.')
        {
            if (hasDot)
            {
                return false;
            }
            hasDot = true;
        }
        else
        {
            return false;
        }
        str++;
    }

    return hasDigit;
}
bool Focl_isDecimal_View(const Focl_StringView* strView)
{
    char* str = strView->strPtr;
    if (*str == '-' || *str == '+')
    {
        str++;
    }

    bool hasDot = false;
    bool hasDigit = false;

    while (!isStringViewEnd(strView, str))
    {
        if (*str >= '0' && *str <= '9')
        {
            hasDigit = true;
        }
        else if (*str == '.')
        {
            if (hasDot)
            {
                return false;
            }
            hasDot = true;
        }
        else
        {
            return false;
        }
        str++;
    }

    return hasDigit;
}
bool Focl_isFloat(const char* str)
{
    if (*str == '-' || *str == '+')
    {
        str++;
    }

    bool hasDot = false;
    bool hasDigit = false;

    while (*str)
    {
        if (*str >= '0' && *str <= '9')
        {
            hasDigit = true;
        }
        else if (*str == '.')
        {
            if (hasDot)
            {
                return false;
            }
            hasDot = true;
        }
        else
        {
            return false;
        }
        str++;
    }

    return hasDigit && hasDot;
}
bool Focl_isFloat_View(const Focl_StringView* strView)
{
    char* str = strView->strPtr;
    if (*str == '-' || *str == '+')
    {
        str++;
    }

    bool hasDot = false;
    bool hasDigit = false;

    while (!isStringViewEnd(strView, str))
    {
        if (*str >= '0' && *str <= '9')
        {
            hasDigit = true;
        }
        else if (*str == '.')
        {
            if (hasDot)
            {
                return false;
            }
            hasDot = true;
        }
        else
        {
            return false;
        }
        str++;
    }

    return hasDigit && hasDot;
}
static bool Focl_isVoid(const char* str) /* pure "" */
{
    return *str == '\0';
}
static bool Focl_isVoidView(const Focl_StringView* strView) /* pure "" */
{
    return (*(strView->strPtr) == '\0' || strView->len == 0);
}
bool Focl_isString(const char* str) /* Has "" */
{
    if (*str != '"')
    {
        return false;
    }

    str++;
    while (*str && *str != '"')
    {
        if (*str == '\\')
        {
            str++;
            if (*str == '\0')
            {
                return false;
            }
        }
        str++;
    }

    return *str == '"';
}
bool Focl_isString_View(const Focl_StringView* strView) /* Has "" */
{
    char* str = strView->strPtr;
    if (*str == '"' && *(str + strView->len - 1) == '"')
    {
        return true;
    }

    return false;
}
bool Focl_isBlock(const char* str)
{
    if (*str != '{')
    {
        return false;
    }

    int depth = 1;
    str++;
    while (*str && depth > 0)
    {
        if (*str == '\\')
        {
            str++;
            if (*str == '\0')
            {
                return false;
            }
        }
        else if (*str == '{')
        {
            depth++;
        }
        else if (*str == '}')
        {
            depth--;
        }
        str++;
    }

    return depth == 0 && *str == '\0';
}
bool Focl_isBlock_View(const Focl_StringView* strView)
{
    char* str = strView->strPtr;
    if (*str != '{')
    {
        return false;
    }

    int depth = 1;
    str++;
    while (!isStringViewEnd(strView, str) && depth > 0)
    {
        if (*str == '\\')
        {
            str++;
            if (isStringViewEnd(strView, str))
            {
                return false;
            }
        }
        else if (*str == '{')
        {
            depth++;
        }
        else if (*str == '}')
        {
            depth--;
        }
        str++;
    }

    return depth == 0 && *str == '\0';
}
bool Focl_isCmdSubstition(const char* str)
{
    if (*str != '[')
    {
        return false;
    }

    char* start = str;
    char* pace = start + 1;
    
    while (*pace)
    {
        if (*pace == ']')
        {
            return true;
        }
        pace++;
    }
    return false;
}
bool Focl_isCmdSubstition_View(const Focl_StringView* strView)
{
    char* str = strView->strPtr;

    if (*str == '[' && *(str + strView->len - 1) == ']')
    {
        return true;
    }
    return false;
}
bool Focl_isRawString(const char* str) /* Has no "" */
{
    if (Focl_isVoid(str))
    {
        return false;
    }

    while (*str)
    {
        if (*str == '"')
        {
            return false;
        }
        str++;
    }

    return true;
}
bool Focl_isRawString_View(const Focl_StringView* strView) /* Has no "" */
{
    if (Focl_isVoidView(strView))
    {
        return false;
    }

    char* str = strView->strPtr;

    while (!isStringViewEnd(strView, str))
    {
        if (*str == '"')
        {
            return false;
        }
        str++;
    }

    return true;
}
bool Focl_isVarSubstition(const char* str)
{
    if (*str == '$' && (*str + 1) != '\0')
    {
        return true;
    }
    return false;
}
bool Focl_isVarSubstition_View(const Focl_StringView* strView)
{
    if (*(strView->strPtr) == '$' && strView->len > 1)
    {
        return true;
    }
    return false;
}

size_t hashDjb2(const Focl_String* str)
{
    size_t hash = 5381;
    const char* p = (const char*)str->data;
    char c;
    while ((c = *p++))
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}
static size_t hashDjb2AsArg(void* str)
{
    return hashDjb2((Focl_String*)str);
}

static bool StrKeyCompare(void* a, void* b)
{
    return (FoclStrCompStr((Focl_String*)a, (Focl_String*)b) == 0);
}

/* STRING */

/* VECTOR */

Focl_Vector* createFoclVector(size_t itemSize_, size_t iCapacity)
{
    Focl_Vector* vec = (Focl_Vector*)malloc(sizeof(Focl_Vector));
    vec->capacity = iCapacity;
    vec->size = 0;
    vec->itemSize = itemSize_;
    vec->data = malloc(sizeof(char) * itemSize_ * iCapacity);
    return vec;
}
void initFoclVector(Focl_Vector* vec, size_t itemSize_, size_t iCapacity)
{
    vec->data = malloc(sizeof(char) * itemSize_ * iCapacity);
    vec->capacity = iCapacity;
    vec->size = 0;
    vec->itemSize = itemSize_;
}
static void FoclVectorReserve(Focl_Vector* vec, size_t newCapacity_)
{
    vec->data = realloc(vec->data, vec->itemSize * newCapacity_);
    vec->capacity = newCapacity_;
}
static void FoclVectorDoubleReserve(Focl_Vector* vec)
{
    FoclVectorReserve(vec, vec->capacity * 2);
}
static void* FoclVectorAt(Focl_Vector* vec, size_t idx) /* NULL-able */
{
    if (idx >= vec->size)
    {
        return NULL;
    }
    return (void*)((uint8_t*)(vec->data) + vec->itemSize * idx);
}
static void* FoclVectorAtNoCheck(Focl_Vector* vec, size_t idx) /* Warning: it will not check whether index out of bounds. */
{
    return (void*)((uint8_t*)(vec->data) + vec->itemSize * idx);
}
static void FoclVectorPushBack(Focl_Vector* vec, void* data)
{
    if (vec->size >= vec->capacity)
    {
        FoclVectorDoubleReserve(vec);
    }
    memcpy((uint8_t*)vec->data + (vec->size++ * vec->itemSize), data, vec->itemSize);
}
static void FoclVectorPopBack(Focl_Vector* vec)
{
    if (vec->size != 0)
    {
        vec->size--;
    }
}
static void FoclVectorClear(Focl_Vector* vec)
{
    vec->size = 0;
}
static size_t FoclVectorGetSize(Focl_Vector* vec)
{
    return (vec->size);
}
void freeFoclVector(Focl_Vector* vec, VectorItemFreeFunc freeFunc) /* If NULL, won't free item. */
{
    if (freeFunc != NULL)
    {
        size_t itemCount = vec->size;
        size_t itemSize_ = vec->itemSize;
        for (size_t i = 0; i < itemCount; i++)
        {
            freeFunc((uint8_t*)vec->data + i * itemSize_);
        }
    }
    free(vec->data);
    free(vec);
}
static void freeFoclVectorData(Focl_Vector* vec, VectorItemFreeFunc freeFunc) /* If NULL, won't free item. */
{
    if (freeFunc != NULL)
    {
        size_t itemCount = vec->size;
        size_t itemSize_ = vec->itemSize;
        for (size_t i = 0; i < itemCount; i++)
        {
            freeFunc((uint8_t*)vec->data + i * itemSize_);
        }
    }
    free(vec->data);
}

/* VECTOR */

/* HASH TABLE */

Focl_HashTableUnit* createFoclHashTableUnit(void* key_, void* value_)
{
    Focl_HashTableUnit* unit = (Focl_HashTableUnit*)malloc(sizeof(Focl_HashTableUnit));
    unit->key = key_;
    unit->value = value_;
    unit->next = NULL;
    return unit;
}
void freeFoclHashTableUnit(Focl_HashTableUnit* unit, KeyFreeFunc keyFreeFunc, ValueFreeFunc valueFreeFunc)
{
    if (keyFreeFunc != NULL)
    {
        keyFreeFunc(unit->key);
    }
    if (valueFreeFunc != NULL)
    {
        valueFreeFunc(unit->value);
    }
    free(unit);
}

Focl_HashTable* createFoclHashTable(size_t iCapacity, float loadFactor_, Focl_HashFunc hashFunc_)
{
    Focl_HashTable* table = (Focl_HashTable*)malloc(sizeof(Focl_HashTable));
    table->buckets = (Focl_HashTableUnit**)calloc(iCapacity, sizeof(Focl_HashTableUnit*));
    table->capacity = iCapacity;
    table->size = 0;
    table->loadFactor = loadFactor_;
    table->hashFunc = hashFunc_;
    table->rehashLimit = (size_t)(loadFactor_ * iCapacity) + 1;
    return table;
}
void FoclHashTableRehash(Focl_HashTable* table)
{
    Focl_HashTableUnit** oldBuckets = table->buckets;
    size_t old_capacity = table->capacity;
    size_t newCapacity = table->capacity * 2;
    Focl_HashTableUnit** newBuckets = (Focl_HashTableUnit**)calloc(newCapacity, sizeof(Focl_HashTableUnit*));
    table->buckets = newBuckets;
    table->capacity = newCapacity;
    table->size = 0;
    for (size_t i = 0; i < old_capacity; i++)
    {
        Focl_HashTableUnit* current = oldBuckets[i];
        while (current != NULL)
        {
            Focl_HashTableUnit* next = current->next;
            size_t newIdx = table->hashFunc(current->key) % table->capacity;
            current->next = table->buckets[newIdx];
            table->buckets[newIdx] = current;
            table->size++;
            current = next;
        }
    }
    table->rehashLimit = (size_t)(table->loadFactor * table->capacity) + 1;
    free(oldBuckets);
}
void FoclHashTableInsert(Focl_HashTable* table, void* key, void* value, KeyCompareFunc keyCompareFunc, ValueFreeFunc valueFreeFunc)
{
    if (table->size >= table->rehashLimit)
    {
        FoclHashTableRehash(table);
    }

    size_t idx = (table->hashFunc(key)) % table->capacity;
    Focl_HashTableUnit* current = table->buckets[idx];
    while (current != NULL)
    {
        if (keyCompareFunc(current->key, key))
        {
            if (current->value != value && valueFreeFunc != NULL)
            {
                valueFreeFunc(current->value);
            }
            current->value = value;
            return;
        }
        current = current->next;
    }
    Focl_HashTableUnit* unit = createFoclHashTableUnit(key, value);
    unit->next = table->buckets[idx];
    table->buckets[idx] = unit;
    table->size++;
}
void FoclHashTableDelete(Focl_HashTable* table, void* key, KeyCompareFunc keyCompareFunc, KeyFreeFunc keyFreeFunc, ValueFreeFunc valueFreeFunc)
{
    size_t idx = (table->hashFunc(key)) % table->capacity;
    Focl_HashTableUnit* current = table->buckets[idx];
    Focl_HashTableUnit* prev = NULL;
    if (keyFreeFunc != NULL || valueFreeFunc != NULL)
    {
        while (current != NULL)
        {
            if (keyCompareFunc(current->key, key))
            {
                if (prev == NULL)
                {
                    table->buckets[idx] = current->next;
                }
                else
                {
                    prev->next = current->next;
                }
                freeFoclHashTableUnit(current, keyFreeFunc, valueFreeFunc);
                table->size--;
                /* won't need to free(current) again, because it was free in freeFoclHashTableUnit() */
                return;
            }
            prev = current;
            current = current->next;
        }
    }
    else
    {
        while (current != NULL)
        {
            if (keyCompareFunc(current->key, key))
            {
                if (prev == NULL)
                {
                    table->buckets[idx] = current->next;
                }
                else
                {
                    prev->next = current->next;
                }
                table->size--;
                free(current);
                return;
            }
            prev = current;
            current = current->next;
        }
    }
}
void* FoclHashTableFind(Focl_HashTable* table, void* key, KeyCompareFunc keyCompareFunc)
{
    size_t idx = table->hashFunc(key) % table->capacity;
    Focl_HashTableUnit* current = table->buckets[idx];
    while (current != NULL)
    {
        if (keyCompareFunc(current->key, key))
        {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}
void freeFoclHashTable(Focl_HashTable* table, KeyFreeFunc keyFreeFunc, ValueFreeFunc valueFreeFunc)
{
    Focl_HashTableUnit* unit;
    Focl_HashTableUnit* next;
    if (keyFreeFunc != NULL || valueFreeFunc != NULL)
    {
        for (size_t i = 0; i < table->capacity; i++)
        {
            unit = table->buckets[i];
            while (unit != NULL)
            {
                next = unit->next;
                freeFoclHashTableUnit(unit, keyFreeFunc, valueFreeFunc);
                unit = next;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < table->capacity; i++)
        {
            unit = table->buckets[i];
            while (unit != NULL)
            {
                next = unit->next;
                free(unit);
                unit = next;
            }
        }
    }
    free(table->buckets);
    free(table);
}

/* HASH TABLE */

/* POOL */

Focl_PoolBlock* createFoclPoolBlock(size_t itemCount, size_t itemSize, PoolItemInitFunc initFunc)
{
    Focl_PoolBlock* block = (Focl_PoolBlock*)malloc(sizeof(Focl_PoolBlock));
    block->itemCount = itemCount;
    block->itemSize = itemSize;
    block->freeTop = itemCount;
    block->freeStack = (size_t*)malloc(itemCount * sizeof(size_t));
    block->data = malloc(itemCount * itemSize);
    if (initFunc != NULL)
    {
        for (size_t i = 0; i < itemCount; i++)
        {
            block->freeStack[i] = i;
            initFunc((uint8_t*)block->data + i * itemSize);
        }
    }
    else
    {
        for (size_t i = 0; i < itemCount; i++)
        {
            block->freeStack[i] = i;
        }
    }
    return block;
}
void freeFoclPoolBlock(Focl_PoolBlock* block, PoolItemFreeFunc freeFunc)
{
    if (freeFunc)
    {
        for (size_t i = 0; i < block->itemCount; i++)
        {
            freeFunc((uint8_t*)block->data + i * block->itemSize);
        }
    }
    free(block->freeStack);
    free(block->data);
    free(block);
}
Focl_Pool* createFoclPool(size_t itemSize_, size_t itemPerBlock_, size_t iBlockCount, PoolItemInitFunc initFunc)
{
    Focl_Pool* pool = (Focl_Pool*)malloc(sizeof(Focl_Pool));
    pool->itemSize = itemSize_;
    pool->itemPerBlock = itemPerBlock_;
    pool->blocks = (Focl_PoolBlock**)malloc(sizeof(Focl_PoolBlock*) * iBlockCount);
    pool->blockCount = iBlockCount;
    for (size_t i = 0; i < iBlockCount; i++)
    {
        pool->blocks[i] = createFoclPoolBlock(itemPerBlock_, itemSize_, initFunc);
    }
    return pool;
}
void freeFoclPool(Focl_Pool* pool, PoolItemFreeFunc freeFunc) /* Distinguish it from FoclPoolFree() */
{
    size_t count = pool->blockCount;
    for (size_t i = 0; i < count; i++)
    {
        freeFoclPoolBlock(pool->blocks[i], freeFunc);
    }
    free(pool->blocks);
    free(pool);
}
void FoclPoolBlockExpand(Focl_Pool* pool, size_t newBlockCount, PoolItemInitFunc initFunc)
{
    pool->blocks = (Focl_PoolBlock**)realloc(pool->blocks, sizeof(Focl_PoolBlock*) * newBlockCount);
    size_t iS = pool->itemSize;
    size_t iPB = pool->itemPerBlock;
    for (size_t i = pool->blockCount; i < newBlockCount; i++)
    {
        pool->blocks[i] = createFoclPoolBlock(iPB, iS, initFunc);
    }
    pool->blockCount = newBlockCount;
}
static void FoclPoolBlockDouble(Focl_Pool* pool, PoolItemInitFunc initFunc)
{
    FoclPoolBlockExpand(pool, pool->blockCount * 2, initFunc);
}
void* FoclPoolAlloc(Focl_Pool* pool, PoolItemAllocResetFunc resetFunc)
{
    for (size_t i = 0; i < pool->blockCount; i++)
    {
        Focl_PoolBlock* block = pool->blocks[i];
        if (block->freeTop > 0)
        {
            block->freeTop--;
            size_t idx = block->freeStack[block->freeTop];
            void* obj = (uint8_t*)block->data + idx * pool->itemSize;
            if (resetFunc)
            {
                resetFunc(obj);
            }
            return obj;
        }
    }
    return NULL;
}

void* FoclPoolAllocEx(Focl_Pool* pool, PoolItemInitFunc initFunc, PoolItemAllocResetFunc resetFunc)
{
    for (size_t i = 0; i < pool->blockCount; i++)
    {
        Focl_PoolBlock* block = pool->blocks[i];
        if (block->freeTop > 0)
        {
            block->freeTop--;
            size_t idx = block->freeStack[block->freeTop];
            void* obj = (uint8_t*)block->data + idx * pool->itemSize;
            if (resetFunc != NULL)
            {
                resetFunc(obj);
            }
            return obj;
        }
    }

    size_t oldCount = pool->blockCount;
    FoclPoolBlockDouble(pool, initFunc);

    Focl_PoolBlock* newBlock = pool->blocks[oldCount];
    newBlock->freeTop--;
    size_t idx = newBlock->freeStack[newBlock->freeTop];
    void* obj = (uint8_t*)newBlock->data + idx * pool->itemSize;
    if (resetFunc != NULL)
    {
        resetFunc(obj);
    }
    return obj;
}

void FoclPoolFree(Focl_Pool* pool, void* obj) /* Distinguish it from freeFoclPool() */
{
    for (size_t i = 0; i < pool->blockCount; i++)
    {
        Focl_PoolBlock* block = pool->blocks[i];
        uint8_t* data = (uint8_t*)block->data;
        size_t maxOffset = pool->itemSize * pool->itemPerBlock;

        if ((uint8_t*)obj >= data && (uint8_t*)obj < data + maxOffset)
        {
            size_t idx = ((uint8_t*)obj - data) / pool->itemSize;
            block->freeStack[block->freeTop++] = idx;
            return;
        }
    }
}

/* POOL */

/* STRING POOL */

static void initFoclStringAsVoid(void* str)
{
    initFoclString((Focl_String*)str, FOCL_STRING_INIT_CAPACITY);
}
static void FoclStrClearAsVoid(void* str)
{
    FoclStrClear((Focl_String*)str);
}

static Focl_StringPool* createFoclStringPool()
{
    return createFoclPool(sizeof(Focl_String), FOCL_STRING_POOL_ITEM_PER_BLOCK, FOCL_STRING_POOL_BLOCK_COUNT_INIT, initFoclStringAsVoid);
}
static void freeFoclStringPool(Focl_StringPool* strPool)
{
    freeFoclPool(strPool, freeFoclStringDataAsVoid);
}
static Focl_String* FoclStringPoolAlloc(Focl_StringPool* strPool) /* It will alloc string and clear it. */
{
    Focl_String* str = (Focl_String*)FoclPoolAllocEx(strPool, initFoclStringAsVoid, FoclStrClearAsVoid);
    return str;
}
static void FoclStringPoolFree(Focl_StringPool* strPool, Focl_String* str)
{
    FoclPoolFree(strPool, (void*)str);
} 

/* STRING POOL */

/* VECTOR POOL */

void initFoclVectorAsVoid(void* vec)
{
    initFoclVector(vec, sizeof(Focl_Object*), FOCL_VECTOR_INIT_CAPACITY);
}
void FoclVectorClearAsVoid(void* vec)
{
    FoclVectorClear((Focl_Vector*)vec);
}

static Focl_VectorPool* createFoclVectorPool()
{
    return createFoclPool(sizeof(Focl_Vector), FOCL_VECTOR_POOL_ITEM_PER_BLOCK, FOCL_VECTOR_POOL_BLOCK_COUNT_INIT, initFoclVectorAsVoid);
}
static void freeFoclVectorDataAsVoid(void* vec)
{
    freeFoclVectorData((Focl_Vector*)vec, NULL);
}
static void freeFoclVectorPool(Focl_VectorPool* vecPool)
{
    freeFoclPool(vecPool, freeFoclVectorDataAsVoid);
}
static Focl_Vector* FoclVectorPoolAlloc(Focl_VectorPool* vecPool)
{
    Focl_Vector* vec = (Focl_Vector*)FoclPoolAllocEx(vecPool, initFoclVectorAsVoid, FoclVectorClearAsVoid);
    FoclVectorClear(vec);
    return vec;
}
static void FoclVectorPoolFree(Focl_VectorPool* vecPool, Focl_Vector* vec)
{
    FoclPoolFree(vecPool, (void*)vec);
} 

/* VECTOR POOL */

/* VAR */

#define FOCL_OBJECT_ERROR NULL

#define FOCL_OBJ_TYPE_INT 0
#define FOCL_OBJ_TYPE_FLOAT 1
#define FOCL_OBJ_TYPE_BOOL 2
#define FOCL_OBJ_TYPE_VOID 3
#define FOCL_OBJ_TYPE_ERROR 4
#define FOCL_OBJ_TYPE_BYTECODE 5
#define FOCL_OBJ_TYPE_STR 6
#define FOCL_OBJ_TYPE_BLOCK 7
#define FOCL_OBJ_TYPE_COMPLEX 8

/* ERROR MESSAGE */

#define FOCL_ERR_INVALID_ARG "Invalid argument"
#define FOCL_ERR_UNSUPPORTED_ARG_COUNT "Unsupported argument counts"
#define FOCL_ERR_CANNOT_FIND_OBJECT "Cannot find object"
#define FOCL_ERR_UNCLOSED_CURLY_BRACKET "Unclosed curly bracket"
#define FOCL_ERR_UNCLOSED_SQUARE_BRACKET "Unclosed square bracket"
#define FOCL_ERR_UNKNOWN_COMMAND "Unknown command"
#define FOCL_ERR_WRONG_TYPE_ASSIGNMENT "Wrong type in assignment"
#define FOCL_ERR_YSNBH "You should not be here"

/* ERROR MESSAGE */

static Focl_Obj_Int FoclObjectUnboxInt(Focl_Object* obj)
{
    return (obj->as.i);
}
static Focl_Obj_Float FoclObjectUnboxFloat(Focl_Object* obj)
{
    return (obj->as.f);
}
static Focl_String* FoclObjectGetString(Focl_Object* obj)
{
    return (obj->as.data);
}
static void FoclObjectBoxInt(Focl_Object* obj, Focl_Obj_Int i_)
{
    obj->as.i = i_;
}
static void FoclObjectBoxFloat(Focl_Object* obj, Focl_Obj_Float f_)
{
    obj->as.f = f_;
}
static bool isFoclObjectUseString(Focl_Object* obj)
{
    return (obj->type >= FOCL_OBJ_TYPE_VOID);
}

Focl_Obj_Int Focl_StrToInt_View(const Focl_StringView* strView)
{
    char* savedPos = strView->strPtr + strView->len;
    char saved = *savedPos;
    *savedPos = '\0';
    Focl_Obj_Int i = Focl_StrToInt(strView->strPtr);
    *savedPos = saved;
    return i;
}
Focl_Obj_Float Focl_StrToFloat_View(const Focl_StringView* strView)
{
    char* savedPos = strView->strPtr + strView->len;
    char saved = *savedPos;
    *savedPos = '\0';
    Focl_Obj_Float f = Focl_StrToFloat(strView->strPtr);
    *savedPos = saved;
    return f;
}

static Focl_Object* createFoclObject(Focl_Obj_Type type_)
{
    Focl_Object* obj = (Focl_Object*)malloc(sizeof(Focl_Object));
    obj->refCount = 1;
    obj->type = type_;
    return obj;
}
static Focl_Object* createStringFoclObject(Focl_Obj_Type type_, Focl_StringPool* strPool)
{
    Focl_Object* obj = (Focl_Object*)malloc(sizeof(Focl_Object));
    obj->refCount = 1;
    obj->type = type_;
    obj->as.data = FoclStringPoolAlloc(strPool);
    return obj;
}
void FoclObjectAssign(Focl_Object* dst, Focl_Object* src, Focl_StringPool* strPool)
{
    /* This is a strong type language! */

    if (isFoclObjectUseString(src))
    {
        FoclStringPoolFree(strPool, dst->as.data);
        dst->as.data = FoclStringPoolAlloc(strPool);
        FoclStrAssignStr(dst->as.data, src->as.data);
    }
    else
    {
        dst->as = src->as;
    }
}
static Focl_Object* FoclObjectError(Focl_StringPool* strPool, const char* errmsg)
{
    Focl_Object* obj = createStringFoclObject(FOCL_OBJ_TYPE_ERROR, strPool);
    FoclStrAssign(obj->as.data, errmsg);
    return obj;
}
Focl_Object* createFoclObjectAssign(Focl_StringPool* strPool, Focl_Object* src)
{
    Focl_Object* obj = (Focl_Object*)malloc(sizeof(Focl_Object));
    obj->refCount = 1;
    obj->type = src->type;
    if (isFoclObjectUseString(src))
    {
        obj->as.data = FoclStringPoolAlloc(strPool);
        FoclStrAssignStr(obj->as.data, src->as.data);
    }
    else
    {
        obj->as = src->as;
    }
    return obj;
}
Focl_Object* getFoclObjectWithStringView(Focl_Context* context, const Focl_StringView* strView)
{
    /*
     * People may concern that why I need to pass both context and strPool arguments.
     * Well, that's because I just doesn't want to resolve the strPool pointer for 
     * so many times.
     */
    Focl_Object* obj;
    Focl_StringPool* strPool = context->strPool;
    if (Focl_isInteger_View(strView))
    {
        obj = createFoclObject(FOCL_OBJ_TYPE_INT);
        FoclObjectBoxInt(obj, Focl_StrToInt_View(strView));
    }
    else if (Focl_isFloat_View(strView))
    {
        obj = createFoclObject(FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(obj, Focl_StrToFloat_View(strView));
    }
    else if (Focl_isBlock_View(strView))
    {
        obj = createStringFoclObject(FOCL_OBJ_TYPE_BLOCK, strPool);
        FoclStrAssignView(obj->as.data, strView);
    }
    else if (Focl_isString_View(strView))
    {
        obj = createStringFoclObject(FOCL_OBJ_TYPE_STR, strPool);
        FoclStrExpansion(context, obj->as.data, strView);
    }
    else if (Focl_isVarSubstition_View(strView))
    {
        Focl_StringView varStrView = {strView->len - 1, strView->strPtr + 1};
        obj = FindObjectInContext(context, &varStrView);
        if (obj == FOCL_OBJECT_ERROR)
        {
            return FoclObjectError(strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
        }
        FoclObjectRetain(obj);
        return obj;
    }
    else if (Focl_isCmdSubstition_View(strView))
    {
        Focl_StringView cmdStrView = {strView->len - 2, strView->strPtr + 1};
        return Focl_parseCommand(context, &cmdStrView);
    }
    else
    {
        obj = createStringFoclObject(FOCL_OBJ_TYPE_STR, strPool);
        FoclStrAssignView(obj->as.data, strView);
    }
    return obj;
}
static Focl_Object* FoclObjectVoid(Focl_StringPool* strPool)
{
    return createStringFoclObject(FOCL_OBJ_TYPE_VOID, strPool);
}
static Focl_Object* FoclObjectBool(Focl_Obj_Bool booleanValue)
{
    Focl_Object* obj = createFoclObject(FOCL_OBJ_TYPE_BOOL);
    obj->as.i = booleanValue;
    return obj;
}
static void freeFoclObject(Focl_Object* obj, Focl_StringPool* strPool)
{
    if (isFoclObjectUseString(obj))
    {
        FoclStringPoolFree(strPool, obj->as.data);
    }
    free(obj);
}

static void FoclObjectRetain(Focl_Object* obj)
{
    if (obj != NULL)
    {
        obj->refCount++;
    }
}
static void FoclObjectRelease(Focl_Object* obj, Focl_StringPool* strPool)
{
    obj->refCount--;
    if (obj->refCount == 0)
    {
        freeFoclObject(obj, strPool);
    }
}

/* OBJECT TABLE */

#define FOCL_OBJ_TABLE_INIT_CAPACITY 64
#define FOCL_OBJ_TABLE_LOAD_FACTOR 0.75f

static Focl_ObjTable* createFoclObjTable()
{
    return createFoclHashTable(FOCL_OBJ_TABLE_INIT_CAPACITY, FOCL_OBJ_TABLE_LOAD_FACTOR, hashDjb2AsArg);
}
static Focl_Object* FindObjectInTable(Focl_ObjTable* objTable, const Focl_String* strView)
{
    return (Focl_Object*)FoclHashTableFind(objTable, (void*)strView, StrKeyCompare);
}
static void LinkObjectWithName_View(Focl_ObjTable* objTable, Focl_StringPool* strPool, Focl_Object* obj, const Focl_StringView* strView)
{
    Focl_String* str = FoclStringPoolAlloc(strPool);
    FoclStrAssignView(str, strView);
    FoclHashTableInsert(objTable, (void*)str, (void*)obj, StrKeyCompare, NULL);
}
static void freeFoclObjTable(Focl_ObjTable* objTable)
{
    freeFoclHashTable(objTable, NULL, NULL); /* The items should be free by their pool. */
}

/* OBJECT TABLE */

/* COMMAND TABLE */

#define FOCL_COMMAND_ERROR NULL

Focl_Command* createFoclCommand(Focl_CommandFunc cmdFunc)
{
    Focl_Command* cmd = (Focl_Command*)malloc(sizeof(Focl_Command));
    cmd->func = cmdFunc;
    return cmd;
}
static void freeFoclCommand(Focl_Command* cmd)
{
    free(cmd);
}
static void freeFoclCommandAsVoid(void* cmd)
{
    freeFoclCommand((Focl_Command*)cmd);
}

#define FOCL_COMMAND_TABLE_INIT_CAPACITY 128
#define FOCL_COMMAND_TABLE_LOAD_FACTOR 0.85f

static Focl_CommandTable* createFoclCommandTable()
{
    return createFoclHashTable(FOCL_COMMAND_TABLE_INIT_CAPACITY, FOCL_COMMAND_TABLE_LOAD_FACTOR, hashDjb2AsArg);
}
static Focl_Command* FindCommandInTable(Focl_CommandTable* cmdTable, const Focl_String* str)
{
    return (Focl_Command*)FoclHashTableFind(cmdTable, (void*)str, StrKeyCompare);
}
static void freeFoclCommandTable(Focl_CommandTable* cmdTable)
{
    freeFoclHashTable(cmdTable, NULL, freeFoclCommandAsVoid);
}

/* COMMAND TABLE */

/* REGISTER */

Focl_Registers* createFoclRegisters(size_t regCount_)
{
    Focl_Registers* regs = (Focl_Registers*)malloc(sizeof(Focl_Registers));
    regs->r = (Focl_Register*)malloc(sizeof(Focl_Register) * regCount_);
    regs->regCount = regCount_;
    return regs;
}
void freeFoclRegisters(Focl_Registers* regs)
{
    free(regs->r);
    free(regs);
}

/* REGISTER */

/* ENVIRONMENT */

Focl_Environment* createFoclEnvironment(Focl_Environment* parent_) /* If parent_ is NULL, it will create a root(or global) environment. */
{
    Focl_Environment* env = (Focl_Environment*)malloc(sizeof(Focl_Environment));
    env->parent = parent_;
    if (parent_ == NULL)
    {
        env->level = 0;
    }
    else
    {
        env->level = parent_->level + 1;
    }
    env->cmdTable = createFoclCommandTable();
    env->objTable = createFoclObjTable();
    return env;
}
static void freeFoclEnvironment(Focl_Environment* env)
{
    freeFoclCommandTable(env->cmdTable);
    freeFoclObjTable(env->objTable);
    free(env);
}

/* ENVIRONMENT */

/* CONTEXT */

Focl_Context* createFoclContext()
{
    Focl_Context* context = (Focl_Context*)malloc(sizeof(Focl_Context));
    context->globalEnv = createFoclEnvironment(NULL);
    context->curEnv = context->globalEnv;
    context->regs = createFoclRegisters(FOCL_REGISTERS_INIT_SIZE);
    context->strPool = createFoclStringPool();
    context->vecPool = createFoclVectorPool();
    return context;
}
void freeFoclContext(Focl_Context* context)
{
    Focl_Environment* cEnv = context->curEnv;
    Focl_Environment* pEnv;
    do
    {
        pEnv = cEnv->parent;
        freeFoclEnvironment(cEnv);
        cEnv = pEnv;
    }
    while (cEnv != NULL);
    freeFoclRegisters(context->regs);
    freeFoclStringPool(context->strPool);
    freeFoclVectorPool(context->vecPool);
    free(context);
}

Focl_Object* FindObjectInContext(Focl_Context* context, Focl_StringView* strView)
{
    Focl_Environment* cEnv = context->curEnv;
    Focl_Object* obj;
    Focl_String tmpStr;
    char saved = initTempFoclStringWithView(&tmpStr, strView);
    while (cEnv != NULL)
    {
        obj = FindObjectInTable(cEnv->objTable, &tmpStr);
        if (obj != FOCL_OBJECT_ERROR)
        {
            restoreFoclStringViewFromTempString(strView, saved);
            return obj;
        }
        cEnv = cEnv->parent;
    }
    restoreFoclStringViewFromTempString(strView, saved);
    return FOCL_OBJECT_ERROR;
}

Focl_Command* FindCommandInContext(Focl_Context* context, const Focl_StringView* strView)
{
    /* It will search in basic command(global command) first. */
    Focl_Environment* gEnv = context->globalEnv;
    Focl_String tmpStr;
    char saved = initTempFoclStringWithView(&tmpStr, strView);
    Focl_Command* cmd = FindCommandInTable(gEnv->cmdTable, &tmpStr);
    if (cmd != FOCL_COMMAND_ERROR)
    {
        restoreFoclStringViewFromTempString(strView, saved);
        return cmd;
    }

    Focl_Environment* cEnv = context->curEnv;
    while (cEnv != gEnv)
    {
        cmd = FindCommandInTable(cEnv->cmdTable, &tmpStr);
        if (cmd != FOCL_COMMAND_ERROR)
        {
            restoreFoclStringViewFromTempString(strView, saved);
            return cmd;
        }
        cEnv = cEnv->parent;
    }
    restoreFoclStringViewFromTempString(strView, saved);
    return FOCL_COMMAND_ERROR;
}

/* CONTEXT */

Focl_StringView getNextWord(const Focl_StringView* src, Focl_StringView* start)
{
    const char* ptr = start->strPtr;
    const char* srcEnd = src->strPtr + src->len;
    const char* startEnd = start->strPtr + start->len;
    
    while (ptr < startEnd && isspace(*ptr))
    {
        ptr++;
    }
    if (ptr >= startEnd || *ptr == '\0')
    {
        start->strPtr = startEnd;
        start->len = 0;
        return (Focl_StringView){0, NULL};
    }
    
    const char* wordStart = ptr;
    
    if (*ptr == '"')
    {
        ptr++;
        while (ptr < startEnd && *ptr != '"')
        {
            if (*ptr == '\\' && (ptr + 1) < startEnd)
                ptr++;
            ptr++;
        }
        if (ptr < startEnd)
            ptr++;
    }
    else if (*ptr == '[')
    {
        int depth = 1;
        ptr++;
        while (ptr < startEnd && depth > 0)
        {
            if (*ptr == '\\' && (ptr + 1) < startEnd)
            {
                ptr += 2;
            }
            else if (*ptr == '[')
            {
                depth++, ptr++;
            }
            else if (*ptr == ']')
            {
                depth--, ptr++;
            }
            else
            {
                ptr++;
            }
        }
    }
    else if (*ptr == '{')
    {
        int depth = 1;
        ptr++;
        while (ptr < startEnd && depth > 0)
        {
            if (*ptr == '\\' && (ptr + 1) < startEnd)
            {
                ptr += 2;
            }
            else if (*ptr == '{')
            {
                depth++, ptr++;
            }
            else if (*ptr == '}')
            {
                depth--, ptr++;
            }
            else
            {
                ptr++;
            }
        }
    }
    else
    {
        while (ptr < startEnd && !isspace(*ptr) && *ptr != ';')
        {
            ptr++;
        }
    }
    
    size_t wordLen = ptr - wordStart;
    start->strPtr = ptr;
    start->len = startEnd - ptr;
    return (Focl_StringView){wordLen, (char*)wordStart};
}
Focl_StringView getNextLine(Focl_StringView* start)
{
    const char* ptr = start->strPtr;
    const char* startEnd = start->strPtr + start->len;
    
    while (ptr < startEnd && isspace(*ptr))
    {
        ptr++;
    }
    if (ptr >= startEnd || *ptr == '\0')
    {
        start->strPtr = startEnd;
        start->len = 0;
        return (Focl_StringView){0, NULL};
    }
    
    const char* wordStart = ptr;
    bool inString = false;
    int braceDepth = 0;
    int bracketDepth = 0;
    
    while (ptr < startEnd)
    {
        if (*ptr == '\\' && (inString || braceDepth > 0 || bracketDepth > 0))
        {
            ptr++;
            if (ptr < startEnd)
                ptr++;
            continue;
        }
        if (*ptr == '"' && braceDepth == 0 && bracketDepth == 0)
        {
            inString = !inString;
            ptr++;
            continue;
        }
        if (!inString)
        {
            if (*ptr == '{')
            {
                braceDepth++;
                ptr++;
                continue;
            }
            if (*ptr == '}')
            {
                if (braceDepth > 0)
                    braceDepth--;
                ptr++;
                continue;
            }
            if (*ptr == '[')
            {
                bracketDepth++;
                ptr++;
                continue;
            }
            if (*ptr == ']')
            {
                if (bracketDepth > 0)
                    bracketDepth--;
                ptr++;
                continue;
            }
        }
        if (!inString && braceDepth == 0 && bracketDepth == 0)
        {
            if (*ptr == '\n' || *ptr == ';')
            {
                break;
            }
        }
        
        ptr++;
    }
    size_t wordLen = ptr - wordStart;
    if (ptr < startEnd)
    {
        ptr++;
    }
    start->strPtr = ptr;
    start->len = startEnd - ptr;
    
    return (Focl_StringView){wordLen, (char*)wordStart};
}
static bool isWordParseEnd(const Focl_StringView* strView, const char* pos)
{
    return (pos >= strView->strPtr + strView->len);
}

/* STRING EXPANSION */

void FoclStrExpansion(Focl_Context* context, Focl_String* dst, const Focl_StringView* src)
{
    const char* readPos = src->strPtr;
    const char* strEnd = src->strPtr + src->len;

    bool hasQuotes = false;
    
    if (src->len >= 2 && src->strPtr[0] == '"' && src->strPtr[src->len - 1] == '"')
    {
        hasQuotes = true;
        readPos++;
        strEnd--;
    }

    while (readPos < strEnd)
    {
        if (*readPos == '\\' && hasQuotes)
        {
            readPos++;

            if (readPos >= strEnd)
            {
                break;
            }

            char ch;
            switch (*readPos)
            {
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case '\\': ch = '\\'; break;
                case '$': ch = '$'; break;
                case '[': ch = '['; break;
                case ']': ch = ']'; break;
                case '"': ch = '"'; break;
                default: ch = *readPos; break;
            }

            size_t curLen = dst->length;
            if (curLen + 1 >= dst->capacity)
            {
                FoclStrReserve(dst, (dst->capacity == 0) ? 16 : dst->capacity * 2);
            }
            dst->data[curLen] = ch;
            dst->length = curLen + 1;
            dst->data[dst->length] = '\0';

            readPos++;
        }
        else if (*readPos == '$')
        {
            readPos++;

            if (readPos >= strEnd)
            {
                size_t curLen = dst->length;
                if (curLen + 1 >= dst->capacity)
                {
                    FoclStrReserve(dst, (dst->capacity == 0) ? 16 : dst->capacity * 2);
                }
                dst->data[curLen] = '$';
                dst->length = curLen + 1;
                dst->data[dst->length] = '\0';
                break;
            }

            const char* varStart = readPos;
            const char* varEnd = varStart;

            while (varEnd < strEnd)
            {
                int32_t cp = getUtf8CodePoint(varEnd);
                size_t cpLen = getUtf8CodePointLength((uint8_t)*varEnd);
                
                if (cp == 0) break;

                bool isIdent = ((cp >= 'a' && cp <= 'z') ||
                               (cp >= 'A' && cp <= 'Z') ||
                               (cp >= '0' && cp <= '9') ||
                               cp == '_');

                if (!isIdent) break;
                varEnd += cpLen;
            }

            if (varStart == varEnd)
            {
                size_t curLen = dst->length;
                if (curLen + 1 >= dst->capacity)
                {
                    FoclStrReserve(dst, (dst->capacity == 0) ? 16 : dst->capacity * 2);
                }
                dst->data[curLen] = '$';
                dst->length = curLen + 1;
                dst->data[dst->length] = '\0';
                readPos = varStart;
            }
            else
            {
                Focl_StringView varName = {(size_t)(varEnd - varStart), (char*)varStart};
                Focl_Object* objPtr = FindObjectInContext(context, &varName);

                if (objPtr != FOCL_OBJECT_ERROR)
                {
                    Focl_Object* object = (Focl_Object*)objPtr;
                    if (isFoclObjectUseString(object))
                    {
                        Focl_String* objStr = FoclObjectGetString(object);
                        if (objStr != NULL && objStr->data != NULL && objStr->length > 0)
                        {
                            size_t curLen = dst->length;
                            size_t needed = curLen + objStr->length;
                            while (needed >= dst->capacity)
                            {
                                FoclStrReserve(dst, (dst->capacity == 0) ? 16 : dst->capacity * 2);
                            }
                            memcpy(dst->data + curLen, objStr->data, objStr->length);
                            dst->length = curLen + objStr->length;
                            dst->data[dst->length] = '\0';
                        }
                    }
                    else
                    {
                        Focl_String tempStr;
                        switch (object->type)
                        {
                            case FOCL_OBJ_TYPE_INT:
                                initFoclString(&tempStr, FOCL_INT_TO_STR_TMP_BUFFER_SIZE);
                                tempStr.length = sprintf(tempStr.data, "%" FOCL_FORMAT_INT, object->as.i);
                                break;
                            case FOCL_OBJ_TYPE_FLOAT:
                                initFoclString(&tempStr, FOCL_FLOAT_TO_STR_TMP_BUFFER_SIZE);
                                tempStr.length = sprintf(tempStr.data, "%" FOCL_FORMAT_FLOAT, object->as.f);
                                break;
                            case FOCL_OBJ_TYPE_BOOL:
                                initFoclString(&tempStr, (object->as.i == FOCL_OBJ_TRUE) ? sizeof("true") : sizeof("false"));
                                tempStr.length = sprintf(tempStr.data, "%s", (object->as.i == FOCL_OBJ_TRUE) ? "true" : "false");
                                break;
                            default:
                                tempStr.length = 0;
                                break;
                        }
                        tempStr.data[tempStr.length] = '\0';
                        size_t curLen = dst->length;
                        size_t needed = curLen + tempStr.length;
                        while (needed >= dst->capacity)
                        {
                            FoclStrReserve(dst, (dst->capacity == 0) ? 16 : dst->capacity * 2);
                        }
                        memcpy(dst->data + curLen, tempStr.data, tempStr.length);
                        dst->length = curLen + tempStr.length;
                        dst->data[dst->length] = '\0';
                        freeFoclStringData(&tempStr);
                    }
                }
                readPos = varEnd;
            }
        }
        else if (*readPos == '[')
        {
            readPos++;

            const char* cmdStart = readPos;
            int32_t bracketDepth = 1;
            const char* cmdEnd = cmdStart;

            while (cmdEnd < strEnd && bracketDepth > 0)
            {
                if (*cmdEnd == '\\' && (cmdEnd + 1) < strEnd)
                {
                    cmdEnd += 2;
                }
                else if (*cmdEnd == '[')
                {
                    bracketDepth++;
                    cmdEnd++;
                }
                else if (*cmdEnd == ']')
                {
                    bracketDepth--;
                    if (bracketDepth > 0) cmdEnd++;
                }
                else
                {
                    cmdEnd++;
                }
            }

            if (bracketDepth == 0)
            {
                Focl_StringView cmdView = {(size_t)(cmdEnd - cmdStart), (char*)cmdStart};
                Focl_Object* cmdResult = Focl_parseCommand(context, &cmdView);

                if (cmdResult != NULL)
                {
                    if (cmdResult->type != FOCL_OBJ_TYPE_ERROR && isFoclObjectUseString(cmdResult))
                    {
                        Focl_String* outStr = FoclObjectGetString(cmdResult);
                        if (outStr != NULL && outStr->data != NULL && outStr->length > 0)
                        {
                            size_t curLen = dst->length;
                            size_t needed = curLen + outStr->length;
                            while (needed >= dst->capacity)
                            {
                                FoclStrReserve(dst, (dst->capacity == 0) ? 16 : dst->capacity * 2);
                            }
                            memcpy(dst->data + curLen, outStr->data, outStr->length);
                            dst->length = curLen + outStr->length;
                            dst->data[dst->length] = '\0';
                        }
                    }
                    FoclObjectRelease(cmdResult, context->strPool);
                }
                readPos = cmdEnd + 1;
            }
            else
            {
                size_t curLen = dst->length;
                if (curLen + 1 >= dst->capacity)
                {
                    FoclStrReserve(dst, (dst->capacity == 0) ? 16 : dst->capacity * 2);
                }
                dst->data[curLen] = '[';
                dst->length = curLen + 1;
                dst->data[dst->length] = '\0';
            }
        }
        else
        {
            size_t curLen = dst->length;
            if (curLen + 1 >= dst->capacity)
            {
                FoclStrReserve(dst, (dst->capacity == 0) ? 16 : dst->capacity * 2);
            }
            dst->data[curLen] = *readPos;
            dst->length = curLen + 1;
            dst->data[dst->length] = '\0';
            readPos++;
        }
    }
}

/* STRING EXPANSION */

/* EXPRESSION */

Focl_Object* Focl_exprBool(Focl_Context* context, const Focl_StringView* strView)
{
    const char* start = strView->strPtr;
    const char* end = start + strView->len;
    while (start < end && isspace(*start)) start++;
    while (end > start && isspace(*(end - 1))) end--;
    size_t len = end - start;
    if (len == 0)
    {
        return FoclObjectError(context->strPool, "Empty boolean expression");
    }

    Focl_StringView trimmed = {len, (char*)start};

    if (trimmed.strPtr[0] == '!')
    {
        Focl_StringView inner = {trimmed.len - 1, trimmed.strPtr + 1};
        Focl_Object* innerObj = Focl_exprBool(context, &inner);
        if (innerObj->type == FOCL_OBJ_TYPE_ERROR) return innerObj;
        Focl_Obj_Bool val = innerObj->as.i;
        FoclObjectRelease(innerObj, context->strPool);
        return FoclObjectBool(!val);
    }

    if (trimmed.strPtr[0] == '(' && trimmed.strPtr[trimmed.len - 1] == ')')
    {
        Focl_StringView inner = {trimmed.len - 2, trimmed.strPtr + 1};
        return Focl_exprBool(context, &inner);
    }

    int depth = 0;
    for (size_t i = 0; i < trimmed.len; i++)
    {
        char c = trimmed.strPtr[i];
        if (c == '[' || c == '(' || c == '{') depth++;
        else if (c == ']' || c == ')' || c == '}') depth--;
        else if (depth == 0)
        {
            if (c == '&' && i + 1 < trimmed.len && trimmed.strPtr[i + 1] == '&')
            {
                Focl_StringView left = {i, trimmed.strPtr};
                Focl_StringView right = {trimmed.len - i - 2, trimmed.strPtr + i + 2};

                Focl_Object* leftObj = Focl_exprBool(context, &left);
                if (leftObj->type == FOCL_OBJ_TYPE_ERROR) return leftObj;
                Focl_Obj_Bool lv = leftObj->as.i;
                FoclObjectRelease(leftObj, context->strPool);
                if (!lv) return FoclObjectBool(FOCL_OBJ_FALSE);

                Focl_Object* rightObj = Focl_exprBool(context, &right);
                if (rightObj->type == FOCL_OBJ_TYPE_ERROR) return rightObj;
                Focl_Obj_Bool rv = rightObj->as.i;
                FoclObjectRelease(rightObj, context->strPool);
                return FoclObjectBool(rv);
            }
            if (c == '|' && i + 1 < trimmed.len && trimmed.strPtr[i + 1] == '|')
            {
                Focl_StringView left = {i, trimmed.strPtr};
                Focl_StringView right = {trimmed.len - i - 2, trimmed.strPtr + i + 2};

                Focl_Object* leftObj = Focl_exprBool(context, &left);
                if (leftObj->type == FOCL_OBJ_TYPE_ERROR) return leftObj;
                Focl_Obj_Bool lv = leftObj->as.i;
                FoclObjectRelease(leftObj, context->strPool);
                if (lv) return FoclObjectBool(FOCL_OBJ_TRUE);

                Focl_Object* rightObj = Focl_exprBool(context, &right);
                if (rightObj->type == FOCL_OBJ_TYPE_ERROR) return rightObj;
                Focl_Obj_Bool rv = rightObj->as.i;
                FoclObjectRelease(rightObj, context->strPool);
                return FoclObjectBool(rv);
            }
        }
    }

    const char* ops[] = {"==", "!=", "<=", ">=", "<", ">"};
    for (int k = 0; k < 6; k++)
    {
        size_t opLen = strlen(ops[k]);
        depth = 0;
        for (size_t i = 0; i + opLen <= trimmed.len; i++)
        {
            char c = trimmed.strPtr[i];
            if (c == '[' || c == '(' || c == '{') depth++;
            else if (c == ']' || c == ')' || c == '}') depth--;
            else if (depth == 0 && memcmp(trimmed.strPtr + i, ops[k], opLen) == 0)
            {
                Focl_StringView left = {i, trimmed.strPtr};
                Focl_StringView right = {trimmed.len - i - opLen, trimmed.strPtr + i + opLen};

                while (left.len > 0 && isspace(*left.strPtr)) { left.strPtr++; left.len--; }
                while (left.len > 0 && isspace(left.strPtr[left.len - 1])) left.len--;
                while (right.len > 0 && isspace(*right.strPtr)) { right.strPtr++; right.len--; }
                while (right.len > 0 && isspace(right.strPtr[right.len - 1])) right.len--;

                if (left.len == 0 || right.len == 0)
                    return FoclObjectError(context->strPool, "Missing operand in comparison");

                Focl_Object* leftObj = getFoclObjectWithStringView(context, &left);
                Focl_Object* rightObj = getFoclObjectWithStringView(context, &right);

                if (leftObj->type == FOCL_OBJ_TYPE_ERROR || rightObj->type == FOCL_OBJ_TYPE_ERROR)
                {
                    if (leftObj->type == FOCL_OBJ_TYPE_ERROR) FoclObjectRelease(leftObj, context->strPool);
                    if (rightObj->type == FOCL_OBJ_TYPE_ERROR) FoclObjectRelease(rightObj, context->strPool);
                    return FoclObjectError(context->strPool, "Invalid operand in comparison");
                }

                if (leftObj->type != FOCL_OBJ_TYPE_INT || rightObj->type != FOCL_OBJ_TYPE_INT)
                {
                    FoclObjectRelease(leftObj, context->strPool);
                    FoclObjectRelease(rightObj, context->strPool);
                    return FoclObjectError(context->strPool, "Comparison operands must be integers");
                }

                Focl_Obj_Int l = leftObj->as.i;
                Focl_Obj_Int r = rightObj->as.i;
                Focl_Obj_Bool result = FOCL_OBJ_FALSE;
                switch (k)
                {
                    case 0: result = (l == r); break;
                    case 1: result = (l != r); break;
                    case 2: result = (l <= r); break;
                    case 3: result = (l >= r); break;
                    case 4: result = (l < r); break;
                    case 5: result = (l > r); break;
                }

                FoclObjectRelease(leftObj, context->strPool);
                FoclObjectRelease(rightObj, context->strPool);
                return FoclObjectBool(result);
            }
        }
    }

    Focl_Object* obj = getFoclObjectWithStringView(context, &trimmed);

    if (obj->type == FOCL_OBJ_TYPE_ERROR)
    {
        return obj;
    }

    if (obj->type != FOCL_OBJ_TYPE_BOOL)
    {
        FoclObjectRelease(obj, context->strPool);
        return FoclObjectError(context->strPool, "Expected boolean expression, got non-boolean value");
    }

    Focl_Obj_Bool result = obj->as.i;
    FoclObjectRelease(obj, context->strPool);
    return FoclObjectBool(result);
}

/* EXPRESSION */

static Focl_Object* FoclObjVecAt(Focl_Vector* objVec, size_t idx)
{
    return *(Focl_Object**)FoclVectorAtNoCheck(objVec, idx);
}
static Focl_String* FoclObjVecAtAsString(Focl_Vector* objVec, size_t idx)
{
    return (*(Focl_Object**)FoclVectorAtNoCheck(objVec, idx))->as.data;
}
static Focl_StringView FoclObjVecAtAsStringToView(Focl_Vector* objVec, size_t idx)
{
    Focl_String* str = FoclObjVecAtAsString(objVec, idx);
    Focl_StringView strView = {str->length, str->data};
    return strView;
}

static Focl_Object* Focl_parseBlock(Focl_Context* context, Focl_StringView* strView)
{
    Focl_StringView inner = FoclStringViewPeelBoth(strView);
    return Focl_parseCommandSequence(context, &inner);
}
Focl_Object* Focl_parseCommand(Focl_Context* context, const Focl_StringView* strView)
{
    Focl_StringView remaining = *strView;
    Focl_StringView cmdView = getNextWord(strView, &remaining);
    if (cmdView.strPtr == NULL || cmdView.len == 0)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNKNOWN_COMMAND);
    }
    Focl_Command* cmd = FindCommandInContext(context, &cmdView);
    if (cmd == FOCL_COMMAND_ERROR)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNKNOWN_COMMAND);
    }
    Focl_Vector* vec = FoclVectorPoolAlloc(context->vecPool);
    Focl_Object* obj;
    while (remaining.len > 0)
    {
        Focl_StringView argView = getNextWord(strView, &remaining);
        if (argView.strPtr == NULL || argView.len == 0)
        {
            break;
        }
        obj = getFoclObjectWithStringView(context, &argView);
        FoclVectorPushBack(vec, &obj);
    }
    obj = cmd->func(context, vec);
    for (size_t i = 0; i < FoclVectorGetSize(vec); i++)
    {
        FoclObjectRelease(FoclObjVecAt(vec, i), context->strPool);
    }
    FoclVectorPoolFree(context->vecPool, vec);
    return obj;
}
Focl_Object* Focl_parseCommandSequence(Focl_Context* context, Focl_StringView* strView)
{
    Focl_StringView remaining = *strView;
    Focl_Object* lastResult = FoclObjectVoid(context->strPool);
    while (1)
    {
        Focl_StringView cmdView = getNextLine(&remaining);
        if (remaining.len == 0)
        {
            if (cmdView.strPtr == NULL || cmdView.len == 0)
            {
                break;
            }
        }
        if (cmdView.strPtr == NULL || cmdView.len == 0)
        {
            continue;
        }
        Focl_Object* result = Focl_parseCommand(context, &cmdView);
        FoclObjectRelease(lastResult, context->strPool);
        lastResult = result;
        if (result->type == FOCL_OBJ_TYPE_ERROR)
        {
            break;
        }
    }
    return lastResult;
}

static Focl_Object* Focl_parseLine(Focl_Context* context, Focl_String* lineStr)
{
    Focl_StringView strView = {lineStr->length, lineStr->data};
    return Focl_parseCommandSequence(context, &strView);
}

/* BUILTIN COMMAND */

#define FOCL_ERR_INVALID_ARG "Invalid argument"
#define FOCL_ERR_UNSUPPORTED_ARG_COUNT "Unsupported argument counts"
#define FOCL_ERR_CANNOT_FIND_OBJECT "Cannot find object"
#define FOCL_ERR_NO_EXEC_BLOCK "No block to execute when the if command is true"
#define FOCL_ERR_UNKNOWN_ARG "Unknown argument"
#define FOCL_ERR_READ_ERR_STDIN "EOF or read error on stdin"
#define FOCL_ERR_MUST_BE_BLOCK "For arguments must be blocks"

void FoclObjectPrint(Focl_Object* obj)
{
    switch (obj->type)
    {
        case FOCL_OBJ_TYPE_INT:
            printf("%" FOCL_FORMAT_INT, obj->as.i);
            break;
        case FOCL_OBJ_TYPE_FLOAT:
            printf("%" FOCL_FORMAT_FLOAT, obj->as.f);
            break;
        case FOCL_OBJ_TYPE_BOOL:
            printf("%s", obj->as.i ? "true" : "false");
            break;
        case FOCL_OBJ_TYPE_STR: /* FALLTHROUGH */
        case FOCL_OBJ_TYPE_BLOCK: /* FALLTHROUGH */
        case FOCL_OBJ_TYPE_ERROR:
            printf("%s", FoclStrCStr(obj->as.data));
            break;
        case FOCL_OBJ_TYPE_VOID:
            break;
        default:
            printf(FOCL_ERR_YSNBH);
            break;
    }
}
Focl_Object* FoclObjectScan(Focl_StringPool* strPool, Focl_Object* obj)
{
    size_t len;
    char* input = Focl_getline(stdin, &len);
    if (input == NULL)
    {
        return FoclObjectError(strPool, FOCL_ERR_READ_ERR_STDIN);
    }
    Focl_Object* result = obj;
    if (Focl_isInteger(input))
    {
        if (obj->type != FOCL_OBJ_TYPE_INT)
        {
            result = FoclObjectError(strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
        }
        else
        {
            obj->as.i = Focl_StrToInt(input);
        }
    }
    else if (Focl_isFloat(input))
    {
        if (obj->type != FOCL_OBJ_TYPE_FLOAT)
        {
            result = FoclObjectError(strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
        }
        else
        {
            obj->as.f = Focl_StrToFloat(input);
        }
    }
    else
    {
        if (obj->type != FOCL_OBJ_TYPE_STR)
        {
            result = FoclObjectError(strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
        }
        else
        {
            FoclStrAssign(obj->as.data, input);
        }
    }
    free(input);
    return result;
}

Focl_Object* buildIn_puts(Focl_Context* context, Focl_Vector* objVec)
{
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    FoclObjectPrint(FoclObjVecAt(objVec, 0));
    putchar('\n');
    return FoclObjectVoid(context->strPool);
}
Focl_Object* buildIn_gets(Focl_Context* context, Focl_Vector* objVec)
{
    if (FoclVectorGetSize(objVec) != 2)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView inPipe = FoclObjVecAtAsStringToView(objVec, 0);
    if (FoclStringViewComp(&inPipe, "stdin") != 0)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNKNOWN_ARG);
    }
    Focl_StringView varView = FoclObjVecAtAsStringToView(objVec, 1);
    Focl_Object* obj = FindObjectInContext(context, &varView);
    if (obj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    return createFoclObjectAssign(context->strPool, obj);
}
Focl_Object* buildIn_set(Focl_Context* context, Focl_Vector* objVec)
{
    if (FoclVectorGetSize(objVec) != 2)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView strView = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* obj = FindObjectInContext(context, &strView);
    if (obj == FOCL_OBJECT_ERROR)
    {
        /* Create it! */
        
        obj = createFoclObjectAssign(context->strPool, FoclObjVecAt(objVec, 1));
        LinkObjectWithName_View(context->curEnv->objTable, context->strPool, obj, &strView);
    }
    else
    {
        /* Change it! */

        /* I decided to make Focl into a dynamic but strong type language! */

        Focl_Object* src = FoclObjVecAt(objVec, 1);
        if (obj->type != src->type)
        {
            return FoclObjectError(context->strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
        }
        FoclObjectAssign(obj, src, context->strPool);
    }

    return createFoclObjectAssign(context->strPool, obj);
}
Focl_Object* buildIn_unset(Focl_Context* context, Focl_Vector* objVec)
{
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView strView = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* obj = FindObjectInContext(context, &strView);
    if (obj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    Focl_String tmpStr;
    char saved = initTempFoclStringWithView(&tmpStr, &strView);
    FoclHashTableDelete(context->curEnv->objTable, &tmpStr, StrKeyCompare, NULL, NULL);
    restoreFoclStringViewFromTempString(&strView, saved);
    FoclObjectRelease(obj, context->strPool);
    
    return FoclObjectVoid(context->strPool);
}
Focl_Object* buildIn_incr(Focl_Context* context, Focl_Vector* objVec)
{
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount == 0 || argCount > 2)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView strView = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* obj = FindObjectInContext(context, &strView);
    if (obj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    else if (obj->type != FOCL_OBJ_TYPE_INT)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
    }
    else
    {
        if (argCount == 1)
        {
            ++(obj->as.i);
        }
        else
        {
            obj->as.i += FoclObjVecAt(objVec, 1)->as.i;
        }
    }
    return createFoclObjectAssign(context->strPool, obj);
}
Focl_Object* buildIn_if(Focl_Context* context, Focl_Vector* objVec)
{
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount < 2)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }

    size_t i = 0;
    while (i < argCount)
    {
        Focl_StringView condBlock = FoclObjVecAtAsStringToView(objVec, i);
        Focl_StringView condExpr = FoclStringViewPeelBoth(&condBlock);
        Focl_Object* condResult = Focl_exprBool(context, &condExpr);
        if (condResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            return condResult;
        }

        if (condResult->as.i == FOCL_OBJ_TRUE)
        {
            FoclObjectRelease(condResult, context->strPool);
            if (i + 1 >= argCount)
            {
                return FoclObjectError(context->strPool, FOCL_ERR_NO_EXEC_BLOCK);
            }
            Focl_StringView execBlock = FoclObjVecAtAsStringToView(objVec, i + 1);
            return Focl_parseBlock(context, &execBlock);
        }
        FoclObjectRelease(condResult, context->strPool);

        if (i + 2 >= argCount)
        {
            return FoclObjectVoid(context->strPool);
        }

        Focl_StringView next = FoclObjVecAtAsStringToView(objVec, i + 2);
        if (FoclStringViewComp(&next, "elseif") == 0)
        {
            if (i + 4 >= argCount)
            {
                return FoclObjectError(context->strPool, FOCL_ERR_NO_EXEC_BLOCK);
            }
            i += 2;
        }
        else if (FoclStringViewComp(&next, "else") == 0)
        {
            if (i + 3 >= argCount)
            {
                return FoclObjectError(context->strPool, FOCL_ERR_NO_EXEC_BLOCK);
            }
            Focl_StringView elseBlock = FoclObjVecAtAsStringToView(objVec, i + 3);
            return Focl_parseBlock(context, &elseBlock);
        }
        else
        {
            return FoclObjectError(context->strPool, FOCL_ERR_UNKNOWN_ARG);
        }
    }
    return FoclObjectError(context->strPool, FOCL_ERR_YSNBH);
}
Focl_Object* buildIn_while(Focl_Context* context, Focl_Vector* objVec)
{
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 2)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }

    Focl_StringView condBlock = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_StringView condExpr = FoclStringViewPeelBoth(&condBlock);
    Focl_Object* condResult = Focl_exprBool(context, &condExpr);
    if (condResult->type == FOCL_OBJ_TYPE_ERROR)
    {
        return condResult;
    }

    Focl_Object* result = FoclObjectVoid(context->strPool);

    while (condResult->as.i == FOCL_OBJ_TRUE)
    {
        FoclObjectRelease(condResult, context->strPool);
        Focl_StringView execBlock = FoclObjVecAtAsStringToView(objVec, 1);
        Focl_Object* bodyResult = Focl_parseBlock(context, &execBlock);
        if (bodyResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(result, context->strPool);
            return bodyResult;
        }
        FoclObjectRelease(result, context->strPool);
        result = bodyResult;
        condResult = Focl_exprBool(context, &condExpr);
        if (condResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(result, context->strPool);
            return condResult;
        }
    }

    FoclObjectRelease(condResult, context->strPool);
    return result;
}
Focl_Object* buildIn_for(Focl_Context* context, Focl_Vector* objVec)
{
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 4)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }

    for (size_t i = 0; i < 4; i++)
    {
        if (!isFoclObjectUseString(FoclObjVecAt(objVec, i)))
        {
            return FoclObjectError(context->strPool, FOCL_ERR_MUST_BE_BLOCK);
        }
    }
    Focl_StringView initBlock = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* initResult = Focl_parseBlock(context, &initBlock);
    if (initResult->type == FOCL_OBJ_TYPE_ERROR)
    {
        return initResult;
    }
    FoclObjectRelease(initResult, context->strPool);

    Focl_StringView condBlock = FoclObjVecAtAsStringToView(objVec, 1);
    Focl_StringView condExpr = FoclStringViewPeelBoth(&condBlock);
    Focl_StringView updateBlock = FoclObjVecAtAsStringToView(objVec, 2);
    Focl_StringView bodyBlock = FoclObjVecAtAsStringToView(objVec, 3);
    Focl_Object* result = FoclObjectVoid(context->strPool);

    while (1)
    {
        Focl_Object* condResult = Focl_exprBool(context, &condExpr);
        if (condResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(result, context->strPool);
            return condResult;
        }

        if (condResult->as.i != FOCL_OBJ_TRUE)
        {
            FoclObjectRelease(condResult, context->strPool);
            break;
        }
        FoclObjectRelease(condResult, context->strPool);
        Focl_Object* bodyResult = Focl_parseBlock(context, &bodyBlock);
        if (bodyResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(result, context->strPool);
            return bodyResult;
        }

        FoclObjectRelease(result, context->strPool);
        result = bodyResult;

        Focl_Object* updateResult = Focl_parseBlock(context, &updateBlock);
        if (updateResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(result, context->strPool);
            return updateResult;
        }
        FoclObjectRelease(updateResult, context->strPool);
    }

    return result;
}
Focl_Object* buildIn_typename(Focl_Context* context, Focl_Vector* objVec)
{
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 1)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView objName = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* obj = FindObjectInContext(context, &objName);
    if (obj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    switch (obj->type)
    {
        case FOCL_OBJ_TYPE_INT:
            printf("Integer\n");
            break;
        case FOCL_OBJ_TYPE_FLOAT:
            printf("Float\n");
            break;
        case FOCL_OBJ_TYPE_BOOL:
            printf("Boolean\n");
            break;
        case FOCL_OBJ_TYPE_VOID:
            printf("Void\n");
            break;
        case FOCL_OBJ_TYPE_STR:
            printf("String\n");
            break;
        case FOCL_OBJ_TYPE_ERROR:
            printf("Error\n:");
            break;
        case FOCL_OBJ_TYPE_BLOCK:
            printf("Block\n");
            break;
        case FOCL_OBJ_TYPE_BYTECODE:
            printf("Focl ByteCode\n");
            break;
        case FOCL_OBJ_TYPE_COMPLEX:
            printf("Complex\n");
            break;
    }
    return FoclObjectVoid(context->strPool);
}
Focl_Object* buildIn_typeid(Focl_Context* context, Focl_Vector* objVec)
{
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 1)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView objName = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* obj = FindObjectInContext(context, &objName);
    if (obj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    Focl_Object* idObj = createFoclObject(FOCL_OBJ_TYPE_INT);
    idObj->as.i = obj->type;
    return idObj;
}
#define REGISTER_CMD(ctx, cmdName, func) \
    do \
    { \
        Focl_String* _name = FoclStringPoolAlloc((ctx)->strPool); \
        FoclStrAssign(_name, (cmdName)); \
        Focl_Command* _cmd = createFoclCommand((func)); \
        FoclHashTableInsert((ctx)->globalEnv->cmdTable, _name, _cmd, StrKeyCompare, NULL); \
    }while (0)

void registerBuiltinCommands(Focl_Context* context)
{
    REGISTER_CMD(context, "puts", buildIn_puts);
    REGISTER_CMD(context, "gets", buildIn_gets);
    REGISTER_CMD(context, "set", buildIn_set);
    REGISTER_CMD(context, "unset", buildIn_unset);
    REGISTER_CMD(context, "incr", buildIn_incr);
    REGISTER_CMD(context, "if", buildIn_if);
    REGISTER_CMD(context, "while", buildIn_while);
    REGISTER_CMD(context, "for", buildIn_for);
    REGISTER_CMD(context, "typename", buildIn_typename);
    REGISTER_CMD(context, "typeid", buildIn_typeid);
}

/* BUILTIN COMMAND */

char* Focl_getline(FILE* fp, size_t* len)
{
    size_t capacity = 128;
    size_t length = 0;
    char* buf = malloc(capacity);
    int c;
    while ((c = fgetc(fp)) != EOF && c != '\n')
    {
        if (c == '\r')
        {
            continue;
        }
        buf[length++] = (char)c;
        if (length + 1 >= capacity)
        {
            capacity *= 2;
            char* newbuf = realloc(buf, capacity);
            if (newbuf == NULL)
            {
                free(buf);
                return NULL;
            }
            buf = newbuf;
        }
    }
    if (length == 0 && c == EOF)
    {
        free(buf);
        if (len) *len = 0;
        return NULL;
    }
    buf[length] = '\0';
    if (len) *len = length;
    return buf;
}
int focl_countBraceDepth(const char* str)
{
    int depth = 0;
    bool inString = false;

    for (const char* p = str; *p; p++)
    {
        if (*p == '\\' && inString)
        {
            p++;
            if (*p == '\0')
            {
                break;
            }
            continue;
        }
        if (*p == '"')
        {
            inString = !inString;
            continue;
        }
        if (inString)
        {
            continue;
        }

        if (*p == '[' || *p == '{')
        {
            depth++;
        }
        else if (*p == ']' || *p == '}')
        {
            if (depth > 0) depth--;
        }
    }
    return depth;
}
void Focl_REPL(Focl_Context* ctx)
{
    Focl_String buffer;
    initFoclString(&buffer, 64);
    int depth = 0;
    printf("Focl REPL\n");
    printf("Type 'exit' to quit.\n\n");
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
        char* input = Focl_getline(stdin, &lineLen);
        if (input == NULL)
        {
            printf("\n");
            break;
        }
        if (lineLen == 0 && depth == 0)
        {
            free(input);
            continue;
        }
        if (depth == 0 && strcmp(input, "exit") == 0)
        {
            free(input);
            printf("Goodbye!\n");
            break;
        }
        if (buffer.length > 0)
        {
            FoclStrAppend(&buffer, "\n");
        }
        FoclStrAppend(&buffer, input);
        depth += focl_countBraceDepth(input);
        free(input);

        if (depth > 0)
        {
            continue;
        }
        Focl_Object* result = Focl_parseLine(ctx, &buffer);

        if (result->type == FOCL_OBJ_TYPE_ERROR)
        {
            printf("Error: %s\n", FoclStrCStr(result->as.data));
        }
        else if (result->type != FOCL_OBJ_TYPE_VOID)
        {
            FoclObjectPrint(result);
            printf("\n");
        }

        FoclObjectRelease(result, ctx->strPool);
        FoclStrClear(&buffer);
        depth = 0;
    }

    freeFoclStringData(&buffer);
}
void Focl_ExecFile(Focl_Context* ctx, const char* filename)
{
    FILE* fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Error: Cannot open file '%s'\n", filename);
        return;
    }
    Focl_String buffer;
    initFoclString(&buffer, 64);
    int depth = 0;
    size_t lineLen;
    char* input;
    while ((input = Focl_getline(fp, &lineLen)) != NULL)
    {
        if (buffer.length > 0)
        {
            FoclStrAppend(&buffer, "\n");
        }
        FoclStrAppend(&buffer, input);
        depth += focl_countBraceDepth(input);
        free(input);
        if (depth > 0)
        {
            continue;
        }
        Focl_Object* result = Focl_parseLine(ctx, &buffer);
        if (result->type == FOCL_OBJ_TYPE_ERROR)
        {
            printf("Error: %s\n", FoclStrCStr(result->as.data));
            FoclObjectRelease(result, ctx->strPool);
            break;
        }
        FoclObjectRelease(result, ctx->strPool);
        FoclStrClear(&buffer);
        depth = 0;
    }

    if (depth > 0)
    {
        Focl_Object* result = Focl_parseLine(ctx, &buffer);
        if (result->type == FOCL_OBJ_TYPE_ERROR)
        {
            printf("Error: %s\n", FoclStrCStr(result->as.data));
        }
        FoclObjectRelease(result, ctx->strPool);
    }
    freeFoclStringData(&buffer);
    fclose(fp);
}
int main(int argc, char* argv[])
{
    Focl_Context* ctx = createFoclContext();
    registerBuiltinCommands(ctx);
    if (argc > 1)
    {
        Focl_ExecFile(ctx, argv[1]);
    }
    else
    {
        Focl_REPL(ctx);
    }
    freeFoclContext(ctx);
    return 0;
}