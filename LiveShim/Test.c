//
//  Test.c
//  Nyxian
//
//  Created by Catelyn on 15.08.26.
//

#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/attr.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>

#define INTERPOSE(_replacement, _replacee)                       \
    __attribute__((used))                                       \
    static struct {                                              \
        const void *replacement;                                 \
        const void *replacee;                                    \
    } _interpose_##_replacee                                     \
    __attribute__((section("__DATA,__interpose"))) = {           \
        (const void *)(unsigned long)&_replacement,               \
        (const void *)(unsigned long)&_replacee                   \
    }

struct interpose_pair {
    const void *replacement;
    const void *replacee;
};

static int ksurface_user_open(const char *path, int flags, ...);
static DIR *ksurface_user_opendir(const char *path);

INTERPOSE(ksurface_user_open, open);
INTERPOSE(ksurface_user_opendir, opendir);

static void hook_log(const char *s)
{
    syscall(SYS_write, STDERR_FILENO, s, strlen(s));
}

static int ksurface_user_open(const char *path,
                              int flags,
                              ...)
{
    mode_t mode = 0;
    
    if(flags & O_CREAT)
    {
        va_list ap;
        va_start(ap, flags);
        mode = (mode_t)va_arg(ap, int);
        va_end(ap);
    }
    
    hook_log("[open] ");
    hook_log(path);
    hook_log("\n");
    
    return (int)syscall(SYS_open, path, flags, mode);
}

static DIR *ksurface_user_opendir(const char *path)
{
    hook_log("[opendir] ");
    hook_log(path);
    hook_log("\n");
    
    DIR *(*real_user_opendir)(const char *path) = _interpose_opendir.replacee;
    return real_user_opendir(path);
}
