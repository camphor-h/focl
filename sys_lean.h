#ifndef SYS_LEAN_H
#define SYS_LEAN_H

#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
int access(const char* filename, int accessMode);
#endif

bool Focl_isFileExist(const char* filename);
bool Focl_isDir(const char* path);
bool Focl_isNormalFile(const char* path);
ptrdiff_t Focl_GetFileSize(const char* path); /* Yes, it return ptrdiff_t. nevermind! */
int Focl_mkdir(const char* path); /* no -r */
char* Focl_realpath(const char* path, char* resolvedPath, size_t bufferSize);
char* Focl_dirname(const char* path); /* free the return value! */
int Focl_fileCopy(const char* src, const char* dst);

#endif