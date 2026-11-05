#include <stdlib.h>
#include <string.h>
#include "focl_dev.h"
#include "sys_lean.h"

Focl_Result sys_name(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 0)
    {
        return FoclResultError(FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT));
    }
    Focl_Object* sysName = FoclStringObjPoolAlloc(context->strObjPool, context->strPool, FOCL_OBJ_TYPE_STR);
#ifdef _WIN32
    FoclStrAssign(FoclObjectGetString(sysName), "Windows", sizeof("Windows") - 1);
#elif __linux__
    FoclStrAssign(FoclObjectGetString(sysName), "Linux", sizeof("Linux") - 1);
#elif __APPLE__
    FoclStrAssign(FoclObjectGetString(sysName), "Mac OS", sizeof("Mac OS") - 1);
#elif __FreeBSD__
    FoclStrAssign(FoclObjectGetString(sysName), "FreeBSD", sizeof("FreeBSD") - 1);
#elif __ANDROID__
    FoclStrAssign(FoclObjectGetString(sysName), "Android", sizeof("Android") - 1);
#else
#ifdef FOCL_CUSTOM_AIM_SYSTEM_NAME
    FoclStrAssign(FoclObjectGetString(sysName), FOCL_CUSTOM_AIM_SYSTEM_NAME, sizeof(FOCL_CUSTOM_AIM_SYSTEM_NAME) - 1);
#else
    FoclStrAssign(FoclObjectGetString(sysName), "Unknown", sizeof("Unknown") - 1);
#endif
#endif
    return FoclResultOk(sysName);
}
Focl_Result sys_sleepms(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclResultError(FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT));
    }
    Focl_Object* msObj;
    FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 0, msObj, context->strObjPool, context->strPool);
    Focl_Obj_Int ms = FoclObjectUnboxInt(msObj);
    if (ms < 0)
    {
        return FoclResultError(FoclObjectError(context->strObjPool, context->strPool, "Sleep time should not be negative."));
    }
    Focl_sleepms(ms);
    return FoclResultOk(FoclObjectVoid(context->flatObjPool));
}

void Focl_RegisterSystemCommand(Focl_Context* context)
{
    FoclRegisterCommand(context, "sys::name", sys_name);
    FoclRegisterCommand(context, "sys::sleepms", sys_sleepms);
}
