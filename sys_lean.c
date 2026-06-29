#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

#ifdef _WIN32
#include <dirent.h>
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
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

int Focl_fileCopy(const char* src, const char* dst)
{
#ifdef _WIN32
    if (CopyFile(src, dst, FALSE))
    {
        return 0;
    }
    return -1;
#else
    int fdin = open(src, O_RDONLY);
    if (fdin < 0)
    {
        return -1;
    }
    int fdout = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fdout < 0)
    {
        close(fdin);
        return -1;
    }
    struct stat st;
    fstat(fdin, &st);
    ssize_t result = sendfile(fdout, fdin, NULL, st.st_size);
    close(fdin);
    close(fdout);
    return (result == st.st_size) ? 0 : -1;
#endif
}