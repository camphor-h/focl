#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pwd.h>
#include <libgen.h>
    #include <sys/wait.h>

#ifdef __linux__
    #include <linux/fs.h>
    #ifndef HAVE_COPY_FILE_RANGE
    #define HAVE_COPY_FILE_RANGE 1
    #endif
#endif
    
#ifdef __APPLE__
    #include <copyfile.h>
#endif

#endif

char* Focl_strdup(const char* src);
void* Focl_malloc(size_t size);

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
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    if (attrs & FILE_ATTRIBUTE_DIRECTORY)
    {
        return false;
    }
    if (attrs & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        return false;
    }
    if (attrs & FILE_ATTRIBUTE_DEVICE)
    {
        return false;
    }
    return true;
#else
    struct stat st;
    if (lstat(path, &st) != 0)
    {
        return false;
    }
    return S_ISREG(st.st_mode);
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


char* Focl_GetPathLastName(const char *path)
{
    const char* last = strrchr(path, '/');
#ifdef _WIN32
    const char* last2 = strrchr(path, '\\');
    if (last2 && (!last || last2 > last))
    {
        last = last2;
    }
#endif
    if (last)
    {
        return Focl_strdup(last + 1);
    }
    return Focl_strdup(path);
}

#define FOCL_FILECOPY_BUFFER_SIZE 4096

int Focl_fileCopy(const char* src, const char* dst)
{
#ifdef _WIN32
    if (!CopyFileA(src, dst, TRUE))
    {
        return -1;
    }
    return 0;
    
#else
    struct stat st;
    if (lstat(src, &st) != 0)
    {
        return -1;
    }
    
    if (S_ISLNK(st.st_mode))
    {
        char linkTarget[PATH_MAX];
        ssize_t len = readlink(src, linkTarget, sizeof(linkTarget) - 1);
        if (len == -1)
        {
            return -1;
        }
        linkTarget[len] = '\0';
        unlink(dst);
        
        if (symlink(linkTarget, dst) != 0)
        {
            return -1;
        }
        return 0;
    }
    int fd_src = open(src, O_RDONLY);
    if (fd_src == -1)
    {
        return -1;
    }
    mode_t mode = st.st_mode & 0777;
    
    int fd_dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd_dst == -1)
    {
        close(fd_src);
        return -1;
    }
    
    off_t offset = 0;
    ssize_t bytes_sent;
    int result = 0;
#if defined(__linux__) && defined(HAVE_COPY_FILE_RANGE)
    while ((bytes_sent = copy_file_range(fd_src, NULL, fd_dst, NULL, 
                                         1024 * 1024, 0)) > 0) {}
    if (bytes_sent == -1)
    {
        bytes_sent = 0;
    }
    else
    {
        result = 0;
    }
#endif
#if defined(__linux__) || defined(__sun__)
    if (result == 0 && bytes_sent == 0)
    {
        while ((bytes_sent = sendfile(fd_dst, fd_src, &offset, 
                                      1024 * 1024)) > 0) {}
        if (bytes_sent == -1 && offset == 0)
        {
            char buffer[8192];
            ssize_t n;
            while ((n = read(fd_src, buffer, sizeof(buffer))) > 0)
            {
                if (write(fd_dst, buffer, n) != n)
                {
                    result = -1;
                    break;
                }
            }
            if (n == -1) result = -1;
        }
    }
#else
    #ifdef __APPLE__
    if (fcopyfile(fd_src, fd_dst, NULL, COPYFILE_ALL) != 0)
    {
        result = -1;
    }
    #else
    char buffer[8192];
    ssize_t n;
    while ((n = read(fd_src, buffer, sizeof(buffer))) > 0)
    {
        if (write(fd_dst, buffer, n) != n)
        {
            result = -1;
            break;
        }
    }
    if (n == -1) result = -1;
    #endif
#endif
    struct timespec times[2];
    times[0] = st.st_atim;
    times[1] = st.st_mtim;
    futimens(fd_dst, times);
    
    close(fd_src);
    close(fd_dst);
    
    if (result == -1)
    {
        unlink(dst);
        return -1;
    }
    
    return 0;
#endif
}

char* Focl_GetHomeDirectory()
{
    char* home = NULL;
#ifdef _WIN32
    char path[MAX_PATH];
    HRESULT result = SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
    if (SUCCEEDED(result))
    {
        home = (char*)Focl_malloc(strlen(path) + 1);
        if (home)
        {
            strcpy(home, path);
        }
    }
#else
    home = getenv("HOME");
    if (home)
    {
        char* temp = (char*)Focl_malloc(strlen(home) + 1);
        if (temp)
        {
            strcpy(temp, home);
            return temp;
        }
    }
    struct passwd* passwordEntry = getpwuid(getuid());
    if (passwordEntry && passwordEntry->pw_dir)
    {
        home = (char*)Focl_malloc(strlen(passwordEntry->pw_dir) + 1);
        if (home)
        {
            strcpy(home, passwordEntry->pw_dir);
        }
    }
#endif
    return home;
}

int Focl_removeDir(const char* path)
{
    if (!path || !Focl_isDir(path))
    {
        return -1;
    }

#ifdef _WIN32
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    
    do
    {
        if (strcmp(findData.cFileName, ".") == 0 || 
            strcmp(findData.cFileName, "..") == 0)
        {
            continue;
        }
        
        char fullPath[MAX_PATH];
        snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, findData.cFileName);
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (Focl_removeDir(fullPath) != 0)
            {
                FindClose(hFind);
                return -1;
            }
        }
        else
        {
            if (DeleteFileA(fullPath) == 0)
            {
                FindClose(hFind);
                return -1;
            }
        }
    }
    while (FindNextFileA(hFind, &findData) != 0);
    
    FindClose(hFind);
    
    if (RemoveDirectoryA(path) == 0)
    {
        return -1;
    }
    
    return 0;
    
#else
    DIR* dir = opendir(path);
    if (!dir)
    {
        return -1;
    }
    
    struct dirent* entry;
    struct stat st;
    char fullPath[PATH_MAX];
    
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        
        if (lstat(fullPath, &st) != 0)
        {
            closedir(dir);
            return -1;
        }
        
        if (S_ISDIR(st.st_mode))
        {
            if (Focl_removeDir(fullPath) != 0)
            {
                closedir(dir);
                return -1;
            }
        }
        else
        {
            if (unlink(fullPath) != 0)
            {
                closedir(dir);
                return -1;
            }
        }
    }
    
    closedir(dir);
    if (rmdir(path) != 0)
    {
        return -1;
    }
    
    return 0;
#endif
}

int Focl_copyDir(const char* src, const char* dst)
{
    if (!src || !dst || !Focl_isDir(src))
    {
        return -1;
    }
    if (!Focl_isFileExist(dst))
    {
        if (Focl_mkdir(dst) != 0)
        {
            return -1;
        }
    }
    else if (!Focl_isDir(dst))
    {
        return -1;
    }

#ifdef _WIN32
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", src);
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    
    do
    {
        if (strcmp(findData.cFileName, ".") == 0 || 
            strcmp(findData.cFileName, "..") == 0)
        {
            continue;
        }
        
        char srcFullPath[MAX_PATH];
        char dstFullPath[MAX_PATH];
        snprintf(srcFullPath, sizeof(srcFullPath), "%s\\%s", src, findData.cFileName);
        snprintf(dstFullPath, sizeof(dstFullPath), "%s\\%s", dst, findData.cFileName);
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (Focl_copyDir(srcFullPath, dstFullPath) != 0)
            {
                FindClose(hFind);
                return -1;
            }
        }
        else
        {
            if (Focl_fileCopy(srcFullPath, dstFullPath) != 0)
            {
                FindClose(hFind);
                return -1;
            }
        }
    }
    while (FindNextFileA(hFind, &findData) != 0);
    
    FindClose(hFind);
    return 0;
    
#else
    DIR* dir = opendir(src);
    if (!dir)
    {
        return -1;
    }
    
    struct dirent* entry;
    struct stat st;
    char srcFullPath[PATH_MAX];
    char dstFullPath[PATH_MAX];
    
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        
        snprintf(srcFullPath, sizeof(srcFullPath), "%s/%s", src, entry->d_name);
        snprintf(dstFullPath, sizeof(dstFullPath), "%s/%s", dst, entry->d_name);
        
        if (lstat(srcFullPath, &st) != 0)
        {
            closedir(dir);
            return -1;
        }
        
        if (S_ISDIR(st.st_mode))
        {
            if (Focl_copyDir(srcFullPath, dstFullPath) != 0)
            {
                closedir(dir);
                return -1;
            }
        }
        else if (S_ISREG(st.st_mode))
        {
            if (Focl_fileCopy(srcFullPath, dstFullPath) != 0)
            {
                closedir(dir);
                return -1;
            }
        }
    }
    
    closedir(dir);
    return 0;
#endif
}

char* _Focl_normalizePath(const char* path, char* buffer, size_t bufferSize)
{
    if (!path || !buffer || bufferSize == 0)
    {
        return NULL;
    }

    char temp[PATH_MAX];
    char cwd[PATH_MAX];
    
    if (path[0] == '~')
    {
        char* home = Focl_GetHomeDirectory();
        if (!home)
        {
            return NULL;
        }
        
        if (path[1] == '\0' || path[1] == '/' || path[1] == '\\')
        {
            snprintf(temp, sizeof(temp), "%s%s", home, path + 1);
        }
        else
        {
            free(home);
            return NULL;
        }
        free(home);
    }
    else if (path[0] == '/' || path[0] == '\\')
    {
        strncpy(temp, path, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';
    }
#ifdef _WIN32
    else if (isalpha(path[0]) && path[1] == ':')
    {
        strncpy(temp, path, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';
    }
#endif
    else
    {
        if (!getcwd(cwd, sizeof(cwd)))
        {
            return NULL;
        }
        snprintf(temp, sizeof(temp), "%s/%s", cwd, path);
    }
    
#ifdef _WIN32
    for (char* p = temp; *p; p++)
    {
        if (*p == '/') *p = '\\';
    }
#else
    for (char* p = temp; *p; p++)
    {
        if (*p == '\\') *p = '/';
    }
#endif
    
    size_t len = strlen(temp);
    while (len > 1 && (temp[len-1] == '/' || temp[len-1] == '\\'))
    {
        temp[--len] = '\0';
    }
    
    if (!Focl_realpath(temp, buffer, bufferSize))
    {
        return NULL;
    }
    
    return buffer;
}

char* Focl_normalizePath(const char* path)
{
    if (!path)
    {
        return NULL;
    }

    char* buffer = (char*)Focl_malloc(PATH_MAX);
    if (!buffer)
    {
        return NULL;
    }
    
    if (!_Focl_normalizePath(path, buffer, PATH_MAX))
    {
        free(buffer);
        return NULL;
    }
    
    return buffer;
}

int Focl_remove(const char* path)
{
    if (!path || !Focl_isFileExist(path))
    {
        return -1;
    }
    
    if (Focl_isNormalFile(path))
    {
        return remove(path);
    }
    return Focl_removeDir(path);
}

int Focl_copy(const char* src, const char* dst)
{
    if (!src || !dst || !Focl_isFileExist(src))
    {
        return -1;
    }
    
    if (Focl_isNormalFile(src))
    {
        return Focl_fileCopy(src, dst);
    }
    return Focl_copyDir(src, dst);
}

int Focl_execAndWait(const char* command, char* const argv[])
{
    #ifdef _WIN32
        (void)command;
        STARTUPINFO si = {0};
        PROCESS_INFORMATION pi = {0};
        char cmdline[4096] = {0};
        int i;
        DWORD exit_code;
        
        for (i = 0; argv[i] != NULL; i++)
        {
            strcat(cmdline, argv[i]);
            if (argv[i + 1] != NULL)
            {
                strcat(cmdline, " ");
            }
        }
        
        si.cb = sizeof(STARTUPINFO);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            return -1;
        }
        
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        GetExitCodeProcess(pi.hProcess, &exit_code);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        return (int)exit_code;
        
    #else
        pid_t pid = fork();
        int status;
        
        if (pid < 0)
        {
            return -1;
        }
        
        if (pid == 0)
        {
            execvp(command, argv);
            exit(127);
        }
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }
        else
        {
            return -1;
        }
    #endif
}

char* Focl_GetFileNameWithoutExt(const char* path)
{
    char* fullname = Focl_GetPathLastName(path);
    if (!fullname)
    {
        return NULL;
    }
    
    char* dot = strrchr(fullname, '.');
    if (dot)
    {
        if (dot == fullname || (dot > fullname && *(dot - 1) == '.'))
        {
            return fullname;
        }
        if (*(dot + 1) != '\0')
        {
            *dot = '\0';
        }
    }
    
    return fullname;
}