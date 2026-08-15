/*
 SPDX-License-Identifier: AGPL-3.0-or-later

 Copyright (C) 2025 - 2026 emexlab

 This file is part of Nyxian.

 Nyxian is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Nyxian is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with Nyxian. If not, see <https://www.gnu.org/licenses/>.
*/

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
#include <LiveShim/LiveShimSyscall.h>
#include <LiveShim/fileport.h>

#define INTERPOSE(_replacement, _replacee)                          \
    __attribute__((used))                                           \
    static struct {                                                 \
        const void *replacement;                                    \
        const void *replacee;                                       \
    } _interpose_##_replacee                                        \
    __attribute__((section("__DATA,__interpose"))) = {              \
        (const void *)(unsigned long)&_replacement,                 \
        (const void *)(unsigned long)&_replacee                     \
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
    write(STDERR_FILENO, s, strlen(s));
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
    
    fileport_t fileport;
    if(liveshim_syscall(SYS_open, path, flags, mode, &fileport) != 0)
    {
        return (int)syscall(SYS_open, path, flags, mode);
    }
    
    int fd = fileport_makefd(fileport);
    mach_port_deallocate(mach_task_self(), fileport);
    if(fd < 0)
    {
        return (int)syscall(SYS_open, path, flags, mode);
    }
    
    return fd;
}

static DIR *ksurface_user_opendir(const char *path)
{
    fileport_t fileport;
    if(liveshim_syscall(SYS_open, path, O_RDONLY | O_DIRECTORY, 0, &fileport) != 0)
    {
        DIR *(*real_user_opendir)(const char *path) = _interpose_opendir.replacee;
        return real_user_opendir(path);
    }
    
    int dirFd = fileport_makefd(fileport);
    mach_port_deallocate(mach_task_self(), fileport);
    if(dirFd < 0)
    {
        DIR *(*real_user_opendir)(const char *path) = _interpose_opendir.replacee;
        return real_user_opendir(path);
    }
    
    DIR *dir = fdopendir(dirFd);
    if(dir == NULL)
    {
        close(dirFd);
        return NULL;
    }
    
    return dir;
}
