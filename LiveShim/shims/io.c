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

#include <LiveShim/shim.h>

#if LIVESHIM_IO_ENABLED

static int ksurface_user_open(const char *path, int flags, ...);
static DIR *ksurface_user_opendir(const char *path);
static int ksurface_user_faccessat(int dirFd, const char *access, int mode, int flags);
static int ksurface_user_access(const char *path, int mode);
static int ksurface_user_getattrlist(const char *a, void *b, void *c, size_t d, unsigned int e);

INTERPOSE(ksurface_user_open, open);
INTERPOSE(ksurface_user_opendir, opendir);
INTERPOSE(ksurface_user_faccessat, faccessat);
INTERPOSE(ksurface_user_access, access);
INTERPOSE(ksurface_user_getattrlist, getattrlist);

static int ksurface_user_open(const char *path,
                              int flags,
                              ...)
{
    int (*darwin_user_open)(const char *path, int flags, ...) = _interpose_open.replacee;
    
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
        return darwin_user_open(path, flags, mode);
    }
    
    int fd = fileport_makefd(fileport);
    mach_port_deallocate(mach_task_self(), fileport);
    if(fd < 0)
    {
        return darwin_user_open(path, flags, mode);
    }
    
    return fd;
}

static DIR *ksurface_user_opendir(const char *path)
{
    DIR *(*darwin_user_opendir)(const char *path) = _interpose_opendir.replacee;
    
    fileport_t fileport;
    if(liveshim_syscall(SYS_open, path, O_RDONLY | O_DIRECTORY, 0, &fileport) != 0)
    {
        return darwin_user_opendir(path);
    }
    
    int dirFd = fileport_makefd(fileport);
    mach_port_deallocate(mach_task_self(), fileport);
    if(dirFd < 0)
    {
        return darwin_user_opendir(path);
    }
    
    DIR *dir = fdopendir(dirFd);
    if(dir == NULL)
    {
        close(dirFd);
        return NULL;
    }
    
    return dir;
}

static int ksurface_user_faccessat(int dirFd,
                                   const char *path,
                                   int mode,
                                   int flags)
{
    int (*darwin_user_faccessat)(int dirFd, const char *path, int mode, int flags) = _interpose_faccessat.replacee;
    if(liveshim_syscall(SYS_faccessat, dirFd, path, mode, flags) != 0)
    {
        return darwin_user_faccessat(dirFd, path, mode, flags);
    }
    return 0;
}

static int ksurface_user_access(const char *path,
                                int mode)
{
    return ksurface_user_faccessat(AT_FDCWD, path, mode, 0);
}

static int ksurface_user_getattrlist(const char *path,
                                     void *attrList,
                                     void *attrBuf,
                                     size_t attrBufSize,
                                     unsigned int options)
{
    int (*darwin_user_getattrlist)(const char *a, void *b, void *c, size_t d, unsigned int e) = _interpose_getattrlist.replacee;
    int ret = (int)liveshim_syscall(SYS_getattrlist, path, attrList, attrBuf, attrBufSize, options);
    if(ret == -1 && errno == ENOSYS)
    {
        ret = darwin_user_getattrlist(path, attrList, attrBuf, attrBufSize, options);
    }
    return (int)ret;
}

#endif /* LIVESHIM_IO_ENABLED */
