#include "focl.h"
void Focl_RegisterUtilsCommand(Focl_Context* context);
void Focl_RegisterMathCommand(Focl_Context* context);
void Focl_RegisterSystemCommand(Focl_Context* context);
void Focl_RegisterTerminalCommand(Focl_Context* context);

void Focl_RegisterBuiltinCommands(Focl_Context* context)
{
#ifdef FOCL_REGISTER_UTILS
    Focl_RegisterUtilsCommand(context);
#endif
#ifdef FOCL_REGISTER_MATH
    Focl_RegisterMathCommand(context);
#endif
#ifdef FOCL_REGISTER_SYS
    Focl_RegisterSystemCommand(context);
#endif
#ifdef FOCL_REGISTER_TERM
    Focl_RegisterTerminalCommand(context);
#endif
}