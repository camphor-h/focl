#ifndef FOCL_H
#define FOCL_H

/* 
 * The aim of the file is to create a interface for outer program who wants
 * to embed Focl. The focl runtime don't lean on it
 */

typedef struct Focl_Vector Focl_Vector;
typedef struct Focl_Object Focl_Object;
typedef struct Focl_Context Focl_Context;
typedef struct Focl_Command Focl_Command;
typedef Focl_Object* (*Focl_CommandFunc)(Focl_Context* context, Focl_Vector* objVec, Focl_Command* cmd);

Focl_Context* createFoclContext();
void freeFoclContext(Focl_Context* context);
int Focl_REPL(Focl_Context* ctx);
int Focl_ExecFile(Focl_Context* ctx, const char* filename);
void FoclRegisterCommand(Focl_Context* context, const char* cmdName, Focl_CommandFunc func);
Focl_Object* Focl_eval(Focl_Context* context, const char* Cstr);

#endif