#include "focl_dev.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <wchar.h>
#include <locale.h>
#include <wctype.h>

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
    Focl_Object* colObj;
    Focl_Object* rowObj;
    FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 0, colObj, context->strObjPool, context->strPool);
    FOCL_OBJ_VEC_AT_AS_INT_OBJ(objVec, 1, rowObj, context->strObjPool, context->strPool);
    Focl_Obj_Int col = FoclObjectUnboxInt(colObj);
    Focl_Obj_Int row = FoclObjectUnboxInt(rowObj);
    if (col < 0 || row < 0)
    {
        return FoclObjectError(context->strObjPool, context->strPool, "Coordinate cannot be negative");
    }
    FoclIOBufferPrintf(context->outBuffer, "\033[%" FOCL_FORMAT_INT ";%" FOCL_FORMAT_INT "H", row + 1, col + 1);
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
    Focl_Object* wObj = FoclFlatObjPoolAlloc(context->flatObjPool, FOCL_OBJ_TYPE_INT);
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
    Focl_Object* hObj = FoclFlatObjPoolAlloc(context->flatObjPool, FOCL_OBJ_TYPE_INT);
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

typedef struct
{
    const char* name;
    const char* code;
} Focl_TermColor;

static const Focl_TermColor focl_termColors[] =
{
    {"black", "30"},
    {"red", "31"},
    {"green", "32"},
    {"yellow", "33"},
    {"blue", "34"},
    {"magenta", "35"},
    {"cyan", "36"},
    {"white", "37"},
    {"lightblack", "90"},
    {"lightred", "91"},
    {"lightgreen", "92"},
    {"lightyellow", "93"},
    {"lightblue", "94"},
    {"lightmagenta", "95"},
    {"lightcyan", "96"},
    {"lightwhite", "97"},
    {"reverse", "7"},
    {"noreverse", "27"},
};

#define FOCL_TERM_COLOR_COUNT (sizeof(focl_termColors) / sizeof(focl_termColors[0]))

Focl_Object* term_color(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* colorNameObj;
    Focl_String* colorName;
    FOCL_OBJ_VEC_AT_AS_STRING(objVec, 0, colorNameObj, colorName, context->strObjPool, context->strPool);

    if (FoclStrComp(colorName, "list") == 0)
    {
        for (size_t i = 0; i < FOCL_TERM_COLOR_COUNT; i++)
        {
            FoclIOBufferPrintf(context->outBuffer, "%s\n", focl_termColors[i].name);
        }
    }
    else if (FoclStrComp(colorName, "clear") == 0)
    {
        FoclIOBufferPrintf(context->outBuffer, "\033[0m");
        FoclIOBufferFlushOut(context->outBuffer);
    }
    else
    {
        for (size_t i = 0; i < FOCL_TERM_COLOR_COUNT; i++)
        {
            if (FoclStrComp(colorName, focl_termColors[i].name) == 0)
            {
                FoclIOBufferPrintf(context->outBuffer, "\033[%sm", focl_termColors[i].code);
                FoclIOBufferFlushOut(context->outBuffer);
                return FoclObjectVoid(context->strObjPool, context->strPool);
            }
        }
        return FoclObjectError(context->strObjPool, context->strPool, "unknown color");
    }
    return FoclObjectVoid(context->strObjPool, context->strPool);
}

int32_t getUtf8CodePoint(const char* bytes);

int getUtf8CharDisplayWidth(const char* str)
{
    int32_t cp = getUtf8CodePoint(str);

    if (cp < 0x80)
    {
        return 1;
    }

    if ((cp >= 0x4E00 && cp <= 0x9FFF) ||
        (cp >= 0x3400 && cp <= 0x4DBF))
    {
        return 2;
    }

#if WCHAR_MAX >= 0x20000
    if ((cp >= 0x20000 && cp <= 0x2A6DF) ||
        (cp >= 0x2A700 && cp <= 0x2B73F) ||
        (cp >= 0x2B740 && cp <= 0x2B81F) ||
        (cp >= 0x2B820 && cp <= 0x2CEAF) ||
        (cp >= 0x2CEB0 && cp <= 0x2EBEF))
    {
        return 2;
    }
#endif

    if ((cp >= 0xFF00 && cp <= 0xFFEF) ||
        (cp >= 0x3000 && cp <= 0x303F))
    {
        return 2;
    }

    if (cp >= 0xAC00 && cp <= 0xD7AF)
    {
        return 2;
    }

    if ((cp >= 0x3040 && cp <= 0x309F) ||
        (cp >= 0x30A0 && cp <= 0x30FF))
    {
        return 2;
    }

    return 1;
}

int getUtf8StringDisplayWidth(const char* str)
{
    int width = 0;
    const char* p = str;

    while (*p != 0)
    {
        width += getUtf8CharDisplayWidth(p);
        size_t len = getUtf8CodePointLength((unsigned char)*p);
        p += len;
    }

    return width;
}
Focl_Object* term_stringwidth(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd)
{
    (void)cmd;
    if (FoclVectorGetSize(objVec) != 1)
    {
        return FoclObjectError(context->strObjPool, context->strPool, FOCL_ERR_UNSUPPORTED_ARG_COUNT);
    }
    Focl_Object* stringObj;
    Focl_String* string;
    FOCL_OBJ_VEC_AT_AS_STRING(objVec, 0, stringObj, string, context->strObjPool, context->strPool);
    Focl_Object* widthObj = FoclFlatObjPoolAlloc(context->flatObjPool, FOCL_OBJ_TYPE_INT);
    FoclObjectBoxInt(widthObj, getUtf8StringDisplayWidth(FoclStrCStr(string)));
    return widthObj;
}

void Focl_RegisterTerminalCommand(Focl_Context* context)
{
    FoclRegisterCommand(context, "term::clear", term_clear);
    FoclRegisterCommand(context, "term::gotoxy", term_gotoxy);
    FoclRegisterCommand(context, "term::getw", term_getw);
    FoclRegisterCommand(context, "term::geth", term_geth);
    FoclRegisterCommand(context, "term::hidecursor", term_hidecursor);
    FoclRegisterCommand(context, "term::showcursor", term_showcursor);
    FoclRegisterCommand(context, "term::color", term_color);
    FoclRegisterCommand(context, "term::stringwidth", term_stringwidth);
}