#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "focl_dev.h"

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
    Focl_Obj_Int Focl_StrToInt(const char* str)
    {
        return strtoll(str, NULL, 0);
    }
    Focl_Obj_Float Focl_StrToFloat(const char* str)
    {
        return strtod(str, NULL);
    }
#elif SIZE_MAX == 0xFFFFFFFF
    Focl_Obj_Int Focl_StrToInt(const char* str)
    {
        return strtol(str, NULL, 0);
    }
    Focl_Obj_Float Focl_StrToFloat(const char* str)
    {
        return strtod(str, NULL);
    }
#else
    #error "Unsupported word length platform. Though I want to see this program run in every platform. But now it couldn't run yours. Sorry. :("
#endif

void FoclObjWithNoStringPoolFree(Focl_Object* obj, Focl_ObjWithNoStrPool* objPool);
void FoclStringObjPoolFree(Focl_Object* obj, Focl_StrObjPool* objPool);
void FoclCmpdObjPoolFree(Focl_Object* obj, Focl_CmpdObjPool* cmpdObjPool);

void FoclStrExpansion(Focl_Context* context, Focl_String* dst, const Focl_StringView* src);

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
char* Focl_strdup(const char* src)
{
    size_t len = strlen(src);
    char* s = malloc(len + 1);
    memcpy(s, src, len);
    s[len] = '\0';
    return s;
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
void restoreFoclStringViewFromTempString(Focl_StringView* strView, char saved) /* The tmpStr should be on stack! */
{
    strView->strPtr[strView->len] = saved;
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
char* FoclStrCStr(const Focl_String* str)
{
    return (char*)str->data;
}
void FoclStrAssignStr(Focl_String* dst, const Focl_String* src)
{
    FoclStrAssign(dst, FoclStrCStr(src));
}
void FoclStrReserve(Focl_String* str, size_t newSize_)
{
    if (str->capacity >= newSize_)
    {
        return;
    }
    str->data = (char*)realloc(str->data, sizeof(char) * newSize_);
    str->capacity = newSize_;
}
void FoclStrReserveWithoutCheck(Focl_String* str, size_t newSize_)
{
    str->data = (char*)realloc(str->data, sizeof(char) * newSize_);
    str->capacity = newSize_;
}
void FoclStrAssignView(Focl_String* dst, const Focl_StringView* view)
{
    if (view->len + 1 > dst->capacity)
    {
        FoclStrReserve(dst, view->len + 1);
    }
    memcpy(dst->data, view->strPtr, sizeof(char) * view->len);
    dst->data[view->len] = '\0';
    dst->length = view->len;
}
void FoclStrDoubleReserve(Focl_String* str)
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
void FoclStrAppendStr(Focl_String* dst, const Focl_String* src)
{
    FoclStrAppend(dst, FoclStrCStr(src));
}
void FoclStrAppendView(Focl_String* dst, const Focl_StringView* view)
{
    if (view->len == 0)
    {
        return;
    }
    size_t newLen = dst->length + view->len;
    if (newLen >= dst->capacity)
    {
        size_t newCap = dst->capacity * 2;
        if (newCap < newLen + 1)
        {
            newCap = newLen + 1;
        }
        FoclStrReserve(dst, newCap);
    }
    memcpy(dst->data + dst->length, view->strPtr, view->len);
    dst->length = newLen;
    dst->data[newLen] = '\0';
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
int FoclStrComp(const Focl_String* str, const char* cStr)
{
    return strcmp(str->data, cStr);
}
int FoclStrCompStr(const Focl_String* str1, const Focl_String* str2)
{
    return strcmp(str1->data, str2->data);
}
int FoclStrCompStrView(const Focl_String* str, const Focl_StringView* strView)
{
    if (str->length != strView->len)
    {
        return (str->length < strView->len) ? -1 : 1;
    }
    return memcmp(str->data, strView->strPtr, strView->len);
}
size_t FoclStrCharCount(const Focl_String* str)
{   
    size_t count = 0;
    const char* ptr = str->data;
    while (*ptr)
    {
        int32_t cpLen = getUtf8CodePointLength((uint8_t)*ptr);
        ptr += cpLen;
        count++;
    }
    return count;
}
void FoclStrClear(Focl_String* str)
{
    str->length = 0;
    str->data[0] = '\0';
}
void freeFoclString(Focl_String* str)
{
    free(str->data);
    free(str);
}

void FoclStringOpCt(Focl_String* str, size_t iCapacity)
{
    str->data = malloc(sizeof(char) * iCapacity);
    str->capacity = iCapacity;
    str->length = 0;
    str->data[iCapacity - 1] = '\0';
}
void FoclStringOpCtVoid(void* str, void* ctx)
{
    /* The second parameter is useless currently. the function will keep use FOCL_STRING_INIT_CAPACITY */
    (void)ctx;
    FoclStringOpCt((Focl_String*)str, FOCL_STRING_INIT_CAPACITY);
}
void FoclStringOpDt(Focl_String* str)
{
    free(str->data);
}
void FoclStringOpDtVoid(void* str, void* ctx)
{
    (void)ctx;
    FoclStringOpDt((Focl_String*)str);
}
void FoclStringOpClVoid(void* str, void* ctx)
{
    (void)ctx;
    FoclStrClear((Focl_String*)str);
}

Focl_StringView FoclStringViewPeelFront(Focl_StringView* strView)
{
    Focl_StringView peelView = {strView->len - 1, strView->strPtr + 1};
    return peelView;
}
Focl_StringView FoclStringViewPeelBoth(Focl_StringView* strView)
{
    if (strView->len < 2)
    {
        return (Focl_StringView){.len = 0, .strPtr = NULL};
    }
    Focl_StringView peelView = {strView->len - 2, strView->strPtr + 1};
    return peelView;
}
size_t FoclStringViewCharCount(const Focl_StringView* strView)
{   
    size_t count = 0;
    const char* ptr = strView->strPtr;
    const char* end = ptr + strView->len;
    while (ptr < end)
    {
        int32_t cpLen = getUtf8CodePointLength((uint8_t)*ptr);
        ptr += cpLen;
        count++;
    }
    return count;
}

int FoclStringViewComp(Focl_StringView* strView, const char* Cstr)
{
    size_t cStrLen = strlen(Cstr);
    if (cStrLen != strView->len)
    {
        return (strView->len < cStrLen) ? -1 : 1;
    }
    return memcmp(strView->strPtr, Cstr, cStrLen);
}

bool isStringViewEnd(const Focl_StringView* strView, const char* pos)
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
bool Focl_isVoid(const char* str) /* pure "" */
{
    return *str == '\0';
}
bool Focl_isVoidView(const Focl_StringView* strView) /* pure "" */
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

    return depth == 0 && isStringViewEnd(strView, str);
}
bool Focl_isCmdSubstition(const char* str)
{
    if (*str != '[')
    {
        return false;
    }

    const char* start = str;
    const char* pace = start + 1;
    
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
    if (*str == '$' && *(str + 1) != '\0')
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
size_t hashDjb2AsArg(void* str)
{
    return hashDjb2((Focl_String*)str);
}

bool StrKeyCompare(void* a, void* b)
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
void FoclVectorReserve(Focl_Vector* vec, size_t newCapacity_)
{
    vec->data = realloc(vec->data, vec->itemSize * newCapacity_);
    vec->capacity = newCapacity_;
}
void FoclVectorDoubleReserve(Focl_Vector* vec)
{
    FoclVectorReserve(vec, vec->capacity * 2);
}
void* FoclVectorAt(Focl_Vector* vec, size_t idx) /* NULL-able */
{
    if (idx >= vec->size)
    {
        return NULL;
    }
    return (void*)((uint8_t*)(vec->data) + vec->itemSize * idx);
}
void* FoclVectorAtNoCheck(Focl_Vector* vec, size_t idx) /* Warning: it will not check whether index out of bounds. */
{
    return (void*)((uint8_t*)(vec->data) + vec->itemSize * idx);
}
void FoclVectorPushBack(Focl_Vector* vec, void* data)
{
    if (vec->size >= vec->capacity)
    {
        FoclVectorDoubleReserve(vec);
    }
    memcpy((uint8_t*)vec->data + (vec->size++ * vec->itemSize), data, vec->itemSize);
}
void FoclVectorPopBack(Focl_Vector* vec)
{
    if (vec->size != 0)
    {
        vec->size--;
    }
}
void FoclVectorClear(Focl_Vector* vec)
{
    vec->size = 0;
}
size_t FoclVectorGetSize(Focl_Vector* vec)
{
    return (vec->size);
}
void FoclVectorAssignVector(Focl_Vector* dst, Focl_Vector* src)
{
    if (src->size >= dst->capacity)
    {
        FoclVectorReserve(dst, src->size);
    }
    memcpy(dst->data, src->data, src->itemSize * src->size);
    dst->size = src->size;
}
void FoclObjVectorDeepCopy(Focl_Vector* dst, Focl_Vector* src)
{
    FoclVectorClear(dst);
    size_t srcSize = FoclVectorGetSize(src);
    for (size_t i = 0; i < srcSize; i++)
    {
        Focl_Object* srcObj = FoclObjVecAt(src, i);
        FoclObjectRetain(srcObj);
        FoclVectorPushBack(dst, &srcObj);
    }
}
void FoclVectorAppendVector(Focl_Vector* dst, Focl_Vector* src)
{
    if (src->size == 0)
    {
        return;
    }
    
    size_t totalSize = dst->size + src->size;
    
    while (totalSize >= dst->capacity)
    {
        FoclVectorDoubleReserve(dst);
    }
    
    memcpy((uint8_t*)dst->data + (dst->size * dst->itemSize), src->data, src->size * src->itemSize);
    
    dst->size = totalSize;
}
void freeFoclVector(Focl_Vector* vec, Focl_TypeOpDt* opDt) /* If NULL, won't destruct item. */
{
    if (opDt != NULL)
    {
        size_t itemCount = vec->size;
        size_t itemSize_ = vec->itemSize;
        for (size_t i = 0; i < itemCount; i++)
        {
            opDt->func((uint8_t*)vec->data + i * itemSize_, opDt->ctx);
        }
    }
    free(vec->data);
    free(vec);
}

typedef struct FoclVectorOpCtCtx
{
    size_t itemSize;
    size_t iCapacity;
}FoclVectorOpCtCtx;
void FoclVectorOpCt(Focl_Vector* vec, size_t itemSize_, size_t iCapacity)
{
    vec->data = malloc(sizeof(char) * itemSize_ * iCapacity);
    vec->capacity = iCapacity;
    vec->size = 0;
    vec->itemSize = itemSize_;
}
void FoclVectorOpCtVoid(void* vec, void* ctx)
{
    FoclVectorOpCtCtx* ctCtx = ctx;
    FoclVectorOpCt(vec, ctCtx->itemSize, ctCtx->iCapacity);
}
void FoclVectorOpDt(Focl_Vector* vec, Focl_TypeOpDt* opDt) /* If NULL, won't destruct item. */
{
    if (opDt != NULL)
    {
        size_t itemCount = vec->size;
        size_t itemSize_ = vec->itemSize;
        for (size_t i = 0; i < itemCount; i++)
        {
            opDt->func((uint8_t*)vec->data + i * itemSize_, opDt->ctx);
        }
    }
    free(vec->data);
}
void FoclVectorOpDtVoid(void* vec, void* ctx)
{
    FoclVectorOpDt((Focl_Vector*)vec, (Focl_TypeOpDt*)ctx);
}
void FoclVectorOpClVoid(void* vec, void* ctx)
{
    (void)ctx;
    FoclVectorClear(vec);
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
void freeFoclHashTableUnit(Focl_HashTableUnit* unit, Focl_KeyOpDt* keyOpDt, Focl_ValueOpDt* valueOpDt)
{
    if (keyOpDt != NULL)
    {
        keyOpDt->func(unit->key, keyOpDt->ctx);
    }
    if (valueOpDt != NULL)
    {
        valueOpDt->func(unit->value, valueOpDt->ctx);
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
void FoclHashTableInsert(Focl_HashTable* table, void* key, void* value, KeyCompareFunc keyCompareFunc, Focl_KeyOpDt* keyOpDt, Focl_ValueOpDt* valueOpDt)
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
            if (valueOpDt != NULL && current->value != value)
            {
                valueOpDt->func(current->value, valueOpDt->ctx);
            }
            if (keyOpDt != NULL)
            {
                keyOpDt->func(key, keyOpDt->ctx);
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
void FoclHashTableDelete(Focl_HashTable* table, void* key, KeyCompareFunc keyCompareFunc, Focl_KeyOpDt* keyOpDt, Focl_ValueOpDt* valueOpDt)
{
    size_t idx = (table->hashFunc(key)) % table->capacity;
    Focl_HashTableUnit* current = table->buckets[idx];
    Focl_HashTableUnit* prev = NULL;
    if (keyOpDt != NULL || valueOpDt != NULL)
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
                freeFoclHashTableUnit(current, keyOpDt, valueOpDt);
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
void freeFoclHashTable(Focl_HashTable* table, Focl_KeyOpDt* keyOpDt, Focl_ValueOpDt* valueOpDt)
{
    Focl_HashTableUnit* unit;
    Focl_HashTableUnit* next;
    if (keyOpDt != NULL || valueOpDt != NULL)
    {
        for (size_t i = 0; i < table->capacity; i++)
        {
            unit = table->buckets[i];
            while (unit != NULL)
            {
                next = unit->next;
                freeFoclHashTableUnit(unit, keyOpDt, valueOpDt);
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

Focl_PoolBlock* createFoclPoolBlock(size_t itemCount, size_t itemSize, Focl_TypeOpCt* opCt)
{
    Focl_PoolBlock* block = (Focl_PoolBlock*)malloc(sizeof(Focl_PoolBlock));
    block->itemCount = itemCount;
    block->itemSize = itemSize;
    block->freeTop = itemCount;
    block->freeStack = (size_t*)malloc(itemCount * sizeof(size_t));
    block->data = malloc(itemCount * itemSize);
    if (opCt != NULL)
    {
        for (size_t i = 0; i < itemCount; i++)
        {
            block->freeStack[i] = i;
            opCt->func((uint8_t*)block->data + i * itemSize, opCt->ctx);
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
void freeFoclPoolBlock(Focl_PoolBlock* block, Focl_TypeOpDt* opDt)
{
    if (opDt != NULL)
    {
        for (size_t i = 0; i < block->itemCount; i++)
        {
            opDt->func((uint8_t*)block->data + i * block->itemSize, opDt->ctx);
        }
    }
    free(block->freeStack);
    free(block->data);
    free(block);
}
Focl_Pool* createFoclPool(size_t itemSize_, size_t itemPerBlock_, size_t iBlockCount, Focl_TypeOpCt* opCt)
{
    Focl_Pool* pool = (Focl_Pool*)malloc(sizeof(Focl_Pool));
    pool->itemSize = itemSize_;
    pool->itemPerBlock = itemPerBlock_;
    pool->blocks = (Focl_PoolBlock**)malloc(sizeof(Focl_PoolBlock*) * iBlockCount);
    pool->blockCount = iBlockCount;
    for (size_t i = 0; i < iBlockCount; i++)
    {
        pool->blocks[i] = createFoclPoolBlock(itemPerBlock_, itemSize_, opCt);
    }
    return pool;
}
void freeFoclPool(Focl_Pool* pool, Focl_TypeOpDt* opDt) /* Distinguish it from FoclPoolFree() */
{
    size_t count = pool->blockCount;
    for (size_t i = 0; i < count; i++)
    {
        freeFoclPoolBlock(pool->blocks[i], opDt);
    }
    free(pool->blocks);
    free(pool);
}
void FoclPoolBlockExpand(Focl_Pool* pool, size_t newBlockCount, Focl_TypeOpCt* opCt)
{
    pool->blocks = (Focl_PoolBlock**)realloc(pool->blocks, sizeof(Focl_PoolBlock*) * newBlockCount);
    size_t iS = pool->itemSize;
    size_t iPB = pool->itemPerBlock;
    for (size_t i = pool->blockCount; i < newBlockCount; i++)
    {
        pool->blocks[i] = createFoclPoolBlock(iPB, iS, opCt);
    }
    pool->blockCount = newBlockCount;
}
void FoclPoolBlockDouble(Focl_Pool* pool, Focl_TypeOpCt* opCt)
{
    FoclPoolBlockExpand(pool, pool->blockCount * 2, opCt);
}
void* FoclPoolAlloc(Focl_Pool* pool, Focl_TypeOpCl* opCl)
{
    for (size_t i = 0; i < pool->blockCount; i++)
    {
        Focl_PoolBlock* block = pool->blocks[i];
        if (block->freeTop > 0)
        {
            block->freeTop--;
            size_t idx = block->freeStack[block->freeTop];
            void* obj = (uint8_t*)block->data + idx * pool->itemSize;
            if (opCl != NULL)
            {
                opCl->func(obj, opCl->ctx);
            }
            return obj;
        }
    }
    return NULL;
}

void* FoclPoolAllocEx(Focl_Pool* pool, Focl_TypeOpCt* opCt, Focl_TypeOpCl* opCl)
{
    for (size_t i = 0; i < pool->blockCount; i++)
    {
        Focl_PoolBlock* block = pool->blocks[i];
        if (block->freeTop > 0)
        {
            block->freeTop--;
            size_t idx = block->freeStack[block->freeTop];
            void* obj = (uint8_t*)block->data + idx * pool->itemSize;
            if (opCl != NULL)
            {
                opCl->func(obj, opCl->ctx);
            }
            return obj;
        }
    }

    size_t oldCount = pool->blockCount;
    FoclPoolBlockDouble(pool, opCt);

    Focl_PoolBlock* newBlock = pool->blocks[oldCount];
    newBlock->freeTop--;
    size_t idx = newBlock->freeStack[newBlock->freeTop];
    void* obj = (uint8_t*)newBlock->data + idx * pool->itemSize;
    if (opCl != NULL)
    {
        opCl->func(obj, opCl->ctx);
    }
    return obj;
}

void FoclPoolFree(void* obj, Focl_Pool* pool) /* Distinguish it from freeFoclPool() */
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

Focl_StringPool* createFoclStringPool()
{
    Focl_TypeOpCt opCt = {.func = FoclStringOpCtVoid, .ctx = NULL};
    return createFoclPool(sizeof(Focl_String), FOCL_STRING_POOL_ITEM_PER_BLOCK, FOCL_STRING_POOL_BLOCK_COUNT_INIT, &opCt);
}
void freeFoclStringPool(Focl_StringPool* strPool)
{
    Focl_TypeOpDt opDt = {.func = FoclStringOpDtVoid, .ctx = NULL};
    freeFoclPool(strPool, &opDt);
}
Focl_String* FoclStringPoolAlloc(Focl_StringPool* strPool) /* It will alloc string and clear it. */
{
    Focl_TypeOpCt opCt = {.func = FoclStringOpCtVoid, .ctx = NULL};
    Focl_TypeOpCl opCl = {.func = FoclStringOpClVoid, .ctx = NULL};
    Focl_String* str = (Focl_String*)FoclPoolAllocEx(strPool, &opCt, &opCl);
    return str;
}
void FoclStringPoolFree(Focl_String* str, Focl_StringPool* strPool)
{
    FoclPoolFree((void*)str, strPool);
}
void FoclStringPoolFreeOpDtVoid(void* str, void* strPool)
{
    FoclStringPoolFree(str, strPool);
}

/* STRING POOL */

/* VECTOR POOL */

Focl_VectorPool* createFoclVectorPool(size_t lenOfElement)
{
    Focl_VectorPool* vecPool = malloc(sizeof(Focl_VectorPool));
    vecPool->elementLen = lenOfElement;
    FoclVectorOpCtCtx ctCtx = {.itemSize = lenOfElement, .iCapacity = FOCL_VECTOR_INIT_CAPACITY};
    Focl_TypeOpCt opCt = {.func = FoclVectorOpCtVoid, .ctx = &ctCtx};
    vecPool->pool = createFoclPool(sizeof(Focl_Vector), FOCL_VECTOR_POOL_ITEM_PER_BLOCK, FOCL_VECTOR_POOL_BLOCK_COUNT_INIT, &opCt);
    return vecPool;
}
void freeFoclVectorPool(Focl_VectorPool* vecPool)
{
    Focl_TypeOpDt opDt = {.func = FoclVectorOpDtVoid, .ctx = NULL};
    freeFoclPool(vecPool->pool, &opDt);
    free(vecPool);
}
Focl_Vector* FoclVectorPoolAlloc(Focl_VectorPool* vecPool)
{
    FoclVectorOpCtCtx ctCtx = {.itemSize = vecPool->elementLen, .iCapacity = FOCL_VECTOR_INIT_CAPACITY};
    Focl_TypeOpCt opCt = {.func = FoclVectorOpCtVoid, .ctx = &ctCtx};
    Focl_TypeOpCl opCl = {.func = FoclVectorOpClVoid, .ctx = NULL};
    Focl_Vector* vec = (Focl_Vector*)FoclPoolAllocEx(vecPool->pool, &opCt, &opCl);
    FoclVectorClear(vec);
    return vec;
}
void FoclVectorPoolFree(Focl_Vector* vec, Focl_VectorPool* vecPool)
{
    FoclPoolFree((void*)vec, vecPool->pool);
} 

/* VECTOR POOL */

/* VAR */

Focl_Obj_Int FoclObjectUnboxInt(Focl_Object* obj)
{
    return (obj->as.i);
}
Focl_Obj_Float FoclObjectUnboxFloat(Focl_Object* obj)
{
    return (obj->as.f);
}
Focl_String* FoclObjectGetString(Focl_Object* obj)
{
    return (obj->as.data);
}
Focl_Vector* FoclObjectGetVector(Focl_Object* obj)
{
    return (obj->as.vec);
}
void FoclObjectBoxInt(Focl_Object* obj, Focl_Obj_Int i_)
{
    obj->as.i = i_;
}
void FoclObjectBoxFloat(Focl_Object* obj, Focl_Obj_Float f_)
{
    obj->as.f = f_;
}
bool isFoclObjectUseString(Focl_Object* obj)
{
    return (obj->type >= FOCL_OBJ_TYPE_VOID && obj->type <= FOCL_OBJ_TYPE_STR);
}
bool isFoclObjectCompound(Focl_Object* obj)
{
    return (obj->type == FOCL_OBJ_TYPE_COMPOUND);
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

Focl_Object* createFoclObject(Focl_Obj_Type type_)
{
    Focl_Object* obj = (Focl_Object*)malloc(sizeof(Focl_Object));
    obj->refCount = 1;
    obj->type = type_;
    return obj;
}
Focl_Object* createStringFoclObject(Focl_Obj_Type type_, Focl_StringPool* strPool)
{
    Focl_Object* obj = (Focl_Object*)malloc(sizeof(Focl_Object));
    obj->refCount = 1;
    obj->type = type_;
    obj->as.data = FoclStringPoolAlloc(strPool);
    return obj;
}
void FoclObjectAssign(Focl_Object* dst, Focl_Object* src, Focl_StringPool* strPool, Focl_VectorPool* vecPool)
{
    /* This is a strong type language! You should do a type check before every assign! */

    if (isFoclObjectUseString(src))
    {
        FoclStringPoolFree(dst->as.data, strPool);
        dst->as.data = FoclStringPoolAlloc(strPool);
        FoclStrAssignStr(dst->as.data, src->as.data);
    }
    else if (isFoclObjectCompound(src))
    {
        FoclVectorPoolFree(dst->as.vec, vecPool);
        dst->as.vec = FoclVectorPoolAlloc(vecPool);
        FoclObjVectorDeepCopy(dst->as.vec, src->as.vec);
    }
    else
    {
        dst->as = src->as;
    }
}
Focl_Object* FoclObjectError(Focl_StrObjPool* strObjPool, Focl_StringPool* strPool, const char* errmsg)
{
    Focl_Object* obj = FoclStringObjPoolAlloc(strObjPool, strPool, FOCL_OBJ_TYPE_ERROR);
    FoclStrAssign(FoclObjectGetString(obj), errmsg);
    return obj;
}
Focl_Object* getFoclObjectWithStringView(Focl_Context* context, const Focl_StringView* strView)
{
    Focl_Object* obj;
    Focl_StringPool* strPool = context->strPool;
    Focl_ObjWithNoStrPool* objPool = context->objWithNoStrPool;
    Focl_StrObjPool* strObjPool = context->strObjPool;
    if (Focl_isInteger_View(strView))
    {
        obj = FoclObjWithNoStringPoolAlloc(objPool, FOCL_OBJ_TYPE_INT);
        FoclObjectBoxInt(obj, Focl_StrToInt_View(strView));
    }
    else if (Focl_isFloat_View(strView))
    {
        obj = FoclObjWithNoStringPoolAlloc(objPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(obj, Focl_StrToFloat_View(strView));
    }
    else if (Focl_isString_View(strView))
    {
        obj = FoclStringObjPoolAlloc(strObjPool, strPool, FOCL_OBJ_TYPE_STR);
        FoclStrExpansion(context, obj->as.data, strView);
    }
    else if (Focl_isVarSubstition_View(strView))
    {
        Focl_StringView varStrView = {strView->len - 1, strView->strPtr + 1};
        Focl_String tmpStr;
        char saved = initTempFoclStringWithView(&tmpStr, &varStrView);
        obj = Focl_FindObject(context->curEnv, context->strPool, &tmpStr);
        restoreFoclStringViewFromTempString(&varStrView, saved);
        if (obj == FOCL_OBJECT_ERROR)
        {
            return FoclObjectError(strObjPool, strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
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
        obj = FoclStringObjPoolAlloc(strObjPool, strPool, FOCL_OBJ_TYPE_STR);
        FoclStrAssignView(obj->as.data, strView);
    }
    return obj;
}
Focl_Object* FoclObjectVoid(Focl_StrObjPool* strObjPool, Focl_StringPool* strPool)
{
    return FoclStringObjPoolAlloc(strObjPool, strPool, FOCL_OBJ_TYPE_VOID);
}
Focl_Object* FoclObjectBool(Focl_ObjWithNoStrPool* objPool, Focl_Obj_Bool booleanValue)
{
    Focl_Object* obj = FoclObjWithNoStringPoolAlloc(objPool, FOCL_OBJ_TYPE_BOOL);
    obj->as.i = booleanValue;
    return obj;
}
void freeFoclObject(Focl_Object* obj, Focl_StringPool* strPool, Focl_VectorPool* vecPool)
{
    if (isFoclObjectUseString(obj))
    {
        FoclStringPoolFree(obj->as.data, strPool);
    }
    else if (isFoclObjectCompound(obj))
    {
        FoclVectorPoolFree(obj->as.vec, vecPool);
    }
    free(obj);
}
void FoclObjectRetain(Focl_Object* obj)
{
    if (obj != NULL)
    {
        obj->refCount++;
    }
}
void FoclObjectRelease(Focl_Object* obj, Focl_Context* context)
{
    obj->refCount--;
    if (obj->refCount == 0)
    {
        if (isFoclObjectUseString(obj))
        {
            FoclStringObjPoolFree(obj, context->strObjPool);
        }
        else if (isFoclObjectCompound(obj))
        {
            FoclCmpdObjPoolFree(obj, context->cmpdObjPool);
        }
        else
        {
            FoclObjWithNoStringPoolFree(obj, context->objWithNoStrPool);
        }
    }
}
Focl_String* FoclCmpdObjStringize(Focl_Object* cmpdObj, Focl_StringPool* strPool)
{
    size_t vecSize = FoclVectorGetSize(cmpdObj->as.vec);
    size_t curlength;
    Focl_String* dstStr = FoclStringPoolAlloc(strPool);
    Focl_String* tempStr;
    Focl_Object* obj;
    for (size_t i = 0; i < vecSize; i++)
    {
        obj = FoclObjVecAt(FoclObjectGetVector(cmpdObj), i);
        switch (obj->type)
        {
            case FOCL_OBJ_TYPE_INT:
                tempStr = FoclStringPoolAlloc(strPool);
                FoclStrReserve(tempStr, FOCL_INT_TO_STR_TMP_BUFFER_SIZE);
                curlength = sprintf(tempStr->data, "%" FOCL_FORMAT_INT, obj->as.i);
                tempStr->length = curlength;
                FoclStrAppendStr(dstStr, tempStr);
                FoclStringPoolFree(tempStr, strPool);
                break;
            case FOCL_OBJ_TYPE_FLOAT:
                tempStr = FoclStringPoolAlloc(strPool);
                FoclStrReserve(tempStr, FOCL_FLOAT_TO_STR_TMP_BUFFER_SIZE);
                curlength = sprintf(tempStr->data, "%" FOCL_FORMAT_FLOAT, obj->as.f);
                tempStr->length = curlength;
                FoclStrAppendStr(dstStr, tempStr);
                FoclStringPoolFree(tempStr, strPool);
                break;
            case FOCL_OBJ_TYPE_BOOL:
                tempStr = FoclStringPoolAlloc(strPool);
                FoclStrReserve(tempStr, sizeof("false"));
                curlength = sprintf(tempStr->data, "%s", (obj->as.i == FOCL_OBJ_TRUE) ? "true" : "false");
                tempStr->length = curlength;
                FoclStrAppendStr(dstStr, tempStr);
                FoclStringPoolFree(tempStr, strPool);
                break;
            case FOCL_OBJ_TYPE_VOID:
                /* Do nothing */
                break;
            case FOCL_OBJ_TYPE_STR:
                FoclStrAppendStr(dstStr, FoclObjectGetString(obj));
                break;
            case FOCL_OBJ_TYPE_COMPOUND:
                if (obj != cmpdObj)
                {
                    tempStr = FoclCmpdObjStringize(obj, strPool);
                    FoclStrAppendStr(dstStr, tempStr);
                    FoclStringPoolFree(tempStr, strPool);
                }
                else
                {
                    FoclStrAppend(dstStr, "*self*");
                }
                break;
            default:
                printf(FOCL_ERR_YSNBH);
                break;
        }
        if (i < vecSize - 1)
        {
            FoclStrAppend(dstStr, " ");
        }
    }
    return dstStr;
}

void FoclObjectReleaseOpDtVoid(void* obj, void* ctx)
{
    FoclObjectRelease(obj, ctx);
}

void FoclObjectWithNoStringOpClVoid(void* obj_, void* ctx)
{
    (void)ctx;
    Focl_Object* obj = obj_;
    obj->refCount = 1;
}

void FoclStringObjectOpCt(Focl_Object* obj, Focl_StringPool* strPool)
{
    obj->as.data = FoclStringPoolAlloc(strPool);
}
void FoclStringObjectOpCtVoid(void* obj, void* strPool)
{
    FoclStringObjectOpCt(obj, strPool);
}
void FoclStringObjectOpDt(Focl_Object* obj, Focl_StringPool* strPool)
{
    FoclStringPoolFree(FoclObjectGetString(obj), strPool);
}
void FoclStringObjectOpDtVoid(void* obj, void* strPool)
{
    FoclStringObjectOpDt(obj, strPool);
}
void FoclStringObjectOpCl(Focl_Object* obj)
{
    obj->refCount = 1;
    FoclStrClear(FoclObjectGetString(obj));
}
void FoclStringObjectOpClVoid(void* obj, void* ctx)
{
    (void)ctx;
    FoclStringObjectOpCl(obj);
}

void FoclCmpdObjectOpCt(Focl_Object* obj, Focl_VectorPool* objVecPool)
{
    obj->as.vec = FoclVectorPoolAlloc(objVecPool);
}
void FoclCmpdObjectOpCtVoid(void* obj, void* objVecPool)
{
    FoclCmpdObjectOpCt(obj, objVecPool);
}
void FoclCmpdObjectOpDt(Focl_Object* obj, Focl_Context* context)
{
    size_t vecSize = FoclVectorGetSize(obj->as.vec);
    for (size_t i = 0; i < vecSize; i++)
    {
        Focl_Object* curObj = FoclObjVecAt(FoclObjectGetVector(obj), i);
        if (curObj != obj)
        {
            FoclObjectRelease(curObj, context);
        }
    }
    FoclVectorPoolFree(obj->as.vec, context->objVecPool);
}
void FoclCmpdObjectOpDtVoid(void* obj, void* context)
{
    FoclCmpdObjectOpDt(obj, context);
}
void FoclCmpdObjectOpCl(Focl_Object* obj)
{
    obj->refCount = 1;
    FoclVectorClear(obj->as.vec);
}
void FoclCmpdObjectOpClVoid(void* obj, void* ctx)
{
    (void)ctx;
    FoclCmpdObjectOpCl(obj);
}

/* OBJ POOL */

Focl_ObjWithNoStrPool* createFoclObjWithNoStringPool()
{
    return createFoclPool(sizeof(Focl_Object), FOCL_OBJ_POOL_ITEM_PER_BLOCK, FOCL_OBJ_POOL_BLOCK_COUNT_INIT, NULL);
}
Focl_StrObjPool* createFoclStringObjPool(Focl_StringPool* strPool)
{
    Focl_TypeOpCt opCt = {.ctx = strPool, .func = FoclStringObjectOpCtVoid};
    return createFoclPool(sizeof(Focl_Object), FOCL_OBJ_POOL_ITEM_PER_BLOCK, FOCL_OBJ_POOL_BLOCK_COUNT_INIT, &opCt);
}
Focl_CmpdObjPool* createFoclCmpdObjPool(Focl_VectorPool* objVecPool)
{
    Focl_TypeOpCt opCt = {.ctx = objVecPool, .func = FoclCmpdObjectOpCtVoid};
    return createFoclPool(sizeof(Focl_Object), FOCL_OBJ_POOL_ITEM_PER_BLOCK, FOCL_OBJ_POOL_BLOCK_COUNT_INIT, &opCt);
}

Focl_Object* FoclObjWithNoStringPoolAlloc(Focl_ObjWithNoStrPool* objPool, Focl_Obj_Type type_)
{
    Focl_TypeOpCl opCl = {.ctx = NULL, .func = FoclObjectWithNoStringOpClVoid};
    Focl_Object* obj = (Focl_Object*)FoclPoolAllocEx(objPool, NULL, &opCl);
    obj->type = type_;
    return obj;
}
Focl_Object* FoclStringObjPoolAlloc(Focl_StrObjPool* strObjPool, Focl_StringPool* strPool, Focl_Obj_Type type_)
{
    /* it won't do anything for detail string type. except clear string no matter whether it's void type. */
    Focl_TypeOpCt opCt = {.ctx = strPool, .func = FoclStringObjectOpCtVoid};
    Focl_TypeOpCl opCl = {.ctx = NULL, .func = FoclStringObjectOpClVoid};
    Focl_Object* obj = (Focl_Object*)FoclPoolAllocEx(strObjPool, &opCt, &opCl);
    obj->type = type_;
    return obj;
}
Focl_Object* FoclCmpdObjPoolAlloc(Focl_CmpdObjPool* cmpdObjPool, Focl_VectorPool* objVecPool)
{
    Focl_TypeOpCt opCt = {.ctx = objVecPool, .func = FoclCmpdObjectOpCtVoid};
    Focl_TypeOpCl opCl = {.ctx = NULL, .func = FoclCmpdObjectOpClVoid};
    Focl_Object* obj = (Focl_Object*)FoclPoolAllocEx(cmpdObjPool, &opCt, &opCl);
    obj->type = FOCL_OBJ_TYPE_COMPOUND;
    return obj;
}

Focl_Object* FoclObjPoolWithNoStringAllocAssign(Focl_StrObjPool* objPool, Focl_Object* src)
{
    Focl_Object* obj = FoclObjWithNoStringPoolAlloc(objPool, src->type);
    obj->as = src->as;
    return obj;
}
Focl_Object* FoclStringObjPoolAllocAssign(Focl_StrObjPool* objPool, Focl_StringPool* strPool, Focl_Object* src)
{
    Focl_Object* obj = FoclStringObjPoolAlloc(objPool, strPool, src->type);
    FoclStrAssignStr(FoclObjectGetString(obj), FoclObjectGetString(src));
    return obj;
}
Focl_Object* FoclCmpdObjPoolAllocAssign(Focl_CmpdObjPool* cmpdObjPool, Focl_VectorPool* objVecPool, Focl_Object* src)
{
    Focl_Object* obj = FoclCmpdObjPoolAlloc(cmpdObjPool, objVecPool);
    FoclVectorAssignVector(obj->as.vec, src->as.vec);
    return obj;
}
Focl_Object* FoclObjPoolAllocAssign(Focl_Context* context, Focl_Object* src)
{
    /* Universal pool alloc assign function */
    if (isFoclObjectUseString(src))
    {
        return FoclStringObjPoolAllocAssign(context->strObjPool, context->strPool, src);
    }
    else if (isFoclObjectCompound(src))
    {
        return FoclCmpdObjPoolAllocAssign(context->cmpdObjPool, context->objVecPool, src);
    }
    else
    {
        return FoclObjPoolWithNoStringAllocAssign(context->objWithNoStrPool, src);
    }
}
Focl_Object* FoclObjectCopy(Focl_Context* context, Focl_Object* src)
{
    /* Just a alias */
    return FoclObjPoolAllocAssign(context, src);
}
void FoclObjWithNoStringPoolFree(Focl_Object* obj, Focl_ObjWithNoStrPool* objPool)
{
    FoclPoolFree(obj, objPool);
}
void FoclStringObjPoolFree(Focl_Object* obj, Focl_StrObjPool* strObjPool)
{
    FoclPoolFree(obj, strObjPool);
}
void FoclCmpdObjPoolFree(Focl_Object* obj, Focl_CmpdObjPool* cmpdObjPool)
{
    FoclPoolFree(obj, cmpdObjPool);
}
void freeFoclObjWithNoStringPool(Focl_ObjWithNoStrPool* objPool)
{
    freeFoclPool(objPool, NULL);
}
void freeFoclStringObjPool(Focl_StrObjPool* objPool, Focl_StringPool* strPool)
{
    Focl_TypeOpDt opDt = {.ctx = strPool, .func = FoclStringObjectOpDtVoid};
    freeFoclPool(objPool, &opDt);
}
void freeFoclCmpdObjPool(Focl_Context* ctx)
{
    Focl_TypeOpDt opDt = {.ctx = ctx, .func = FoclCmpdObjectOpDtVoid};
    freeFoclPool(ctx->cmpdObjPool, &opDt);
}

/* OBJ POOL */

/* OBJECT TABLE */

Focl_ObjTable* createFoclObjTable()
{
    return createFoclHashTable(FOCL_OBJ_TABLE_INIT_CAPACITY, FOCL_OBJ_TABLE_LOAD_FACTOR, hashDjb2AsArg);
}
Focl_Object* FindObjectInTable(Focl_ObjTable* objTable, const Focl_String* strView)
{
    return (Focl_Object*)FoclHashTableFind(objTable, (void*)strView, StrKeyCompare);
}
void LinkObjectWithName(Focl_Context* context, Focl_Object* obj, const Focl_String* str)
{
    Focl_String* strName = FoclStringPoolAlloc(context->strPool);
    FoclStrAssignStr(strName, str);
    Focl_KeyOpDt keyOpDt = {.ctx = context->strPool, .func = FoclStringPoolFreeOpDtVoid};
    Focl_ValueOpDt valueOpDt = {.ctx = context, .func = FoclObjectReleaseOpDtVoid};
    FoclHashTableInsert(context->curEnv->objTable, strName, obj, StrKeyCompare, &keyOpDt, &valueOpDt);
}
void freeFoclObjTable(Focl_ObjTable* objTable, Focl_Context* context)
{
    /*
     * So you may ask, why don't we just use context as the only arg? Well, that's because
     * a "free" func is usually use itself's pointer (as "this" in cpp) as the first arg.
     * and also... one day we may also "poolize" the obj table too. Not really, I mean
     */
    Focl_KeyOpDt keyOpDt = {.ctx = context->strPool, .func = FoclStringPoolFreeOpDtVoid};
    Focl_ValueOpDt valueOpDt = {.ctx = context, .func = FoclObjectReleaseOpDtVoid};
    freeFoclHashTable(objTable, &keyOpDt, &valueOpDt);
}

/* OBJECT TABLE */

/* COMMAND TABLE */

Focl_Command* createFoclCommandBuildIn(Focl_CommandFunc cmdFunc, Focl_StringPool* strPool, const char* cmdName)
{
    Focl_Command* cmd = (Focl_Command*)malloc(sizeof(Focl_Command));
    cmd->func = cmdFunc;
    cmd->name = FoclStringPoolAlloc(strPool);
    FoclStrAssign(cmd->name, cmdName);
    cmd->proc = NULL;
    cmd->args = NULL;
    return cmd;
}
Focl_Command* createFoclCommand(Focl_StringPool* strPool, Focl_String* cmdName, Focl_StringView* argsView, Focl_StringView* procView)
{
    Focl_Command* cmd = (Focl_Command*)malloc(sizeof(Focl_Command));
    cmd->name = FoclStringPoolAlloc(strPool);
    FoclStrAssignStr(cmd->name, cmdName);
    cmd->func = Focl_evalProc;
    cmd->proc = FoclStringPoolAlloc(strPool);
    FoclStrAssignView(cmd->proc, procView);
    cmd->args = FoclStringPoolAlloc(strPool);
    FoclStrAssignView(cmd->args, argsView);
    return cmd;
}
void freeFoclCommand(Focl_Command* cmd, Focl_StringPool* strPool)
{
    if (cmd->proc != NULL)
    {
        FoclStringPoolFree(cmd->proc, strPool);
    }
    if (cmd->args != NULL)
    {
        FoclStringPoolFree(cmd->args, strPool);
    }
    free(cmd);
}
void FoclCommandOpDtVoid(void* cmd_, void* ctx)
{
    Focl_Command* cmd = cmd_;
    Focl_StringPool* strPool = ctx;
    if (cmd->proc != NULL)
    {
        FoclStringPoolFree(cmd->proc, strPool);
    }
    if (cmd->args != NULL)
    {
        FoclStringPoolFree(cmd->args, strPool);
    }
}

Focl_CommandTable* createFoclCommandTable()
{
    return createFoclHashTable(FOCL_COMMAND_TABLE_INIT_CAPACITY, FOCL_COMMAND_TABLE_LOAD_FACTOR, hashDjb2AsArg);
}
Focl_Command* FindCommandInTable(Focl_CommandTable* cmdTable, const Focl_String* str)
{
    return (Focl_Command*)FoclHashTableFind(cmdTable, (void*)str, StrKeyCompare);
}
void freeFoclCommandTable(Focl_CommandTable* cmdTable, Focl_StringPool* strPool)
{
    Focl_ValueOpDt valueOpDt = {.func = FoclCommandOpDtVoid, .ctx = strPool};
    freeFoclHashTable(cmdTable, NULL, &valueOpDt);
}

/* COMMAND TABLE */

/* ENVIRONMENT */

/* If parent_ is NULL, it will create a root(or global) environment. and also, if the parent_ is null, it will neglect the envName and keep using "::" */
Focl_Environment* createFoclEnvironment(Focl_Environment* parent_, Focl_StringPool* strPool, Focl_VectorPool* strVecPool, char* envName)
{
    Focl_Environment* env = (Focl_Environment*)malloc(sizeof(Focl_Environment));
    env->parent = parent_;
    if (parent_ != NULL)
    {
        env->level = parent_->level + 1;
    }
    else
    {
        env->level = 0;
    }
    env->cmdTable = createFoclCommandTable();
    env->objTable = createFoclObjTable();
    env->envNamespace = FoclStringPoolAlloc(strPool);
    if (parent_ != NULL)
    {
        FoclStrAssignStr(env->envNamespace, parent_->envNamespace);
        FoclStrAppend(env->envNamespace, envName);
    }
    FoclStrAppend(env->envNamespace, "::");
    env->namespaceVec = FoclVectorPoolAlloc(strVecPool);
    return env;
}
void freeFoclEnvironment(Focl_Environment* env, Focl_Context* context)
{
    freeFoclCommandTable(env->cmdTable, context->strPool);
    freeFoclObjTable(env->objTable, context);
    size_t nsCount = FoclVectorGetSize(env->namespaceVec);
    for (size_t i = 0; i < nsCount; i++)
    {
        Focl_String* ns = *(Focl_String**)FoclVectorAtNoCheck(env->namespaceVec, i);
        FoclStringPoolFree(ns, context->strPool);
    }
    FoclVectorPoolFree(env->namespaceVec, context->strVecPool);
    FoclStringPoolFree(env->envNamespace, context->strPool);
    free(env);
}

/* ENVIRONMENT */

/* CONTEXT */

Focl_Context* createFoclContext(FILE* outpotfPtr)
{
    Focl_Context* context = (Focl_Context*)malloc(sizeof(Focl_Context));
    context->strPool = createFoclStringPool();
    context->objVecPool = createFoclVectorPool(sizeof(Focl_Object*));
    context->strVecPool = createFoclVectorPool(sizeof(Focl_String*));
    context->objWithNoStrPool = createFoclObjWithNoStringPool();
    context->strObjPool = createFoclStringObjPool(context->strPool);
    context->cmpdObjPool = createFoclCmpdObjPool(context->objVecPool);
    context->outBuffer = createFoclIOBuffer(outpotfPtr, FOCL_IOBUFFER_DEFAULT_SIZE);
    context->globalEnv = createFoclEnvironment(NULL, context->strPool, context->strVecPool, NULL);
    context->curEnv = context->globalEnv;
    context->exitCode = 0;
    context->hasBreakBuf = false;
    context->hasContinueBuf = false;
    context->hasExitBuf = false;
    context->hasReturnBuf = false;
    context->returnValue = NULL;
    return context;
}
void freeFoclContext(Focl_Context* context)
{
    Focl_Environment* cEnv = context->curEnv;
    Focl_Environment* pEnv;
    do
    {
        pEnv = cEnv->parent;
        freeFoclEnvironment(cEnv, context);
        cEnv = pEnv;
    }
    while (cEnv != NULL);
    freeFoclCmpdObjPool(context);
    freeFoclStringObjPool(context->strObjPool, context->strPool);
    freeFoclObjWithNoStringPool(context->objWithNoStrPool);
    freeFoclStringPool(context->strPool);
    freeFoclVectorPool(context->objVecPool);
    freeFoclIOBuffer(context->outBuffer);
    free(context);
}
void FoclContextCreateEnterChildEnv(Focl_Context* context, char* envName)
{
    Focl_Environment* childEnv = createFoclEnvironment(context->curEnv, context->strPool, context->strVecPool, envName);
    context->curEnv = childEnv;
}
void FoclContextExitFreeChildEnv(Focl_Context* context)
{
    /* Well, maybe I should protect the system not leave the root env. But
       doesn't need. You should prevent it logically. */
    Focl_Environment* parentEnv = context->curEnv->parent;
    freeFoclEnvironment(context->curEnv, context);
    context->curEnv = parentEnv;
}

/* Firstly, it will search under current namespace, then the imported. */
Focl_Object* Focl_FindObject(Focl_Environment* env, Focl_StringPool* strPool, const Focl_String* target)
{
    Focl_Object* obj;
    Focl_String* tmpStr = FoclStringPoolAlloc(strPool);
    FoclStrAssignStr(tmpStr, env->envNamespace);
    FoclStrAppendStr(tmpStr, target);
    obj = FindObjectInTable(env->objTable, tmpStr);
    if (obj != FOCL_OBJECT_ERROR)
    {
        FoclStringPoolFree(tmpStr, strPool);
        return obj;
    }
    size_t nsVecSize = FoclVectorGetSize(env->namespaceVec);
    for (size_t i = 0; i < nsVecSize; i++)
    {
        FoclStrClear(tmpStr);
        FoclStrAssignStr(tmpStr, *(Focl_String**)FoclVectorAtNoCheck(env->namespaceVec, i));
        FoclStrAppendStr(tmpStr, target);
        obj = FindObjectInTable(env->objTable, tmpStr);
        if (obj != FOCL_OBJECT_ERROR)
        {
            FoclStringPoolFree(tmpStr, strPool);
            return obj;
        }
    }
    FoclStringPoolFree(tmpStr, strPool);
    return FOCL_OBJECT_ERROR;
}

Focl_Command* FindCommandInEnvironment(Focl_Environment* env, Focl_StringPool* strPool, const Focl_String* target)
{
    Focl_Command* cmd;
    Focl_String* tmpStr = FoclStringPoolAlloc(strPool);
    FoclStrAssignStr(tmpStr, env->envNamespace);
    FoclStrAppendStr(tmpStr, target);
    cmd = FindCommandInTable(env->cmdTable, tmpStr);
    if (cmd != FOCL_COMMAND_ERROR)
    {
        FoclStringPoolFree(tmpStr, strPool);
        return cmd;
    }
    size_t nsVecSize = FoclVectorGetSize(env->namespaceVec);
    for (size_t i = 0; i < nsVecSize; i++)
    {
        FoclStrClear(tmpStr);
        FoclStrAssignStr(tmpStr, *(Focl_String**)FoclVectorAtNoCheck(env->namespaceVec, i));
        FoclStrAppendStr(tmpStr, target);
        cmd = FindCommandInTable(env->cmdTable, tmpStr);
        if (cmd != FOCL_COMMAND_ERROR)
        {
            FoclStringPoolFree(tmpStr, strPool);
            return cmd;
        }
    }
    FoclStringPoolFree(tmpStr, strPool);
    return FOCL_COMMAND_ERROR;
}
Focl_Command* Focl_FindCommand(Focl_Context* context, const Focl_String* target)
{
    Focl_Command* cmd;
    Focl_Environment* currentEnv = context->curEnv;
    while (currentEnv != NULL)
    {
        cmd = FindCommandInEnvironment(currentEnv, context->strPool, target);
        if (cmd != FOCL_COMMAND_ERROR)
        {
            return cmd;
        }
        currentEnv = currentEnv->parent;
    }
    return FOCL_COMMAND_ERROR;
}

void exprSkipSpace(Focl_ExprParser* p)
{
    while (p->pos < p->end && isspace(*p->pos))
    {
        p->pos++;
    }
}
bool exprIsDigit(char c)
{
    return (c >= '0' && c <= '9');
}
Focl_Object* exprParseNumber(Focl_ExprParser* p)
{
    const char* start = p->pos;
    bool hasDot = false;
    while (p->pos < p->end && (exprIsDigit(*p->pos) || *p->pos == '.'))
    {
        if (*p->pos == '.')
        {
            if (hasDot) break;
            hasDot = true;
        }
        p->pos++;
    }

    size_t len = p->pos - start;
    if (len == 0)
    {
        return NULL;
    }
    char saved = *p->pos;
    *((char*)p->pos) = '\0';

    Focl_Object* obj;
    if (hasDot)
    {
        obj = FoclObjWithNoStringPoolAlloc(p->context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        obj->as.f = Focl_StrToFloat(start);
    }
    else
    {
        obj = FoclObjWithNoStringPoolAlloc(p->context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
        obj->as.i = Focl_StrToInt(start);
    }

    *((char*)p->pos) = saved;
    return obj;
}

Focl_Object* exprParseString(Focl_ExprParser* p)
{
    if (p->pos >= p->end || *p->pos != '"')
    {
        return NULL;
    }

    p->pos++;
    const char* start = p->pos;

    while (p->pos < p->end && *p->pos != '"')
    {
        if (*p->pos == '\\' && p->pos + 1 < p->end)
        {
            p->pos++;
        }
        p->pos++;
    }

    size_t len = p->pos - start;
    if (p->pos >= p->end)
    {
        return NULL;
    }

    p->pos++;

    Focl_StringView sv = {len, (char*)start};
    Focl_Object* obj = FoclStringObjPoolAlloc(p->context->strObjPool, p->context->strPool, FOCL_OBJ_TYPE_STR);
    FoclStrAssignView(obj->as.data, &sv);
    return obj;
}

Focl_Object* exprParseVariable(Focl_ExprParser* p)
{
    const char* start = p->pos;
    if (!((*p->pos >= 'a' && *p->pos <= 'z') ||
          (*p->pos >= 'A' && *p->pos <= 'Z') ||
          *p->pos == '_' || *p->pos == ':'))
    {
        return NULL;
    }
    p->pos++;
    while (p->pos < p->end &&
           ((*p->pos >= 'a' && *p->pos <= 'z') ||
            (*p->pos >= 'A' && *p->pos <= 'Z') ||
            (*p->pos >= '0' && *p->pos <= '9') ||
            *p->pos == '_' || *p->pos == ':'))
    {
        p->pos++;
    }
    size_t len = p->pos - start;
    Focl_StringView varName = {len, (char*)start};
    Focl_String tmpStr;
    char saved = initTempFoclStringWithView(&tmpStr, &varName);
    Focl_Object* var = Focl_FindObject(p->context->curEnv, p->context->strPool, &tmpStr);
    restoreFoclStringViewFromTempString(&varName, saved);
    if (var == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(p->context->strObjPool, p->context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    FoclObjectRetain(var);
    return var;
}

Focl_Object* exprParsePrimary(Focl_ExprParser* p)
{
    exprSkipSpace(p);
    if (p->pos >= p->end) return NULL;
    if (*p->pos == '(')
    {
        p->pos++;
        Focl_Object* obj = exprParseExpression(p);
        exprSkipSpace(p);
        if (p->pos < p->end && *p->pos == ')')
        {
            p->pos++;
            return obj;
        }
        if (obj)
        {
            FoclObjectRelease(obj, p->context);
        }
        return FoclObjectError(p->context->strObjPool, p->context->strPool, "Missing closing parenthesis");
    }
    if (*p->pos == '-')
    {
        p->pos++;
        Focl_Object* operand = exprParsePrimary(p);
        if (operand->type == FOCL_OBJ_TYPE_ERROR)
        {
            return operand;
        }

        Focl_Object* result = NULL;
        if (operand->type == FOCL_OBJ_TYPE_INT)
        {
            result = FoclObjWithNoStringPoolAlloc(p->context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
            result->as.i = -FoclObjectUnboxInt(operand);
        }
        else if (operand->type == FOCL_OBJ_TYPE_FLOAT)
        {
            result = FoclObjWithNoStringPoolAlloc(p->context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
            result->as.f = -FoclObjectUnboxFloat(operand);
        }
        else
        {
            FoclObjectRelease(operand, p->context);
            return FoclObjectError(p->context->strObjPool, p->context->strPool, "Cannot negate non-numeric value");
        }
        FoclObjectRelease(operand, p->context);
        return result;
    }
    if (*p->pos == '"')
    {
        return exprParseString(p);
    }
    if (exprIsDigit(*p->pos))
    {
        return exprParseNumber(p);
    }
    if ((*p->pos >= 'a' && *p->pos <= 'z') ||
        (*p->pos >= 'A' && *p->pos <= 'Z') ||
        *p->pos == '_')
        return exprParseVariable(p);

    return FoclObjectError(p->context->strObjPool, p->context->strPool, "Unexpected character in expression");
}
Focl_Object* exprParseMulDiv(Focl_ExprParser* p)
{
    Focl_Object* left = exprParsePrimary(p);
    if (left == NULL || left->type == FOCL_OBJ_TYPE_ERROR)
    {
        return left;
    }

    while (1)
    {
        exprSkipSpace(p);
        if (p->pos >= p->end)
        {
            break;
        }

        char op = *p->pos;
        if (op != '*' && op != '/' && op != '%')
        {
            break;
        }

        p->pos++;

        Focl_Object* right = exprParsePrimary(p);
        if (right == NULL || right->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(left, p->context);
            return right;
        }

        if (left->type == FOCL_OBJ_TYPE_FLOAT || right->type == FOCL_OBJ_TYPE_FLOAT)
        {
            double l = (left->type == FOCL_OBJ_TYPE_INT) ? (double)left->as.i : left->as.f;
            double r = (right->type == FOCL_OBJ_TYPE_INT) ? (double)right->as.i : right->as.f;

            FoclObjectRelease(left, p->context);
            FoclObjectRelease(right, p->context);

            left = FoclObjWithNoStringPoolAlloc(p->context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
            switch (op)
            {
                case '*': left->as.f = l * r; break;
                case '/':
                    if (r == 0.0)
                    {
                        FoclObjectRelease(left, p->context);
                        return FoclObjectError(p->context->strObjPool, p->context->strPool, "Division by zero");
                    }
                    left->as.f = l / r;
                    break;
                case '%':
                    FoclObjectRelease(left, p->context);
                    return FoclObjectError(p->context->strObjPool, p->context->strPool, "Modulo requires integer operands");
            }
        }
        else if (left->type == FOCL_OBJ_TYPE_INT && right->type == FOCL_OBJ_TYPE_INT)
        {
            Focl_Obj_Int l = left->as.i;
            Focl_Obj_Int r = right->as.i;

            FoclObjectRelease(left, p->context);
            FoclObjectRelease(right, p->context);

            left = FoclObjWithNoStringPoolAlloc(p->context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
            switch (op)
            {
                case '*': left->as.i = l * r; break;
                case '/':
                    if (r == 0)
                    {
                        FoclObjectRelease(left, p->context);
                        return FoclObjectError(p->context->strObjPool, p->context->strPool, "Division by zero");
                    }
                    left->as.i = l / r;
                    break;
                case '%':
                    if (r == 0)
                    {
                        FoclObjectRelease(left, p->context);
                        return FoclObjectError(p->context->strObjPool, p->context->strPool, "Modulo by zero");
                    }
                    left->as.i = l % r;
                    break;
            }
        }
        else
        {
            FoclObjectRelease(left, p->context);
            FoclObjectRelease(right, p->context);
            return FoclObjectError(p->context->strObjPool, p->context->strPool, "Invalid operand types for arithmetic");
        }
    }

    return left;
}

Focl_Object* exprParseAddSub(Focl_ExprParser* p)
{
    Focl_Object* left = exprParseMulDiv(p);
    if (left == NULL || left->type == FOCL_OBJ_TYPE_ERROR)
    {
        return left;
    }
    while (1)
    {
        exprSkipSpace(p);
        if (p->pos >= p->end)
        {
            break;
        }
        char op = *p->pos;
        if (op != '+' && op != '-')
        {
            break;
        }

        p->pos++;
        Focl_Object* right = exprParseMulDiv(p);
        if (right == NULL || right->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(left, p->context);
            return right;
        }
        if (left->type == FOCL_OBJ_TYPE_FLOAT || right->type == FOCL_OBJ_TYPE_FLOAT)
        {
            double l = (left->type == FOCL_OBJ_TYPE_INT) ? (double)left->as.i : left->as.f;
            double r = (right->type == FOCL_OBJ_TYPE_INT) ? (double)right->as.i : right->as.f;

            FoclObjectRelease(left, p->context);
            FoclObjectRelease(right, p->context);

            left = FoclObjWithNoStringPoolAlloc(p->context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
            left->as.f = (op == '+') ? (l + r) : (l - r);
        }
        else if (left->type == FOCL_OBJ_TYPE_INT && right->type == FOCL_OBJ_TYPE_INT)
        {
            Focl_Obj_Int l = left->as.i;
            Focl_Obj_Int r = right->as.i;

            FoclObjectRelease(left, p->context);
            FoclObjectRelease(right, p->context);

            left = FoclObjWithNoStringPoolAlloc(p->context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
            left->as.i = (op == '+') ? (l + r) : (l - r);
        }
        else
        {
            FoclObjectRelease(left, p->context);
            FoclObjectRelease(right, p->context);
            return FoclObjectError(p->context->strObjPool, p->context->strPool, "Invalid operand types for arithmetic");
        }
    }

    return left;
}

Focl_Object* exprParseComparison(Focl_ExprParser* p)
{
    Focl_Object* left = exprParseAddSub(p);
    if (left == NULL || left->type == FOCL_OBJ_TYPE_ERROR)
    {
        return left;
    }
    exprSkipSpace(p);
    if (p->pos >= p->end)
    {
        return left;
    }

    const char* ops[] = {"==", "!=", "<=", ">=", "<", ">"};
    for (int k = 0; k < 6; k++)
    {
        size_t opLen = strlen(ops[k]);
        if (p->pos + opLen <= p->end && memcmp(p->pos, ops[k], opLen) == 0)
        {
            p->pos += opLen;

            Focl_Object* right = exprParseAddSub(p);
            if (right == NULL || right->type == FOCL_OBJ_TYPE_ERROR)
            {
                FoclObjectRelease(left, p->context);
                return right;
            }

            if (left->type != FOCL_OBJ_TYPE_INT || right->type != FOCL_OBJ_TYPE_INT)
            {
                FoclObjectRelease(left, p->context);
                FoclObjectRelease(right, p->context);
                return FoclObjectError(p->context->strObjPool, p->context->strPool, "Comparison requires integer operands");
            }

            Focl_Obj_Int l = left->as.i;
            Focl_Obj_Int r = right->as.i;
            bool result = false;
            switch (k)
            {
                case 0: result = (l == r); break;
                case 1: result = (l != r); break;
                case 2: result = (l <= r); break;
                case 3: result = (l >= r); break;
                case 4: result = (l < r); break;
                case 5: result = (l > r); break;
            }

            FoclObjectRelease(left, p->context);
            FoclObjectRelease(right, p->context);
            return FoclObjectBool(p->context->objWithNoStrPool, result ? FOCL_OBJ_TRUE : FOCL_OBJ_FALSE);
        }
    }

    return left;
}

Focl_Object* exprParseExpression(Focl_ExprParser* p)
{
    return exprParseComparison(p);
}

/* CONTEXT */

Focl_StringView getNextWord(Focl_StringView* start)
{
    char* ptr = start->strPtr;
    char* startEnd = start->strPtr + start->len;
    
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
    
    char* wordStart = ptr;
    
    if (*ptr == '"')
    {
        ptr++;
        while (ptr < startEnd && *ptr != '"')
        {
            if (*ptr == '\\' && (ptr + 1) < startEnd)
            {
                ptr++;
            }
            ptr++;
        }
        if (ptr < startEnd)
        {
            ptr++;
        }
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
    char* ptr = start->strPtr;
    char* startEnd = start->strPtr + start->len;
    
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
    if (*ptr == '#')
    {
        while (ptr < startEnd && *ptr != '\n')
        {
            ptr++;
        }
        if (ptr < startEnd) ptr++;
        start->strPtr = ptr;
        start->len = startEnd - ptr;
        return getNextLine(start);
    }
    
    char* wordStart = ptr;
    bool inString = false;
    int braceDepth = 0;
    int bracketDepth = 0;
    
    while (ptr < startEnd)
    {
        if (*ptr == '\\' && (inString || braceDepth > 0 || bracketDepth > 0))
        {
            ptr++;
            if (ptr < startEnd) ptr++;
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
                if (braceDepth > 0) braceDepth--;
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
                if (bracketDepth > 0) bracketDepth--;
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
    if (ptr < startEnd) ptr++;
    start->strPtr = ptr;
    start->len = startEnd - ptr;
    return (Focl_StringView){wordLen, (char*)wordStart};
}
bool isWordParseEnd(const Focl_StringView* strView, const char* pos)
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
                Focl_String tmpStr;
                char saved = initTempFoclStringWithView(&tmpStr, &varName);
                Focl_Object* objPtr = Focl_FindObject(context->curEnv, context->strPool, &tmpStr);
                restoreFoclStringViewFromTempString(&varName, saved);
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
                    else if (isFoclObjectCompound(object))
                    {
                        Focl_String* cmpdStr = FoclCmpdObjStringize(object, context->strPool);
                        if (cmpdStr != NULL && cmpdStr->length > 0)
                        {
                            size_t curLen = dst->length;
                            size_t needed = curLen + cmpdStr->length;
                            while (needed >= dst->capacity)
                            {
                                FoclStrReserve(dst, (dst->capacity == 0) ? 16 : dst->capacity * 2);
                            }
                            memcpy(dst->data + curLen, cmpdStr->data, cmpdStr->length);
                            dst->length = curLen + cmpdStr->length;
                            dst->data[dst->length] = '\0';
                        }
                        FoclStringPoolFree(cmpdStr, context->strPool);
                    }
                    else
                    {
                        Focl_String tempStr;
                        switch (object->type)
                        {
                            case FOCL_OBJ_TYPE_INT:
                                FoclStringOpCt(&tempStr, FOCL_INT_TO_STR_TMP_BUFFER_SIZE);
                                tempStr.length = sprintf(tempStr.data, "%" FOCL_FORMAT_INT, object->as.i);
                                break;
                            case FOCL_OBJ_TYPE_FLOAT:
                                FoclStringOpCt(&tempStr, FOCL_FLOAT_TO_STR_TMP_BUFFER_SIZE);
                                tempStr.length = sprintf(tempStr.data, "%" FOCL_FORMAT_FLOAT, object->as.f);
                                break;
                            case FOCL_OBJ_TYPE_BOOL:
                                FoclStringOpCt(&tempStr, (object->as.i == FOCL_OBJ_TRUE) ? sizeof("true") : sizeof("false"));
                                tempStr.length = sprintf(tempStr.data, "%s", (object->as.i == FOCL_OBJ_TRUE) ? "true" : "false");
                                break;
                            default:
                                FoclStringOpCt(&tempStr, FOCL_FLOAT_TO_STR_TMP_BUFFER_SIZE);
                                printf(FOCL_ERR_YSNBH);
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
                        FoclStringOpDt(&tempStr);
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

                if (cmdResult != NULL && cmdResult->type != FOCL_OBJ_TYPE_ERROR)
                {
                    if (isFoclObjectUseString(cmdResult))
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
                    else if (isFoclObjectCompound(cmdResult))
                    {
                        Focl_String* cmpdStr = FoclCmpdObjStringize(cmdResult, context->strPool);
                        if (cmpdStr != NULL && cmpdStr->length > 0)
                        {
                            size_t curLen = dst->length;
                            size_t needed = curLen + cmpdStr->length;
                            while (needed >= dst->capacity)
                            {
                                FoclStrReserve(dst, (dst->capacity == 0) ? 16 : dst->capacity * 2);
                            }
                            memcpy(dst->data + curLen, cmpdStr->data, cmpdStr->length);
                            dst->length = curLen + cmpdStr->length;
                            dst->data[dst->length] = '\0';
                        }
                        FoclStringPoolFree(cmpdStr, context->strPool);
                    }
                    FoclObjectRelease(cmdResult, context);
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
        return FoclObjectError(context->strObjPool, context->strPool, "Empty boolean expression");
    }

    Focl_StringView trimmed = {len, (char*)start};

    if (trimmed.strPtr[0] == '!')
    {
        Focl_StringView inner = {trimmed.len - 1, trimmed.strPtr + 1};
        Focl_Object* innerObj = Focl_exprBool(context, &inner);
        if (innerObj->type == FOCL_OBJ_TYPE_ERROR) return innerObj;
        Focl_Obj_Bool val = innerObj->as.i;
        FoclObjectRelease(innerObj, context);
        return FoclObjectBool(context->objWithNoStrPool, !val);
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
                FoclObjectRelease(leftObj, context);
                if (!lv) return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_FALSE);

                Focl_Object* rightObj = Focl_exprBool(context, &right);
                if (rightObj->type == FOCL_OBJ_TYPE_ERROR) return rightObj;
                Focl_Obj_Bool rv = rightObj->as.i;
                FoclObjectRelease(rightObj, context);
                return FoclObjectBool(context->objWithNoStrPool, rv);
            }
            if (c == '|' && i + 1 < trimmed.len && trimmed.strPtr[i + 1] == '|')
            {
                Focl_StringView left = {i, trimmed.strPtr};
                Focl_StringView right = {trimmed.len - i - 2, trimmed.strPtr + i + 2};

                Focl_Object* leftObj = Focl_exprBool(context, &left);
                if (leftObj->type == FOCL_OBJ_TYPE_ERROR) return leftObj;
                Focl_Obj_Bool lv = leftObj->as.i;
                FoclObjectRelease(leftObj, context);
                if (lv) return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_TRUE);

                Focl_Object* rightObj = Focl_exprBool(context, &right);
                if (rightObj->type == FOCL_OBJ_TYPE_ERROR) return rightObj;
                Focl_Obj_Bool rv = rightObj->as.i;
                FoclObjectRelease(rightObj, context);
                return FoclObjectBool(context->objWithNoStrPool, rv);
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
                    return FoclObjectError(context->strObjPool, context->strPool, "Missing operand in comparison");

                Focl_Object* leftObj = getFoclObjectWithStringView(context, &left);
                Focl_Object* rightObj = getFoclObjectWithStringView(context, &right);

                if (leftObj->type == FOCL_OBJ_TYPE_ERROR || rightObj->type == FOCL_OBJ_TYPE_ERROR)
                {
                    FoclObjectRelease(leftObj, context);
                    FoclObjectRelease(rightObj, context);
                    return FoclObjectError(context->strObjPool, context->strPool, "Invalid operand in comparison");
                }

                if (leftObj->type != FOCL_OBJ_TYPE_INT || rightObj->type != FOCL_OBJ_TYPE_INT)
                {
                    FoclObjectRelease(leftObj, context);
                    FoclObjectRelease(rightObj, context);
                    return FoclObjectError(context->strObjPool, context->strPool, "Comparison operands must be integers");
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

                FoclObjectRelease(leftObj, context);
                FoclObjectRelease(rightObj, context);
                return FoclObjectBool(context->objWithNoStrPool, result);
            }
        }
    }

    if (trimmed.len == 4 && memcmp(trimmed.strPtr, "true", 4) == 0)
    {
        return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_TRUE);
    }
    if (trimmed.len == 5 && memcmp(trimmed.strPtr, "false", 5) == 0)
    {
        return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_FALSE);
    }

    Focl_Object* obj = getFoclObjectWithStringView(context, &trimmed);

    if (obj->type == FOCL_OBJ_TYPE_ERROR)
    {
        return obj;
    }

    if (obj->type != FOCL_OBJ_TYPE_BOOL)
    {
        FoclObjectRelease(obj, context);
        return FoclObjectError(context->strObjPool, context->strPool, "Expected boolean expression, got non-boolean value");
    }

    Focl_Obj_Bool result = obj->as.i;
    FoclObjectRelease(obj, context);
    return FoclObjectBool(context->objWithNoStrPool, result);
}

/* EXPRESSION */

Focl_Object* FoclObjVecAt(Focl_Vector* objVec, size_t idx)
{
    return *(Focl_Object**)FoclVectorAtNoCheck(objVec, idx);
}
Focl_String* FoclObjVecAtAsString(Focl_Vector* objVec, size_t idx)
{
    return (*(Focl_Object**)FoclVectorAtNoCheck(objVec, idx))->as.data;
}
Focl_StringView FoclObjVecAtAsStringToView(Focl_Vector* objVec, size_t idx)
{
    Focl_String* str = FoclObjVecAtAsString(objVec, idx);
    Focl_StringView strView = {str->length, str->data};
    return strView;
}

Focl_Object* Focl_parseBlock(Focl_Context* context, Focl_StringView* strView)
{
    Focl_StringView inner = FoclStringViewPeelBoth(strView);
    return Focl_parseCommandSequence(context, &inner);
}
Focl_Object* Focl_parseCommand(Focl_Context* context, const Focl_StringView* strView)
{
    Focl_StringView remaining = *strView;
    Focl_StringView cmdView = getNextWord(&remaining);
    if (cmdView.strPtr == NULL || cmdView.len == 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNKNOWN_COMMAND);
    }
    Focl_String tmpCmdStr;
    char saved = initTempFoclStringWithView(&tmpCmdStr, &cmdView);
    Focl_Command* cmd = Focl_FindCommand(context, &tmpCmdStr);
    restoreFoclStringViewFromTempString(&cmdView, saved);
    if (cmd == FOCL_COMMAND_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNKNOWN_COMMAND);
    }
    Focl_Vector* vec = FoclVectorPoolAlloc(context->objVecPool);
    Focl_Object* obj;
    while (remaining.len > 0)
    {
        Focl_StringView argView = getNextWord(&remaining);
        if (argView.strPtr == NULL || argView.len == 0)
        {
            break;
        }
        obj = getFoclObjectWithStringView(context, &argView);
        FoclVectorPushBack(vec, &obj);
    }
    obj = cmd->func(context, vec, cmd);
    for (size_t i = 0; i < FoclVectorGetSize(vec); i++)
    {
        FoclObjectRelease(FoclObjVecAt(vec, i), context);
    }
    FoclVectorPoolFree(vec, context->objVecPool);
    return obj;
}
Focl_Object* Focl_parseCommandSequence(Focl_Context* context, Focl_StringView* strView)
{
    Focl_StringView remaining = *strView;
    Focl_Object* lastResult = FoclObjectVoid(context->strObjPool, context->strPool);
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
        FoclObjectRelease(lastResult, context);
        lastResult = result;
        if (result->type == FOCL_OBJ_TYPE_ERROR)
        {
            break;
        }
    }
    return lastResult;
}

Focl_Object* Focl_parseLine(Focl_Context* context, Focl_String* lineStr)
{
    Focl_StringView strView = {lineStr->length, lineStr->data};
    return Focl_parseCommandSequence(context, &strView);
}

Focl_IOBuffer* createFoclIOBuffer(FILE* fptr_, int bufferSize)
{
    Focl_IOBuffer* ioBuffer = (Focl_IOBuffer*)malloc(sizeof(Focl_IOBuffer));
    ioBuffer->size = bufferSize;
    ioBuffer->buf = malloc(sizeof(char) * bufferSize);
    ioBuffer->used = 0;
    ioBuffer->fPtr = fptr_;
    return ioBuffer;
}
void freeFoclIOBuffer(Focl_IOBuffer* ioBuffer)
{
    free(ioBuffer->buf);
    free(ioBuffer);
}
void FoclIOBufferFlushOut(Focl_IOBuffer* ioBuffer)
{
    ioBuffer->buf[ioBuffer->used] = '\0';
    fputs(ioBuffer->buf, ioBuffer->fPtr);
    ioBuffer->used = 0;
}
void FoclIOBufferPrintf(Focl_IOBuffer* ioBuffer, const char* fmt, ...)
{
    va_list args, args_copy;
    va_start(args, fmt);
    
    int remaining = ioBuffer->size - ioBuffer->used;
    int written;
    
    va_copy(args_copy, args);
    written = vsnprintf(ioBuffer->buf + ioBuffer->used, remaining, fmt, args_copy);
    va_end(args_copy);
    
    if (written < 0)
    {
        va_end(args);
        return;
    }
    
    if (written >= remaining)
    {
        FoclIOBufferFlushOut(ioBuffer);
        va_copy(args_copy, args);
        written = vsnprintf(ioBuffer->buf, ioBuffer->size, fmt, args_copy);
        va_end(args_copy);
        if (written < 0 || written >= ioBuffer->size)
        {
            vfprintf(ioBuffer->fPtr, fmt, args);
            va_end(args);
            return;
        }
        ioBuffer->used = written;
    }
    else
    {
        ioBuffer->used += written;
    }
    va_end(args);
    if (ioBuffer->used > 0 && (ioBuffer->buf[ioBuffer->used - 1] == '\n' || ioBuffer->used == ioBuffer->size))
    {
        FoclIOBufferFlushOut(ioBuffer);
    }
}
void FoclIOBufferPutChar(Focl_IOBuffer* ioBuffer, char c)
{
    if (ioBuffer->used >= ioBuffer->size)
    {
        FoclIOBufferFlushOut(ioBuffer);
    }
    ioBuffer->buf[ioBuffer->used++] = c;
    if (c == '\n')
    {
        FoclIOBufferFlushOut(ioBuffer);
    }
}

void FoclObjectPrint(Focl_Object* obj, Focl_IOBuffer* oBuffer, Focl_StringPool* strPool)
{
    switch (obj->type)
    {
        case FOCL_OBJ_TYPE_INT:
            FoclIOBufferPrintf(oBuffer, "%" FOCL_FORMAT_INT, FoclObjectUnboxInt(obj));
            break;
        case FOCL_OBJ_TYPE_FLOAT:
            FoclIOBufferPrintf(oBuffer, "%" FOCL_FORMAT_FLOAT, FoclObjectUnboxFloat(obj));
            break;
        case FOCL_OBJ_TYPE_BOOL:
            FoclIOBufferPrintf(oBuffer, "%s", obj->as.i ? "true" : "false");
            break;
        case FOCL_OBJ_TYPE_STR: /* FALLTHROUGH */
        case FOCL_OBJ_TYPE_ERROR:
            FoclIOBufferPrintf(oBuffer, "%s", FoclStrCStr(obj->as.data));
            break;
        case FOCL_OBJ_TYPE_VOID:
            break;
        case FOCL_OBJ_TYPE_COMPOUND:
            Focl_String* cmpdStr = FoclCmpdObjStringize(obj, strPool);
            FoclIOBufferPrintf(oBuffer, "%s", FoclStrCStr(cmpdStr));
            FoclStringPoolFree(cmpdStr, strPool);
            break;
        default:
            printf(FOCL_ERR_YSNBH);
            break;
    }
}

Focl_Object* FoclObjectScan(Focl_StrObjPool* strObjPool, Focl_StringPool* strPool, Focl_Object* obj)
{
    size_t len;
    char* input = Focl_getline(stdin, &len);
    if (input == NULL)
    {
        return FoclObjectError(strObjPool, strPool, FOCL_ERR_READ_ERR_STDIN);
    }
    Focl_Object* result = obj;
    if (Focl_isInteger(input))
    {
        if (obj->type != FOCL_OBJ_TYPE_INT)
        {
            result = FoclObjectError(strObjPool, strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
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
            result = FoclObjectError(strObjPool, strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
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
            result = FoclObjectError(strObjPool, strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
        }
        else
        {
            FoclStrAssign(obj->as.data, input);
        }
    }
    free(input);
    return result;
}

Focl_Object* Focl_evalProc(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    Focl_StringView argsView = { cmd->args->length, cmd->args->data };
    Focl_Object* retValue;
    FoclContextCreateEnterChildEnv(context, FoclStrCStr(cmd->name));
    if (argsView.len > 2)
    {
        Focl_StringView argView = FoclStringViewPeelBoth(&argsView);
        size_t argCount = FoclVectorGetSize(objVec);
        Focl_StringView remaining = argView;

        Focl_String* argTmpStr = FoclStringPoolAlloc(context->strPool);
        for (size_t i = 0; i < argCount; i++)
        {
            Focl_StringView arg = getNextWord(&remaining);
            if (arg.strPtr == NULL || arg.len == 0)
            {
                retValue = FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
                goto exitChildEnv;
            }
            Focl_Object* originObj = FoclObjVecAt(objVec, i);
            Focl_Object* newObj = FoclObjPoolAllocAssign(context, originObj);
            FoclStrClear(argTmpStr);
            FoclStrAssignStr(argTmpStr, context->curEnv->envNamespace);
            FoclStrAppendView(argTmpStr, &arg);
            LinkObjectWithName(context, newObj, argTmpStr);
        }
        FoclStringPoolFree(argTmpStr, context->strPool);
        Focl_StringView leftover = getNextWord(&remaining);
        if (leftover.strPtr != NULL && leftover.len != 0)
        {
            retValue = FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
            goto exitChildEnv;
        }
    }
    bool oldHasReturn = context->hasReturnBuf;
    context->hasReturnBuf = true;
    context->returnValue = NULL;
    if (setjmp(context->returnBuf) == 0)
    {
        Focl_StringView procView = {cmd->proc->length, cmd->proc->data};
        retValue = Focl_parseBlock(context, &procView);
    }
    else
    {
        retValue = context->returnValue;
    }
    context->hasReturnBuf = oldHasReturn;
    context->returnValue = NULL;

exitChildEnv:
    FoclContextExitFreeChildEnv(context);
    return retValue;
}
void FoclRegisterCommand(Focl_Context* context, const char* cmdName, Focl_CommandFunc func)
{
    Focl_String* _name = FoclStringPoolAlloc(context->strPool);
    FoclStrAssign(_name, "::");
    FoclStrAppend(_name, cmdName);
    Focl_Command* _cmd = createFoclCommandBuildIn(func, context->strPool, cmdName);
    Focl_KeyOpDt keyOpDt = {.ctx = context->strPool, .func = FoclStringPoolFreeOpDtVoid};
    FoclHashTableInsert(context->globalEnv->cmdTable, _name, _cmd, StrKeyCompare, &keyOpDt, NULL);
}

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

    char prev = '\0';
    for (const char* p = str; *p; p++)
    {
        if (*p == '\\' && inString)
        {
            p++;
            if (*p == '\0') break;
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

        if ((*p == '[' && prev != '\\') || (*p == '{' && prev != '\\'))
        {
            depth++;
        }
        else if ((*p == ']' && prev != '\\') || (*p == '}' && prev != '\\'))
        {
            depth--;
        }
        prev = *p;
    }
    return depth;
}
int Focl_ExecFile(Focl_Context* ctx, const char* filename)
{
    FILE* fp = fopen(filename, "r");
    if (fp == NULL)
    {
        FoclIOBufferPrintf(ctx->outBuffer, "Error: Cannot open file '%s'\n", filename);
        return 1;
    }
    Focl_String buffer;
    FoclStringOpCt(&buffer, FOCL_STRING_INIT_CAPACITY);
    int depth = 0;

    ctx->hasExitBuf = true;
    if (setjmp(ctx->exitBuf) != 0)
    {
        FoclStringOpDt(&buffer);
        fclose(fp);
        ctx->hasExitBuf = false;
        return ctx->exitCode;
    }

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
            FoclIOBufferPrintf(ctx->outBuffer, "Error: %s\n", FoclStrCStr(result->as.data));
            FoclObjectRelease(result, ctx);
            break;
        }
        FoclObjectRelease(result, ctx);
        FoclStrClear(&buffer);
        depth = 0;
    }

    if (depth > 0)
    {
        Focl_Object* result = Focl_parseLine(ctx, &buffer);
        if (result->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclIOBufferPrintf(ctx->outBuffer, "Error: %s\n", FoclStrCStr(result->as.data));
        }
        FoclObjectRelease(result, ctx);
    }

    ctx->hasExitBuf = false;
    FoclStringOpDt(&buffer);
    fclose(fp);
    return ctx->exitCode;
}
Focl_Object* Focl_eval(Focl_Context* context, const char* Cstr)
{
    Focl_String* str = FoclStringPoolAlloc(context->strPool);
    FoclStrAssign(str, Cstr);
    Focl_StringView strView = {str->length, str->data};
    Focl_Object* result = Focl_parseCommandSequence(context, &strView);
    FoclStringPoolFree(str, context->strPool);
    return result;
}