#include "focl_dev.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#define CLEAR_SCREEN "\033[2J\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"

Focl_Object* term_clear(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    FoclIOBufferPrintf(context->outBuffer, CLEAR_SCREEN);
    FoclIOBufferFlushOut(context->outBuffer);
    return FoclObjectVoid(context->strObjPool, context->strPool);
}
Focl_Object* term_gotoxy(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 2)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* xObj;
    Focl_Object* yObj;
    FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 0, xObj, context->strObjPool, context->strPool);
    FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 1, yObj, context->strObjPool, context->strPool);
    FoclIOBufferPrintf(context->outBuffer, "\033[%d;%dH", FoclObjectUnboxInt(xObj), FoclObjectUnboxInt(yObj));
    FoclIOBufferFlushOut(context->outBuffer);
    return FoclObjectVoid(context->strObjPool, context->strPool);
}
Focl_Object* term_getw(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* wObj = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        FoclObjectBoxInt(wObj, csbi.srWindow.Right - csbi.srWindow.Left + 1);
    }
    else
    {
        FoclObjectBoxInt(wObj, 0);
    }
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
    {
        FoclObjectBoxInt(wObj, w.ws_col);
    }
    else
    {
        FoclObjectBoxInt(wObj, 0);
    }
#endif
    return wObj;
}
Focl_Object* term_geth(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* hObj = FoclObjWithNoStringPoolAlloc(context->objWithNoStrPool, FOCL_OBJ_TYPE_INT);
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        FoclObjectBoxInt(hObj, csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    }
    else
    {
        FoclObjectBoxInt(hObj, 0);
    }
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
    {
        FoclObjectBoxInt(hObj, w.ws_row);
    }
    else
    {
        FoclObjectBoxInt(hObj, 0);
    }
#endif
    return hObj;
}
Focl_Object* term_hidecursor(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
    FoclIOBufferPrintf(context->outBuffer, HIDE_CURSOR);
    FoclIOBufferFlushOut(context->outBuffer);
#endif
    return FoclObjectVoid(context->strObjPool, context->strPool);
}
Focl_Object* term_showcursor(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
    FoclIOBufferPrintf(context->outBuffer, SHOW_CURSOR);
    FoclIOBufferFlushOut(context->outBuffer);
#endif
    return FoclObjectVoid(context->strObjPool, context->strPool);
}

void Focl_RegisterTerminalCommand(Focl_Context* context)
{
    FoclRegisterCommand(context, "term::clear", term_clear);
    FoclRegisterCommand(context, "term::gotoxy", term_gotoxy);
    FoclRegisterCommand(context, "term::getw", term_getw);
    FoclRegisterCommand(context, "term::geth", term_geth);
    FoclRegisterCommand(context, "term::hidecursor", term_hidecursor);
    FoclRegisterCommand(context, "term::showcursor", term_showcursor);
}