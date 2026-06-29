#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "focl_dev.h"
#include "sys_lean.h"

/* The basic command pack of focl. */

Focl_Object* buildIn_puts(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount == 1)
    {
        FoclObjectPrint(FoclObjVecAt(objVec, 0), context->outBuffer);
        FoclIOBufferPutChar(context->outBuffer, '\n');
    }
    else if (argCount == 2)
    {
        if (FoclStrComp(FoclObjVecAtAsString(objVec, 0), "-nonewline") == 0)
        {
            FoclObjectPrint(FoclObjVecAt(objVec, 1), context->outBuffer);
            FoclIOBufferFlushOut(context->outBuffer);
        }
        else
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNKNOWN_ARG);
        }
    }
    else
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    return FoclObjectVoid(context->strObjPool, context->strPool);
}
Focl_Object* buildIn_gets(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView inPipe = FoclObjVecAtAsStringToView(objVec, 0);
    if (FoclStringViewComp(&inPipe, "stdin") != 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNKNOWN_ARG);
    }
    Focl_StringView varView = FoclObjVecAtAsStringToView(objVec, 1);
    Focl_Object* obj = FindObjectInContext(context, &varView);
    if (obj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    Focl_Object* scanRet = FoclObjectScan(context->strObjPool, context->strPool, obj);
    if (scanRet->type == FOCL_OBJ_TYPE_ERROR)
    {
        return scanRet;
    }
    return FoclObjPoolAllocAssign(context, obj);
}
Focl_Object* buildIn_set(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView strView = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* obj = FindObjectInContext(context, &strView);
    if (obj == FOCL_OBJECT_ERROR)
    {
        /* Create it! */
        
        obj = FoclObjPoolAllocAssign(context, FoclObjVecAt(objVec, 1));
        LinkObjectWithName_View(context, obj, &strView);
    }
    else
    {
        /* Change it! */

        /* I decided to make Focl into a dynamic but strong type language! */

        Focl_Object* src = FoclObjVecAt(objVec, 1);
        if (obj->type != src->type)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
        }
        FoclObjectAssign(obj, src, context->strPool);
    }

    return FoclObjPoolAllocAssign(context, obj);
}
Focl_Object* buildIn_unset(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView strView = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* obj = FindObjectInContext(context, &strView);
    if (obj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    Focl_String tmpStr;
    char saved = initTempFoclStringWithView(&tmpStr, &strView);
    Focl_KeyOpDt keyOpDt = {.ctx = context->strPool, .func = FoclStringPoolFreeOpDtVoid};
    FoclHashTableDelete(context->curEnv->objTable, &tmpStr, StrKeyCompare, &keyOpDt, NULL);
    restoreFoclStringViewFromTempString(&strView, saved);
    FoclObjectRelease(obj, context);
    
    return FoclObjectVoid(context->strObjPool, context->strPool);
}
Focl_Object* buildIn_incr(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount == 0 || argCount > 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView strView = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* obj = FindObjectInContext(context, &strView);
    if (obj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    else if (obj->type != FOCL_OBJ_TYPE_INT)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
    }
    else
    {
        if (argCount == 1)
        {
            ++(obj->as.i);
        }
        else
        {
            Focl_Object* target;
            FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 1, target, context->strObjPool, context->strPool);
            obj->as.i += target->as.i;
        }
    }
    return FoclObjPoolAllocAssign(context, obj);
}
Focl_Object* buildIn_if(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount < 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }

    size_t i = 0;
    while (i < argCount)
    {
        Focl_StringView condBlock = FoclObjVecAtAsStringToView(objVec, i);
        if (condBlock.len <= 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCl_ERR_INVALID_BLOCK);
        }
        Focl_StringView condExpr = FoclStringViewPeelBoth(&condBlock);
        Focl_Object* condResult = Focl_exprBool(context, &condExpr);
        if (condResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            return condResult;
        }

        if (condResult->as.i == FOCL_OBJ_TRUE)
        {
            FoclObjectRelease(condResult, context);
            if (i + 1 >= argCount)
            {
                return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_NO_EXEC_BLOCK);
            }
            Focl_StringView execBlock = FoclObjVecAtAsStringToView(objVec, i + 1);
            return Focl_parseBlock(context, &execBlock);
        }
        FoclObjectRelease(condResult, context);

        if (i + 2 >= argCount)
        {
            return FoclObjectVoid(context->strObjPool, context->strPool);
        }

        Focl_StringView next = FoclObjVecAtAsStringToView(objVec, i + 2);
        if (FoclStringViewComp(&next, "elseif") == 0)
        {
            if (i + 4 >= argCount)
            {
                return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_NO_EXEC_BLOCK);
            }
            i += 2;
        }
        else if (FoclStringViewComp(&next, "else") == 0)
        {
            if (i + 3 >= argCount)
            {
                return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_NO_EXEC_BLOCK);
            }
            Focl_StringView elseBlock = FoclObjVecAtAsStringToView(objVec, i + 3);
            return Focl_parseBlock(context, &elseBlock);
        }
        else
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNKNOWN_ARG);
        }
    }
    return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_YSNBH);
}
Focl_Object* buildIn_while(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView condBlock = FoclObjVecAtAsStringToView(objVec, 0);
    if (condBlock.len <= 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCl_ERR_INVALID_BLOCK);
    }
    Focl_StringView condExpr = FoclStringViewPeelBoth(&condBlock);
    Focl_Object* condResult = Focl_exprBool(context, &condExpr);
    if (condResult->type == FOCL_OBJ_TYPE_ERROR)
    {
        return condResult;
    }
    Focl_Object* result = FoclObjectVoid(context->strObjPool, context->strPool);
    context->hasBreakBuf = true;
    context->hasContinueBuf = true;

    while (condResult->as.i == FOCL_OBJ_TRUE)
    {
        volatile int breakJmp = setjmp(context->breakBuf);
        if (breakJmp != 0)
        {
            FoclObjectRelease(condResult, context);
            break;
        }

        FoclObjectRelease(condResult, context);
        condResult = NULL;
        Focl_StringView execBlock = FoclObjVecAtAsStringToView(objVec, 1);
        Focl_Object* bodyResult = Focl_parseBlock(context, &execBlock);
        if (bodyResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(result, context);
            context->hasBreakBuf = false;
            context->hasContinueBuf = false;
            return bodyResult;
        }
        FoclObjectRelease(result, context);
        result = bodyResult;
        setjmp(context->continueBuf);
        condResult = Focl_exprBool(context, &condExpr);
        if (condResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(result, context);
            context->hasBreakBuf = false;
            context->hasContinueBuf = false;
            return condResult;
        }
    }

    context->hasBreakBuf = false;
    context->hasContinueBuf = false;
    if (condResult != NULL)
    {
        FoclObjectRelease(condResult, context);
    }
    return result;
}
Focl_Object* buildIn_for(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 4)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    for (size_t i = 0; i < 4; i++)
    {
        if (!isFoclObjectUseString(FoclObjVecAt(objVec, i)))
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_MUST_BE_BLOCK);
        }
    }
    Focl_StringView initBlock = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* initResult = Focl_parseBlock(context, &initBlock);
    if (initResult->type == FOCL_OBJ_TYPE_ERROR)
    {
        return initResult;
    }
    FoclObjectRelease(initResult, context);
    Focl_StringView condBlock = FoclObjVecAtAsStringToView(objVec, 1);
    if (condBlock.len <= 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCl_ERR_INVALID_BLOCK);
    }
    Focl_StringView condExpr = FoclStringViewPeelBoth(&condBlock);
    Focl_StringView updateBlock = FoclObjVecAtAsStringToView(objVec, 2);
    Focl_StringView bodyBlock = FoclObjVecAtAsStringToView(objVec, 3);
    Focl_Object* result = FoclObjectVoid(context->strObjPool, context->strPool);
    context->hasBreakBuf = true;
    context->hasContinueBuf = true;
    while (1)
    {
        Focl_Object* condResult = Focl_exprBool(context, &condExpr);
        if (condResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(result, context);
            context->hasBreakBuf = false;
            context->hasContinueBuf = false;
            return condResult;
        }
        if (condResult->as.i != FOCL_OBJ_TRUE)
        {
            FoclObjectRelease(condResult, context);
            break;
        }
        FoclObjectRelease(condResult, context);
        volatile int breakJmp = setjmp(context->breakBuf);
        if (breakJmp != 0)
        {
            context->hasBreakBuf = false;
            context->hasContinueBuf = false;
            return result;
        }
        Focl_Object* bodyResult = Focl_parseBlock(context, &bodyBlock);
        if (bodyResult->type == FOCL_OBJ_TYPE_ERROR)
        {
            FoclObjectRelease(result, context);
            context->hasBreakBuf = false;
            context->hasContinueBuf = false;
            return bodyResult;
        }
        FoclObjectRelease(result, context);
        result = bodyResult;
        {
            setjmp(context->continueBuf);
            Focl_Object* updateResult = Focl_parseBlock(context, &updateBlock);
            if (updateResult->type == FOCL_OBJ_TYPE_ERROR)
            {
                FoclObjectRelease(result, context);
                context->hasBreakBuf = false;
                context->hasContinueBuf = false;
                return updateResult;
            }
            FoclObjectRelease(updateResult, context);
        }
    }
    context->hasBreakBuf = false;
    context->hasContinueBuf = false;
    return result;
}
Focl_Object* buildIn_typename(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objName;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, objName, context->strObjPool, context->strPool);
    Focl_Object* obj;
    Focl_StringView objNameView = {.len = objName->as.data->length, .strPtr = objName->as.data->data};
    obj = FindObjectInContext(context, &objNameView);
    if (obj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    Focl_Object* retValue = FoclStringObjPoolAlloc(context->strObjPool, context->strPool, FOCL_OBJ_TYPE_STR);
    switch (obj->type)
    {
        case FOCL_OBJ_TYPE_INT:
            FoclStrAssign(retValue->as.data, "Integer");
            break;
        case FOCL_OBJ_TYPE_FLOAT:
            FoclStrAssign(retValue->as.data, "Float");
            break;
        case FOCL_OBJ_TYPE_BOOL:
            FoclStrAssign(retValue->as.data, "Boolean");
            break;
        case FOCL_OBJ_TYPE_VOID:
            FoclStrAssign(retValue->as.data, "Void");
            break;
        case FOCL_OBJ_TYPE_STR:
            FoclStrAssign(retValue->as.data, "String");
            break;
        case FOCL_OBJ_TYPE_ERROR:
            FoclStrAssign(retValue->as.data, "Error");
            break;
        case FOCL_OBJ_TYPE_BYTECODE:
            FoclStrAssign(retValue->as.data, "Focl ByteCode");
            break;
        case FOCL_OBJ_TYPE_COMPLEX:
            FoclStrAssign(retValue->as.data, "Complex");
            break;
        default:
            FoclStrAssign(retValue->as.data, FOCL_ERR_YSNBH);
            break;
    }
    
    return retValue;
}
Focl_Object* buildIn_typeid(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objName;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, objName, context->strObjPool, context->strPool);
    Focl_Object* obj;
    Focl_StringView objNameView = {.len = objName->as.data->length, .strPtr = objName->as.data->data};
    obj = FindObjectInContext(context, &objNameView);
    if (obj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    Focl_Object* idObj = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
    FoclObjectBoxInt(idObj, (Focl_Obj_Int)obj->type);
    return idObj;
}
Focl_Object* buildIn_eval(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_String* viewToEval = FoclObjVecAtAsString(objVec, 0);
    Focl_Object* obj = Focl_parseLine(context, viewToEval);
    return obj;
}
Focl_Object* buildIn_expr(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* arg = FoclObjVecAt(objVec, 0);
    if (!isFoclObjectUseString(arg))
    {
        return FoclObjectError(context->strObjPool, context->strPool, "expr requires a string argument");
    }
    Focl_StringView sv = {arg->as.data->length, arg->as.data->data};
    Focl_ExprParser parser;
    parser.context = context;
    parser.pos = sv.strPtr;
    parser.end = sv.strPtr + sv.len;
    Focl_Object* result = exprParseExpression(&parser);
    if (result != NULL && result->type != FOCL_OBJ_TYPE_ERROR)
    {
        exprSkipSpace(&parser);
        if (parser.pos < parser.end)
        {
            FoclObjectRelease(result, context);
            return FoclObjectError(context->strObjPool, context->strPool, "Unexpected characters after expression");
        }
    }
    return result;
}
Focl_Object* buildIn_curtime(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    Focl_Object* timeObj = FoclStringObjPoolAlloc(context->strObjPool, context->strPool, FOCL_OBJ_TYPE_STR);
    FoclStrAssign(timeObj->as.data, asctime(timeinfo));
    return timeObj;
}
Focl_Object* buildIn_break(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    if (context->hasBreakBuf)
    {
        for (size_t i = 0; i < argCount; i++)
        {
            FoclObjectRelease(FoclObjVecAt(objVec, i), context);
        }
        FoclVectorPoolFree(objVec, context->vecPool);
        longjmp(context->breakBuf, 1);
    }
    return FoclObjectError(context->strObjPool, context->strPool, "break outside loop");
}
Focl_Object* buildIn_continue(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    if (context->hasContinueBuf)
    {
        for (size_t i = 0; i < argCount; i++)
        {
            FoclObjectRelease(FoclObjVecAt(objVec, i), context);
        }
        FoclVectorPoolFree(objVec, context->vecPool);
        longjmp(context->continueBuf, 1);
    }
    return FoclObjectError(context->strObjPool, context->strPool, "continue outside loop");
}
Focl_Object* buildIn_exit(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount == 0)
    {
        context->exitCode = 0;
    }
    else if (argCount == 1)
    {
        Focl_Object* obj;
        FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 0, obj, context->strObjPool, context->strPool);
        context->exitCode = FoclObjectUnboxInt(obj);
    }
    else
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }

    if (context->hasExitBuf)
    {
        longjmp(context->exitBuf, 1);
    }
    return FoclObjectError(context->strObjPool, context->strPool, "exit outside repl");
}

Focl_Object* buildIn_asstring(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    /* The command will use the object as string compulsory */

    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* originalObjName;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, originalObjName, context->strObjPool, context->strPool);
    Focl_String* originalObjNameStr = FoclObjectGetString(originalObjName);
    Focl_StringView originalObjNameView = {.len = originalObjNameStr->length, .strPtr = originalObjNameStr->data};
    Focl_Object* originalObj = FindObjectInContext(context, &originalObjNameView);
    if (originalObj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    Focl_Object* obj = FoclObjPoolAllocAssign(context, originalObj);
    if (!isFoclObjectUseString(obj))
    {
        char intToStrBuffer[FOCL_INT_TO_STR_TMP_BUFFER_SIZE];
        char floatToStrBuffer[FOCL_INT_TO_STR_TMP_BUFFER_SIZE];
        switch (obj->type)
        {
            case FOCL_OBJ_TYPE_INT:
                Focl_Obj_Int i = FoclObjectUnboxInt(obj);
                obj->as.data = FoclStringPoolAlloc(context->strPool);
                sprintf(intToStrBuffer, "%"FOCL_FORMAT_INT, i);
                FoclStrAssign(obj->as.data, intToStrBuffer);
                break;
            case FOCL_OBJ_TYPE_FLOAT:
                Focl_Obj_Float f = FoclObjectUnboxFloat(obj);
                obj->as.data = FoclStringPoolAlloc(context->strPool);
                sprintf(floatToStrBuffer, "%"FOCL_FORMAT_FLOAT, f);
                FoclStrAssign(obj->as.data, floatToStrBuffer);
                break;
            case FOCL_OBJ_TYPE_BOOL:
                Focl_Obj_Bool b = obj->as.i;
                obj->as.data = FoclStringPoolAlloc(context->strPool);
                FoclStrAssign(obj->as.data, (b == FOCL_OBJ_TRUE) ? "true" : "false");
                break;
        }
        obj->type = FOCL_OBJ_TYPE_STR;
    }
    return obj;
}
Focl_Object* buildIn_asint(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    /* The command will use the object as string compulsory */

    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* originalObjName;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, originalObjName, context->strObjPool, context->strPool);
    Focl_String* originalObjNameStr = FoclObjectGetString(originalObjName);
    Focl_StringView originalObjNameView = {.len = originalObjNameStr->length, .strPtr = originalObjNameStr->data};
    Focl_Object* originalObj = FindObjectInContext(context, &originalObjNameView);
    if (originalObj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    if (Focl_isInteger(FoclStrCStr(FoclObjectGetString(originalObj))) == false)
    {
        return FoclObjectError(context->strObjPool, context->strPool, "String cannot be parse as a integer");
    }
    Focl_Object* obj = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
    FoclObjectBoxInt(obj, Focl_StrToInt(FoclStrCStr(FoclObjectGetString(originalObj))));
    return obj;
}
Focl_Object* buildIn_asfloat(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    /* The command will use the object as string compulsory */

    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* originalObjName;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, originalObjName, context->strObjPool, context->strPool);
    Focl_String* originalObjNameStr = FoclObjectGetString(originalObjName);
    Focl_StringView originalObjNameView = {.len = originalObjNameStr->length, .strPtr = originalObjNameStr->data};
    Focl_Object* originalObj = FindObjectInContext(context, &originalObjNameView);
    if (originalObj == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    if (Focl_isInteger(FoclStrCStr(FoclObjectGetString(originalObj))) == false)
    {
        return FoclObjectError(context->strObjPool, context->strPool, "String cannot be parse as a float");
    }
    Focl_Object* obj = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
    FoclObjectBoxFloat(obj, Focl_StrToInt(FoclStrCStr(FoclObjectGetString(originalObj))));
    return obj;
}
Focl_Object* buildIn_upvar(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView outerName = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_StringView localName = FoclObjVecAtAsStringToView(objVec, 1);
    Focl_Object* outerVar = FindObjectInEnvironment(context->curEnv->parent, &outerName);
    if (outerVar == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    FoclObjectRetain(outerVar);
    LinkObjectWithName_View(context, outerVar, &localName);
    return FoclObjectVoid(context->strObjPool, context->strPool);
}
Focl_Object* buildIn_proc(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 3)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView procName = FoclObjVecAtAsStringToView(objVec, 0);
    if (FindCommandInContext(context, &procName) != FOCL_COMMAND_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, "repeated command name");
    }
    Focl_StringView argList = FoclObjVecAtAsStringToView(objVec, 1);
    Focl_StringView execBlock = FoclObjVecAtAsStringToView(objVec, 2);
    Focl_String* _name = FoclStringPoolAlloc(context->strPool);
    FoclStrAssignView(_name, &procName);
    Focl_Command* _cmd = createFoclCommand(context->strPool, &argList, &execBlock);
    Focl_KeyOpDt keyOpDt = {.ctx = context->strPool, .func = FoclStringPoolFreeOpDtVoid};
    FoclHashTableInsert(context->curEnv->cmdTable, _name, _cmd, StrKeyCompare, &keyOpDt, NULL);
    return FoclObjectVoid(context->strObjPool, context->strPool);
}
Focl_Object* buildIn_return(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (!context->hasReturnBuf)
    {
        return FoclObjectError(context->strObjPool, context->strPool, "cannot return outside proc");
    }

    Focl_Object* retObj;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount == 0)
    {
        retObj = FoclObjectVoid(context->strObjPool, context->strPool);
    }
    else if (argCount == 1)
    {
        retObj = FoclObjVecAt(objVec, 0);
    }
    else
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }

    FoclObjectRetain(retObj);
    context->returnValue = retObj;

    for (size_t i = 0; i < argCount; i++)
    {
        FoclObjectRelease(FoclObjVecAt(objVec, i), context);
    }
    FoclVectorPoolFree(objVec, context->vecPool);
    longjmp(context->returnBuf, 1);
}
Focl_Object* buildIn_isBuildIn(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_StringView cmdName = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Command* cmd_ = FindCommandInContext(context, &cmdName);
    if (cmd_ == FOCL_COMMAND_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNKNOWN_COMMAND);
    }
    if (cmd_->func != Focl_evalProc)
    {
        return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_TRUE);
    }
    else
    {
        return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_FALSE);
    }
}
Focl_Object* buildIn_srand(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount > 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }

    if (argCount == 0)
    {
        srand(time(NULL));
    }
    else
    {
        Focl_Object* seed;
        FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 0, seed, context->strObjPool, context->strPool);
        srand((unsigned int)FoclObjectUnboxInt(seed));
    }
    return FoclObjectVoid(context->strObjPool, context->strPool);
}
Focl_Object* buildIn_randi(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* min;
    Focl_Object* max;
    FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 0, min, context->strObjPool, context->strPool);
    FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 1, max, context->strObjPool, context->strPool);
    Focl_Obj_Int minI = FoclObjectUnboxInt(min);
    Focl_Obj_Int maxI = FoclObjectUnboxInt(max);
    if (maxI < minI)
    {
        return FoclObjectError(context->strObjPool, context->strPool, "the max should not small that min");
    }
    Focl_Object* randomNumObj = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
    FoclObjectBoxInt(randomNumObj, rand() % (maxI - minI + 1) + minI);
    return randomNumObj;
}
Focl_Object* buildIn_randf(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount != 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* min;
    Focl_Object* max;
    FOCL_OBJ_VEC_AT_AS_FLOAT_OBJ(objVec, 0, min, context->strObjPool, context->strPool);
    FOCL_OBJ_VEC_AT_AS_FLOAT_OBJ(objVec, 1, max, context->strObjPool, context->strPool);
    Focl_Obj_Float minF = FoclObjectUnboxFloat(min);
    Focl_Obj_Float maxF = FoclObjectUnboxFloat(max);
    if (maxF < minF)
    {
        return FoclObjectError(context->strObjPool, context->strPool, "the max should not small that min");
    }
    Focl_Object* randomNumObj = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
    FoclObjectBoxFloat(randomNumObj, minF + (rand() / (RAND_MAX + 1.0)) * (maxF - minF));
    return randomNumObj;
}
Focl_Object* buildIn_string(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount < 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* childCmdObj;
    Focl_Object* targetObj;
    Focl_Object* retValue;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, childCmdObj, context->strObjPool, context->strPool);
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 1, targetObj, context->strObjPool, context->strPool);
    if (FoclStrComp(FoclObjectGetString(childCmdObj), "length") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
        FoclObjectBoxInt(retValue, FoclStrCharCount(FoclObjectGetString(targetObj)));
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "index") == 0)
    {
        if (argCount != 3)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }
        Focl_Object* idxObj;
        FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 2, idxObj, context->strObjPool, context->strPool);
        Focl_Obj_Int idx = FoclObjectUnboxInt(idxObj);
        if (idx < 0)
        {
            return FoclObjectError(context->strObjPool, context->strPool, "Index couldn't be negative");
        }
        size_t len = FoclStrCharCount(FoclObjectGetString(targetObj));
        if ((size_t)idx >= len)
        {
            return FoclObjectError(context->strObjPool, context->strPool, "index out of length");
        }
        retValue = FoclStringObjPoolAlloc(context->strObjPool, context->strPool, FOCL_OBJ_TYPE_STR);
        char tmpBuffer[5] = {0};
        char* start = targetObj->as.data->data;
        size_t searchStartIdx = 0;
        int32_t tmpChar = FoclStrAt(idx, &start, &searchStartIdx);
        memcpy(tmpBuffer, &tmpChar, sizeof(int32_t));
        FoclStrAssign(FoclObjectGetString(retValue), tmpBuffer);
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "range") == 0)
    {
        if (argCount != 4)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }
        Focl_Object* beginObj;
        Focl_Object* endObj;
        FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 2, beginObj, context->strObjPool, context->strPool);
        FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 3, endObj, context->strObjPool, context->strPool);
        Focl_Obj_Int beginIdx = FoclObjectUnboxInt(beginObj);
        Focl_Obj_Int endIdx = FoclObjectUnboxInt(endObj);
        size_t totalChars = FoclStrCharCount(FoclObjectGetString(targetObj));
        if (beginIdx < 0 || endIdx < 0 || beginIdx >= (Focl_Obj_Int)totalChars || endIdx >= (Focl_Obj_Int)totalChars)
        {
            return FoclObjectError(context->strObjPool, context->strPool, "Index out of range");
        }
        if (beginIdx > endIdx)
        {
            return FoclObjectError(context->strObjPool, context->strPool, "Begin must not be greater than end");
        }
        size_t charIdx = 0;
        char* startPtr = FoclObjectGetString(targetObj)->data;
        while (charIdx < (size_t)beginIdx)
        {
            startPtr += getUtf8CodePointLength((uint8_t)*startPtr);
            charIdx++;
        }
        char* endPtr = startPtr;
        while (charIdx <= (size_t)endIdx)
        {
            endPtr += getUtf8CodePointLength((uint8_t)*endPtr);
            charIdx++;
        }
        ptrdiff_t byteLen = endPtr - startPtr;
        char* tmpBuf = (char*)malloc(byteLen + 1);
        memcpy(tmpBuf, startPtr, byteLen);
        tmpBuf[byteLen] = '\0';
        retValue = FoclStringObjPoolAlloc(context->strObjPool, context->strPool, FOCL_OBJ_TYPE_STR);
        FoclStrAssign(FoclObjectGetString(retValue), tmpBuf);
        free(tmpBuf);
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "compare") == 0)
    {
        if (argCount != 3)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }
        Focl_Object* srcObj;
        FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 2, srcObj, context->strObjPool, context->strPool);
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
        FoclObjectBoxInt(retValue, (Focl_Obj_Int)FoclStrCompStr(FoclObjectGetString(targetObj), FoclObjectGetString(srcObj)));
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "equal") == 0)
    {
        if (argCount != 3)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }
        Focl_Object* srcObj;
        FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 2, srcObj, context->strObjPool, context->strPool);
        if (FoclStrCompStr(FoclObjectGetString(targetObj), FoclObjectGetString(srcObj)) == 0)
        {
            retValue = FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_TRUE);
        }
        else
        {
            retValue = FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_FALSE);
        }
    }
    else
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNKNOWN_ARG);
    }
    return retValue;
}
Focl_Object* buildIn_append(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    Focl_Object* srcObj;
    Focl_StringView dstName = FoclObjVecAtAsStringToView(objVec, 0);
    Focl_Object* dst = FindObjectInContext(context, &dstName);
    if (dst == FOCL_OBJECT_ERROR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_OBJECT);
    }
    if (dst->type != FOCL_OBJ_TYPE_STR)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_WRONG_TYPE_ASSIGNMENT);
    }
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 1, srcObj, context->strObjPool, context->strPool);
    FoclStrAppendStr(FoclObjectGetString(dst), FoclObjectGetString(srcObj));
    FoclObjectRetain(dst);
    return dst;
}
Focl_Object* buildIn_file(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t argCount = FoclVectorGetSize(objVec);
    if (argCount < 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* childCmdObj;
    Focl_Object* targetObj;
    Focl_Object* retValue;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, childCmdObj, context->strObjPool, context->strPool);
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 1, targetObj, context->strObjPool, context->strPool);
    if (FoclStrComp(FoclObjectGetString(childCmdObj), "exists") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }
        if (Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_TRUE);
        }
        else
        {
            return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_FALSE);
        }
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "isfile") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }

        if (Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, "file doesn't exist");
        }
        if (Focl_isNormalFile(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_TRUE);
        }
        else
        {
            return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_FALSE);
        }
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "isdirectory") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }

        if (Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, "file doesn't exist");
        }
        if (Focl_isDir(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_TRUE);
        }
        else
        {
            return FoclObjectBool(context->objWithNoStrPool, FOCL_OBJ_FALSE);
        }
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "size") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }

        if (Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, "file doesn't exist");
        }
        if (Focl_isNormalFile(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, "The command \"file size\" should be used on a normal file");
        }
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
        FoclObjectBoxInt(retValue, Focl_GetFileSize(FoclStrCStr(FoclObjectGetString(targetObj))));
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "mkdir") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }
        int retMkdir = Focl_mkdir(FoclStrCStr(FoclObjectGetString(targetObj)));
        if (retMkdir != 0)
        {
            return FoclObjectError(context->strObjPool, context->strPool, "Cannot mkdir");
        }
        retValue = FoclObjectVoid(context->strObjPool, context->strPool);
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "dirname") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }

        if (Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, "file doesn't exist");
        }
        char* dirCStr = Focl_dirname(FoclStrCStr(FoclObjectGetString(targetObj)));
        retValue = FoclStringObjPoolAlloc(context->strObjPool, context->strPool, FOCL_OBJ_TYPE_STR);
        FoclStrAssign(FoclObjectGetString(retValue), dirCStr);
        free(dirCStr);
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "realpath") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }

        if (Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, "file or path doesn't exist");
        }
        char buffer[PATH_MAX];
        Focl_realpath(FoclStrCStr(FoclObjectGetString(targetObj)), buffer, PATH_MAX);
        retValue = FoclStringObjPoolAlloc(context->strObjPool, context->strPool, FOCL_OBJ_TYPE_STR);
        FoclStrAssign(FoclObjectGetString(retValue), buffer);
    }
    else
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNKNOWN_ARG);
    }
    return retValue;
}
Focl_Object* buildIn_exec(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* sysCmdObj;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, sysCmdObj, context->strObjPool, context->strPool);
    system(FoclStrCStr(FoclObjectGetString(sysCmdObj)));
    return FoclObjectVoid(context->strObjPool, context->strPool);
}

void Focl_RegisterBuiltinCommands(Focl_Context* context)
{
    FoclRegisterCommand(context, "puts", buildIn_puts);
    FoclRegisterCommand(context, "gets", buildIn_gets);
    FoclRegisterCommand(context, "set", buildIn_set);
    FoclRegisterCommand(context, "unset", buildIn_unset);
    FoclRegisterCommand(context, "incr", buildIn_incr);
    FoclRegisterCommand(context, "if", buildIn_if);
    FoclRegisterCommand(context, "while", buildIn_while);
    FoclRegisterCommand(context, "for", buildIn_for);
    FoclRegisterCommand(context, "typename", buildIn_typename);
    FoclRegisterCommand(context, "typeid", buildIn_typeid);
    FoclRegisterCommand(context, "eval", buildIn_eval);
    FoclRegisterCommand(context, "expr", buildIn_expr);
    FoclRegisterCommand(context, "curtime", buildIn_curtime);
    FoclRegisterCommand(context, "break", buildIn_break);
    FoclRegisterCommand(context, "continue", buildIn_continue);
    FoclRegisterCommand(context, "asstring", buildIn_asstring);
    FoclRegisterCommand(context, "asint", buildIn_asint);
    FoclRegisterCommand(context, "asfloat", buildIn_asfloat);
    FoclRegisterCommand(context, "exit", buildIn_exit);
    FoclRegisterCommand(context, "upvar", buildIn_upvar);
    FoclRegisterCommand(context, "proc", buildIn_proc);
    FoclRegisterCommand(context, "return", buildIn_return);
    FoclRegisterCommand(context, "isbuildin", buildIn_isBuildIn);
    FoclRegisterCommand(context, "srand", buildIn_srand);
    FoclRegisterCommand(context, "randi", buildIn_randi);
    FoclRegisterCommand(context, "randf", buildIn_randf);
    FoclRegisterCommand(context, "string", buildIn_string);
    FoclRegisterCommand(context, "append", buildIn_append);
    FoclRegisterCommand(context, "file", buildIn_file);
    FoclRegisterCommand(context, "exec", buildIn_exec);
}