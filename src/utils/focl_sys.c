#include <stdlib.h>
#include <string.h>
#include "focl_dev.h"
#include "sys_lean.h"

#define FOCL_SYSERR_CANNOT_FIND_FILE "cannot find file."
#define FOCL_SYSERR_CANNOT_ACCESS_FILE "cannot access file."
#define FOCL_SYSERR_IS_A_DIR "cannot execute the control to a directory without \"-r\"."
#define FOCL_SYSERR_NOT_A_DIR "not a valid directory."
#define FOCL_SYSERR_UNKNOWN_CTLPMT "unknown control prompt."

#define FOCL_SYS_CAT_BUFFER_SIZE 1024

#define FOCL_EDITOR_VAR_NAME "_FOCL_SYS_USING_EDITOR"

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
        Focl_free(realSrcPath);
        Focl_free(realDstPath);
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
            Focl_free(realSrcPath);
            Focl_free(realDstPath);
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
        Focl_free(realSrcPath);
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
            Focl_free(realSrcPath);
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
Focl_Object* sys_mv(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* retValue;
    Focl_Object* srcpathObj;
    Focl_Object* dstpathObj;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, srcpathObj, context->strObjPool, context->strPool);
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 1, dstpathObj, context->strObjPool, context->strPool);
    char* realSrcPath = Focl_normalizePath(FOCL_STROBJ_CSTR(srcpathObj));
    char* realDstPath = Focl_normalizePath(FOCL_STROBJ_CSTR(dstpathObj));
    if (Focl_isFileExist(realDstPath) && Focl_isDir(realDstPath))
    {
        char* filename = Focl_GetPathLastName(realSrcPath);
        size_t lenOfFileName = strlen(filename);
        size_t lenOfDstPath = strlen(realDstPath);
        char* newPath = Focl_realloc(realDstPath, lenOfFileName + lenOfDstPath + 2);
        realDstPath = newPath;
        realDstPath[lenOfDstPath] = '/';
        memcpy(realDstPath + lenOfDstPath + 1, filename, lenOfFileName + 1);
        Focl_free(filename);
    }
    else if (Focl_isFileExist(realDstPath))
    {
        FOCL_ERROBJ_ALLOC(retValue, context, "already had file with the same name.");
        Focl_free(realSrcPath);
        Focl_free(realDstPath);
        return retValue;
    }
    
    if (rename(realSrcPath, realDstPath) == 0)
    {
        retValue = FoclObjectVoid(context->strObjPool, context->strPool);
    }
    else
    {
        FOCL_ERROBJ_ALLOC(retValue, context, "Cannot move or rename file or directory.");
    }
    
    Focl_free(realSrcPath);
    Focl_free(realDstPath);
    return retValue;
}
Focl_Object* sys_cat(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    /* I'm sorry that we have a bad implementation. :( */
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* retValue;
    Focl_Object* srcpathObj;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, srcpathObj, context->strObjPool, context->strPool);
    char* realSrcPath = Focl_normalizePath(FOCL_STROBJ_CSTR(srcpathObj));
    if (Focl_isFileExist(realSrcPath))
    {
        if (!Focl_isDir(realSrcPath))
        {
            FILE* fp = fopen(realSrcPath, "rb");
            if (fp == NULL)
            {
                FOCL_ERROBJ_ALLOC(retValue, context, FOCL_SYSERR_CANNOT_ACCESS_FILE);
            }
            else
            {
                ptrdiff_t fileSize = Focl_GetFileSize(realSrcPath);
                if (fileSize == -1)
                {
                    fclose(fp);
                    FOCL_ERROBJ_ALLOC(retValue, context, "cannot get file size.");
                }
                else
                {
                    char* buffer = Focl_malloc(fileSize + 1);
                    buffer[fileSize] = '\0';
                    (void)fread(buffer, sizeof(char), fileSize, fp);
                    retValue = FoclStringObjPoolAlloc(context->strObjPool, context->strPool, FOCL_OBJ_TYPE_STR);
                    FoclStrAssign(FoclObjectGetString(retValue), buffer);
                    Focl_free(buffer);
                    fclose(fp);
                }
            }
        }
        else
        {
            FOCL_ERROBJ_ALLOC(retValue, context, FOCL_SYSERR_IS_A_DIR);
        }
    }
    else
    {
        FOCL_ERROBJ_ALLOC(retValue, context, FOCL_SYSERR_CANNOT_FIND_FILE);
    }
    Focl_free(realSrcPath);
    return retValue;
}
Focl_Object* sys_edit(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    /* to edit file, you need to have to set a value call FOCL_USING_EDITOR, which attach under the root(global) env. */
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_String* editorName = FoclStringPoolAlloc(context->strPool);
    FoclStrAssign(editorName, FOCL_EDITOR_VAR_NAME);
    Focl_Object* obj = Focl_FindObject(context->globalEnv, context->strPool, editorName);
    if (obj == FOCL_OBJECT_ERROR)
    {
        FoclStringPoolFree(editorName, context->strPool);
        return FoclObjectError(context->strObjPool, context->strPool, "Cannot find editor, please using \" set "FOCL_EDITOR_VAR_NAME" [Editor Name] first.");
    }
    FoclStrAssignStr(editorName, FoclObjectGetString(obj));
    Focl_Object* fileNameObj;
    FOCL_OBJ_VEC_AT_AS_STRING_OBJ(objVec, 0, fileNameObj, context->strObjPool, context->strPool);
    if (!Focl_isNormalFile(FOCL_STROBJ_CSTR(fileNameObj)))
    {
        FoclStringPoolFree(editorName, context->strPool);
        return FoclObjectError(context->strObjPool, context->strPool, "target isn't a normal file.");
    }
    char* argv[] = {FoclStrCStr(editorName), FOCL_STROBJ_CSTR(fileNameObj), NULL};
    Focl_execAndWait(FoclStrCStr(editorName), argv);
    FoclStringPoolFree(editorName, context->strPool);
    return FoclObjectVoid(context->strObjPool, context->strPool);
}


void Focl_RegisterSystemCommand(Focl_Context* context)
{
    FoclRegisterCommand(context, "sys::file", sys_file);
    FoclRegisterCommand(context, "sys::exec", sys_exec);
    FoclRegisterCommand(context, "sys::name", sys_name);
    FoclRegisterCommand(context, "sys::cp", sys_cp);
    FoclRegisterCommand(context, "sys::rm", sys_rm);
    FoclRegisterCommand(context, "sys::mv", sys_mv);
    FoclRegisterCommand(context, "sys::cat", sys_cat);
    FoclRegisterCommand(context, "sys::edit", sys_edit);
}