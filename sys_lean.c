#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <errno.h>

#ifdef _WIN32
#include <dirent.h>
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <libgen.h>
#endif

char* Focl_strdup(const char* src);

#ifdef _WIN32
int access(const char* filename, int accessMode)
{
    return _access(filename, accessMode);
}
#endif

bool Focl_isFileExist(const char* filename)
{
    return (access(filename, F_OK) == 0 ? true : false);
}

#ifndef PATH_MAX

#define PATH_MAX 4096
/* it's a temp method, I know. */

#endif

bool Focl_isDir(const char* path)
{
#ifdef _WIN32
    DWORD attrs = GetFileAttributes(path);
    if (attrs & FILE_ATTRIBUTE_DIRECTORY)
    {
        return true;
    }
    return false;
#else
    struct stat st;
    if (lstat(path, &st) != 0)
    {
        return false;
    }
    if (S_ISDIR(st.st_mode))
    {
        return true;
    }
    return false;
#endif
}
bool Focl_isNormalFile(const char* path)
{
#ifdef _WIN32
    DWORD attrs = GetFileAttributes(path);
    if (attrs & INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    else if (attrs & FILE_ATTRIBUTE_NORMAL)
    {
        return false;
    }
    else if (attrs & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        return false;
    }
    else
    {    
        return true;
    }
#else
    struct stat st;
    if (lstat(path, &st) != 0)
    {
        return false;
    }
    if (S_ISREG(st.st_mode))
    {
        return true;
    }
    return false;
#endif
}

ptrdiff_t Focl_GetFileSize(const char* path) /* Yes, it return ptrdiff_t. nevermind! */
{
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attrData;
    if (!GetFileAttributesEx(path, GetFileExInfoStandard, &attrData))
    {
        return -1;
    }
    LARGE_INTEGER size;
    size.LowPart = attrData.nFileSizeLow;
    size.HighPart = attrData.nFileSizeHigh;
    return size.QuadPart;
#else
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return -1;
    }
    return st.st_size;
#endif
}

int Focl_mkdir(const char* path) /* no -r */
{
#ifdef _WIN32
    return mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

char* Focl_realpath(const char* path, char* resolvedPath, size_t bufferSize)
{
#ifdef _WIN32
    return _fullpath(resolvedPath, path, bufferSize);
#else
    (void)bufferSize;
    return realpath(path, resolvedPath);
#endif
}

char* Focl_dirname(const char* path) /* free the return value! */
{
#ifdef _WIN32
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char result[MAX_PATH];
    
    errno_t err = _splitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
    
    if (err != 0)
    {
        return NULL;
    }
    
    snprintf(result, sizeof(result), "%s%s", drive, dir);
    
    size_t len = strlen(result);
    if (len > 0 && (result[len - 1] == '\\' || result[len - 1] == '/'))
    {
        result[len-1] = '\0';
    }
    
    if (strlen(result) == 0)
    {
        return Focl_strdup(".");
    }
    
    return Focl_strdup(result);
#else
    char *copy = Focl_strdup(path);
    char *dir = dirname(copy);
    char *result = Focl_strdup(dir);
    free(copy);
    return result;
#endif
}

#define FOCL_FILECOPY_BUFFER_SIZE 4096

int Focl_fileCopy(const char* src, const char* dst)
{
    FILE *fsrc = fopen(src, "rb");
    if (!fsrc)
    {
        return -1;
    }
    FILE *fdst = fopen(dst, "wb");
    if (!fdst)
    {
        fclose(fsrc);
        return -1;
    }
    
    char buffer[FOCL_FILECOPY_BUFFER_SIZE];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), fsrc)) > 0)
    {
        if (fwrite(buffer, 1, n, fdst) != n)
        {
            fclose(fsrc);
            fclose(fdst);
            remove(dst);
            return -1;
        }
    }
    int result = ferror(fsrc) ? -1 : 0;
    fclose(fsrc);
    fclose(fdst);
    return result;
}