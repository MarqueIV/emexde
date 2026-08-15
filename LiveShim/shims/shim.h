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

#ifndef LIVESHIM_SHIM_H
#define LIVESHIM_SHIM_H

#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/attr.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
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

#define LIVESHIM_IO_ENABLED  0
#define LIVESHIM_IOCTL_ENABLED  1
#define LIVESHIM_SYSCTL_ENABLED  1

#endif /* LIVESHIM_SHIM_H */
