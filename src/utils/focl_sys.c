#include <stdlib.h>
#include "focl_dev.h"
#include "sys_lean.h"

#define FOCL_SYSERR_IS_A_DIR "cannot execute the control to a directory without \"-r\"."
#define FOCL_SYSERR_NOT_A_DIR "not a valid directory."
#define FOCL_SYSERR_UNKNOWN_CTLPMT "unknown control prompt."

Focl_Object* sys_file(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
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
        if (!Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
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

        if (!Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
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

        if (!Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
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

        if (!Focl_isFileExist(FoclStrCStr(FoclObjectGetString(targetObj))))
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
Focl_Object* sys_exec(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
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
Focl_Object* sys_name(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* sysName = FoclStringObjPoolAlloc(context->strObjPool, context->strPool, FOCL_OBJ_TYPE_STR);
#ifdef _WIN32
    FoclStrAssign(FoclObjectGetString(sysName), "Windows");
#elif __linux__
    FoclStrAssign(FoclObjectGetString(sysName), "Linux");
#elif __APPLE__
    FoclStrAssign(FoclObjectGetString(sysName), "Mac OS");
#elif __FreeBSD__
    FoclStrAssign(FoclObjectGetString(sysName), "FreeBSD");
#elif __ANDROID__
    FoclStrAssign(FoclObjectGetString(sysName), "Android");
#else
#ifdef FOCL_CUSTOM_AIM_SYSTEM_NAME
    FoclStrAssign(FoclObjectGetString(sysName), FOCL_CUSTOM_AIM_SYSTEM_NAME);
#else
    FoclStrAssign(FoclObjectGetString(sysName), "Unknown");
#endif
#endif
    return sysName;
}
Focl_Object* sys_cp(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t objVecSize = FoclVectorGetSize(objVec);
    Focl_Object* retValue;
    Focl_Object* srcpathObj;
    Focl_Object* dstpathObj;
    if (objVecSize == 2)
    {
        FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, srcpathObj, context->strObjPool, context->strPool);
        FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 1, dstpathObj, context->strObjPool, context->strPool);
        char* realSrcPath = Focl_normalizePath(FOCL_STROBJ_CSTR(srcpathObj));
        char* realDstPath = Focl_normalizePath(FOCL_STROBJ_CSTR(dstpathObj));
        if (Focl_isNormalFile(realSrcPath))
        {
            if (Focl_isDir(realDstPath))
            {
                int code = Focl_fileCopy(realSrcPath, realDstPath);
                if (code == -1)
                {
                    FOCL_ERROBJ_ALLOC(retValue, context, "Cannnot copy file.");
                }
                else
                {
                    retValue = FoclObjectVoid(context->strObjPool, context->strPool);
                }
            }
            else
            {
                FOCL_ERROBJ_ALLOC(retValue, context, FOCL_SYSERR_NOT_A_DIR);
            }
        }
        else
        {
            FOCL_ERROBJ_ALLOC(retValue, context, FOCL_SYSERR_IS_A_DIR);
        }
        free(realSrcPath);
        free(realDstPath);
    }
    else if (objVecSize == 3)
    {
        Focl_Object* ctlPrompt;
        FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, ctlPrompt, context->strObjPool, context->strPool);
        FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 1, srcpathObj, context->strObjPool, context->strPool);
        FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 2, dstpathObj, context->strObjPool, context->strPool);
        if (FoclStrComp(FoclObjectGetString(ctlPrompt), "-r") == 0)
        {
            char* realSrcPath = Focl_normalizePath(FOCL_STROBJ_CSTR(srcpathObj));
            char* realDstPath = Focl_normalizePath(FOCL_STROBJ_CSTR(dstpathObj));
            int code = Focl_copy(realSrcPath, realDstPath);
            if (code == -1)
            {
                FOCL_ERROBJ_ALLOC(retValue, context, "Cannnot copy file.");
            }
            retValue = FoclObjectVoid(context->strObjPool, context->strPool);
            free(realSrcPath);
            free(realDstPath);
        }
        else
        {
            FOCL_ERROBJ_ALLOC(retValue, context, FOCL_SYSERR_UNKNOWN_CTLPMT);
        }
    }
    else
    {
        FOCL_ERROBJ_ALLOC(retValue, context, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    return retValue;
}
Focl_Object* sys_rm(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    size_t objVecSize = FoclVectorGetSize(objVec);
    Focl_Object* retValue;
    Focl_Object* srcpathObj;
    if (objVecSize == 1)
    {
        FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, srcpathObj, context->strObjPool, context->strPool);
        char* realSrcPath = Focl_normalizePath(FOCL_STROBJ_CSTR(srcpathObj));
        if (Focl_isNormalFile(realSrcPath))
        {
            int code = remove(realSrcPath);
            if (code == -1)
            {
                FOCL_ERROBJ_ALLOC(retValue, context, "Cannnot remove file.");
            }
            else
            {
                retValue = FoclObjectVoid(context->strObjPool, context->strPool);
            }
        }
        else
        {
            FOCL_ERROBJ_ALLOC(retValue, context, FOCL_SYSERR_IS_A_DIR);
        }
        free(realSrcPath);
    }
    else if (objVecSize == 2)
    {
        Focl_Object* ctlPrompt;
        FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, ctlPrompt, context->strObjPool, context->strPool);
        FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 1, srcpathObj, context->strObjPool, context->strPool);
        if (FoclStrComp(FoclObjectGetString(ctlPrompt), "-r") == 0)
        {
            char* realSrcPath = Focl_normalizePath(FOCL_STROBJ_CSTR(srcpathObj));
            int code = Focl_remove(realSrcPath);
            if (code == -1)
            {
                FOCL_ERROBJ_ALLOC(retValue, context, "Cannnot remove file.");
            }
            else
            {
                retValue = FoclObjectVoid(context->strObjPool, context->strPool);
            }
            free(realSrcPath);
        }
        else
        {
            FOCL_ERROBJ_ALLOC(retValue, context, FOCL_SYSERR_UNKNOWN_CTLPMT);
        }
    }
    else
    {
        FOCL_ERROBJ_ALLOC(retValue, context, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    return retValue;
}

void Focl_RegisterSystemCommand(Focl_Context* context)
{
    FoclRegisterCommand(context, "sys::file", sys_file);
    FoclRegisterCommand(context, "sys::exec", sys_exec);
    FoclRegisterCommand(context, "sys::name", sys_name);
    FoclRegisterCommand(context, "sys::cp", sys_cp);
    FoclRegisterCommand(context, "sys::rm", sys_rm);
}