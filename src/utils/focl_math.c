#include "focl_dev.h"
#include <math.h>

#ifndef M_PI
#define FOCL_MATH_PI 3.14159265358979323846
#else
#define FOCL_MATH_PI M_PI
#endif

#define FOCL_OBJ_VEC_AT_AS_INT_OR_FLOAT(objVec, idx, obj, context, retValue, mathfunc) \
    obj = FoclObjVecAt(objVec, idx); \
    if (obj->type == FOCL_OBJ_TYPE_INT) \
    { \
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT); \
        FoclObjectBoxFloat(retValue, mathfunc((Focl_Obj_Float)FoclObjectUnboxInt(obj))); \
    } \
    else if (obj->type == FOCL_OBJ_TYPE_FLOAT) \
    { \
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT); \
        FoclObjectBoxFloat(retValue, mathfunc(FoclObjectUnboxFloat(obj))); \
    } \
    else \
    { \
        retValue = FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_INVALID_ARG); \
    } \

#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF && !defined(USE_F32_ON_64)
    Focl_Obj_Float Focl_Math_Sin(Focl_Obj_Float x)
    {
        return sin(x);
    }
    Focl_Obj_Float Focl_Math_Cos(Focl_Obj_Float x)
    {
        return cos(x);
    }
    Focl_Obj_Float Focl_Math_Tan(Focl_Obj_Float x)
    {
        return tan(x);
    }
    Focl_Obj_Float Focl_Math_Log(Focl_Obj_Float x)
    {
        return log(x);
    }
    Focl_Obj_Float Focl_Math_Log10(Focl_Obj_Float x)
    {
        return log10(x);
    }
    Focl_Obj_Float Focl_Math_Sqrt(Focl_Obj_Float x)
    {
        return sqrt(x);
    }
    Focl_Obj_Float Focl_Math_Abs(Focl_Obj_Float x)
    {
        return fabs(x);
    }
    Focl_Obj_Float Focl_Math_Exp(Focl_Obj_Float x)
    {
        return exp(x);
    }
#elif SIZE_MAX == 0xFFFFFFFF || defined(USE_F32_ON_64)
    Focl_Obj_Float Focl_Math_Sin(Focl_Obj_Float x)
    {
        return sinf(x);
    }
    Focl_Obj_Float Focl_Math_Cos(Focl_Obj_Float x)
    {
        return cosf(x);
    }
    Focl_Obj_Float Focl_Math_Tan(Focl_Obj_Float x)
    {
        return tanf(x);
    }
    Focl_Obj_Float Focl_Math_Log(Focl_Obj_Float x)
    {
        return logf(x);
    }
    Focl_Obj_Float Focl_Math_Log10(Focl_Obj_Float x)
    {
        return log10f(x);
    }
    Focl_Obj_Float Focl_Math_Sqrt(Focl_Obj_Float x)
    {
        return sqrtf(x);
    }
    Focl_Obj_Float Focl_Math_Abs(Focl_Obj_Float x)
    {
        return fabsf(x);
    }
    Focl_Obj_Float Focl_Math_Exp(Focl_Obj_Float x)
    {
        return expf(x);
    }
#else
    #error "Unsupported word length platform. Though I want to see this program run in every platform. But now it couldn't run yours. Sorry. :("
#endif

Focl_Obj_Float Focl_Math_DegreeToRadians(Focl_Obj_Float deg)
{
    return deg * (FOCL_MATH_PI / 180.0);
}
Focl_Obj_Float Focl_Math_RadiansToDegree(Focl_Obj_Float rad)
{
    return rad * (180.0 / FOCL_MATH_PI);
}

Focl_Object* math_sin(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum = FoclObjVecAt(objVec, 0);
    Focl_Object* retValue;
    if (objNum->type == FOCL_OBJ_TYPE_INT)
    {
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(retValue, Focl_Math_Sin((Focl_Obj_Float)FoclObjectUnboxInt(objNum)));
    }
    else if (objNum->type == FOCL_OBJ_TYPE_FLOAT)
    {
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(retValue, Focl_Math_Sin(FoclObjectUnboxFloat(objNum)));
    }
    else
    {
        retValue = FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_INVALID_ARG);
    }
    return retValue;
}
Focl_Object* math_cos(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum = FoclObjVecAt(objVec, 0);
    Focl_Object* retValue;
    if (objNum->type == FOCL_OBJ_TYPE_INT)
    {
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(retValue, Focl_Math_Cos((Focl_Obj_Float)FoclObjectUnboxInt(objNum)));
    }
    else if (objNum->type == FOCL_OBJ_TYPE_FLOAT)
    {
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(retValue, Focl_Math_Cos(FoclObjectUnboxFloat(objNum)));
    }
    else
    {
        retValue = FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_INVALID_ARG);
    }
    return retValue;
}
Focl_Object* math_tan(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum = FoclObjVecAt(objVec, 0);
    Focl_Object* retValue;
    if (objNum->type == FOCL_OBJ_TYPE_INT)
    {
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(retValue, Focl_Math_Tan((Focl_Obj_Float)FoclObjectUnboxInt(objNum)));
    }
    else if (objNum->type == FOCL_OBJ_TYPE_FLOAT)
    {
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(retValue, Focl_Math_Tan(FoclObjectUnboxFloat(objNum)));
    }
    else
    {
        retValue = FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_INVALID_ARG);
    }
    return retValue;
}
Focl_Object* math_degtorad(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum = FoclObjVecAt(objVec, 0);
    Focl_Object* retValue;
    if (objNum->type == FOCL_OBJ_TYPE_INT)
    {
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(retValue, Focl_Math_DegreeToRadians((Focl_Obj_Float)FoclObjectUnboxInt(objNum)));
    }
    else if (objNum->type == FOCL_OBJ_TYPE_FLOAT)
    {
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(retValue, Focl_Math_DegreeToRadians(FoclObjectUnboxFloat(objNum)));
    }
    else
    {
        retValue = FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_INVALID_ARG);
    }
    return retValue;
}
Focl_Object* math_radtodeg(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum = FoclObjVecAt(objVec, 0);
    Focl_Object* retValue;
    if (objNum->type == FOCL_OBJ_TYPE_INT)
    {
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(retValue, Focl_Math_RadiansToDegree((Focl_Obj_Float)FoclObjectUnboxInt(objNum)));
    }
    else if (objNum->type == FOCL_OBJ_TYPE_FLOAT)
    {
        retValue = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
        FoclObjectBoxFloat(retValue, Focl_Math_RadiansToDegree(FoclObjectUnboxFloat(objNum)));
    }
    else
    {
        retValue = FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_INVALID_ARG);
    }
    return retValue;
}
Focl_Object* math_inttofloat(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum;
    FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 0, objNum, context->strObjPool, context->strPool);
    Focl_Object* fNum = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_FLOAT);
    FoclObjectBoxFloat(fNum, (Focl_Obj_Float)FoclObjectUnboxInt(objNum));
    return fNum;
}
Focl_Object* math_floattoint(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum;
    FOCL_OBJ_VEC_AT_AS_FLOAT_OBJ(objVec, 0, objNum, context->strObjPool, context->strPool);
    Focl_Object* iNum = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
    FoclObjectBoxInt(iNum, (Focl_Obj_Int)FoclObjectUnboxFloat(objNum));
    return iNum;
}
Focl_Object* math_log(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum;
    Focl_Object* retValue;
    FOCL_OBJ_VEC_AT_AS_INT_OR_FLOAT(objVec, 0, objNum, context, retValue, Focl_Math_Log);
    return retValue;
}
Focl_Object* math_log10(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum;
    Focl_Object* retValue;
    FOCL_OBJ_VEC_AT_AS_INT_OR_FLOAT(objVec, 0, objNum, context, retValue, Focl_Math_Log10);
    return retValue;
}
Focl_Object* math_sqrt(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum;
    Focl_Object* retValue;
    FOCL_OBJ_VEC_AT_AS_INT_OR_FLOAT(objVec, 0, objNum, context, retValue, Focl_Math_Sqrt);
    return retValue;
}
Focl_Object* math_abs(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum;
    Focl_Object* retValue;
    FOCL_OBJ_VEC_AT_AS_INT_OR_FLOAT(objVec, 0, objNum, context, retValue, Focl_Math_Abs);
    return retValue;
}
Focl_Object* math_exp(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* objNum;
    Focl_Object* retValue;
    FOCL_OBJ_VEC_AT_AS_INT_OR_FLOAT(objVec, 0, objNum, context, retValue, Focl_Math_Exp);
    return retValue;
}
void Focl_RegisterMathCommand(Focl_Context* context)
{
    FoclRegisterCommand(context, "math::sin", math_sin);
    FoclRegisterCommand(context, "math::cos", math_cos);
    FoclRegisterCommand(context, "math::tan", math_tan);
    FoclRegisterCommand(context, "math::degtorad", math_degtorad);
    FoclRegisterCommand(context, "math::radtodeg", math_radtodeg);
    FoclRegisterCommand(context, "math::inttofloat", math_inttofloat);
    FoclRegisterCommand(context, "math::floattoint", math_floattoint);
    FoclRegisterCommand(context, "math::log", math_log);
    FoclRegisterCommand(context, "math::log10", math_log10);
    FoclRegisterCommand(context, "math::sqrt", math_sqrt);
    FoclRegisterCommand(context, "math::abs", math_abs);
    FoclRegisterCommand(context, "math::exp", math_exp);
}