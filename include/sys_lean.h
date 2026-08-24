#ifndef SYS_LEAN_H
#define SYS_LEAN_H

#include <stdbool.h>
#include <stddef.h>

#ifndef PATH_MAX

#define PATH_MAX 4096
/* it's a temp method, I know. */

#endif

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
char* Focl_GetHomeDirectory(); /* free the return value! */
char* Focl_normalizePath(const char* path); /* free the return value! */
int Focl_remove(const char* path);
int Focl_copy(const char* src, const char* dst);
char* Focl_GetPathLastName(const char *path); /* free the return value! */
int Focl_execAndWait(const char* command, char* const argv[]);
char* Focl_GetFileNameWithoutExt(const char* path); /* free the return value! */

#endif