#include "focl_dev.h"
#include "sys_lean.h"
#include <string.h>

#define FOCL_ERR_CANNOT_FIND_FILE "Cannot find file."
#define FOCL_ERR_CANNOT_ACCESS_FILE "Cannot access file."
#define FOCL_ERR_IS_A_DIR "Cannot execute the control to a directory without \"-r\"."
#define FOCL_ERR_NOT_A_DIR "not a valid directory."
#define FOCL_ERR_UNKNOWN_CTLPMT "Unknown control prompt."

Focl_Object* file_file(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
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
            return FoclObjectBool(context->flatObjPool, FOCL_OBJ_TRUE);
        }
        else
        {
            return FoclObjectBool(context->flatObjPool, FOCL_OBJ_FALSE);
        }
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "isfile") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }

        if (!Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_FILE);
        }
        if (Focl_isNormalFile(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectBool(context->flatObjPool, FOCL_OBJ_TRUE);
        }
        else
        {
            return FoclObjectBool(context->flatObjPool, FOCL_OBJ_FALSE);
        }
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "isdirectory") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }

        if (!Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_FILE);
        }
        if (Focl_isDir(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectBool(context->flatObjPool, FOCL_OBJ_TRUE);
        }
        else
        {
            return FoclObjectBool(context->flatObjPool, FOCL_OBJ_FALSE);
        }
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "size") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }

        if (!Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_FILE);
        }
        if (Focl_isNormalFile(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, "The command \"file size\" should be used on a normal file");
        }
        retValue = FoclFlatObjPoolAlloc(context->flatObjPool, FOCL_OBJ_TYPE_INT);
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
        retValue = FoclObjectVoid(context->flatObjPool);
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
        Focl_free(dirCStr);
    }
    else if (FoclStrComp(FoclObjectGetString(childCmdObj), "realpath") == 0)
    {
        if (argCount != 2)
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
        }

        if (Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
        {
            return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_FILE);
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
Focl_Object* file_open(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* originalPathObj;
    Focl_String* originalPath;
    FOCL_OBJ_VEC_AT_AS_STRING(objVec, 0, originalPathObj, originalPath, context->strObjPool, context->strPool);
    Focl_String* realPath = FoclStringPoolAlloc(context->strPool);
    FoclStrReserve(realPath, PATH_MAX + 1);
    Focl_realpath(FoclStrCStr(originalPath), FoclStrCStr(realPath), PATH_MAX + 1);
    realPath->length = strlen(realPath->data);

    Focl_Object* modeObj;
    Focl_String* mode;
    FOCL_OBJ_VEC_AT_AS_STRING(objVec, 1, modeObj, mode, context->strObjPool, context->strPool);
    Focl_Object* fObj = FoclFileObjAlloc(context->flatObjPool, FoclStrCStr(realPath), FoclStrCStr(mode));
    if (fObj == FOCL_OBJECT_ERROR)
    {
        FoclStringPoolFree(realPath, context->strPool);
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_CANNOT_FIND_FILE);
    }

    FoclStringPoolFree(realPath, context->strPool);

    return fObj;
}
Focl_Object* file_close(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* fObj;
    FOCL_OBJ_VEC_AT_AS_OBJ(objVec, 0, fObj, FOCL_OBJ_TYPE_FILE, context->strObjPool, context->strPool);
    FoclObjectRelease(fObj, context);
    return FoclObjectVoid(context->flatObjPool);
}

void Focl_RegisterFileCommand(Focl_Context* context)
{
    FoclRegisterCommand(context, "file", file_file);
    FoclRegisterCommand(context, "open", file_open);
    FoclRegisterCommand(context, "close", file_close);
}