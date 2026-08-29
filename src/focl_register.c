#include "focl.h"

#ifdef FOCL_REGISTER_UTILS
void Focl_RegisterUtilsCommand(Focl_Context* context);
#endif

#ifdef FOCL_REGISTER_MATH
void Focl_RegisterMathCommand(Focl_Context* context);
#endif

#ifdef FOCL_REGISTER_UTILS
void Focl_RegisterSystemCommand(Focl_Context* context);
#endif

#ifdef FOCL_REGISTER_TERM
void Focl_RegisterTerminalCommand(Focl_Context* context);
#endif

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