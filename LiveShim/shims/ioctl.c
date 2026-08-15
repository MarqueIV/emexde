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

#if LIVESHIM_IOCTL_ENABLED

static int ksurface_user_ioctl(int fd, unsigned long flag, ...);
static int ksurface_user_isatty(int fd);
static int ksurface_user_tcgetattr(int fd, struct termios *t);
static int ksurface_user_tcsetattr(int fd, int options, struct termios *t);
static int ksurface_user_tcsetpgrp(int fd, pid_t pgrp);
static int ksurface_user_tcgetpgrp(int fd);

INTERPOSE(ksurface_user_ioctl, ioctl);
INTERPOSE(ksurface_user_isatty, isatty);
INTERPOSE(ksurface_user_tcgetattr, tcgetattr);
INTERPOSE(ksurface_user_tcsetattr, tcsetattr);
INTERPOSE(ksurface_user_tcsetpgrp, tcsetpgrp);
INTERPOSE(ksurface_user_tcgetpgrp, tcgetpgrp);

static int ksurface_user_ioctl(int fd,
                               unsigned long flag,
                               ...)
{
    /* starting variadic argument parse */
    va_list args;
    va_start(args, flag);
    
    /* parsing arguments */
    int64_t sys_args[7];
    for(uint8_t i = 0; i < 6; i++)
    {
        sys_args[i] = va_arg(args, int64_t);
    }
    
    /* ending parse */
    va_end(args);
    
    int ret = (int)liveshim_syscall(SYS_ioctl, fd, flag, sys_args[0], sys_args[1], sys_args[2], sys_args[3], sys_args[4], sys_args[5], sys_args[6]);
    
    if(ret != 0 &&
       errno == ENOSYS)
    {
        int (*darwin_user_ioctl)(int fd, unsigned long flag, ...) = _interpose_ioctl.replacee;
        return darwin_user_ioctl(fd, flag, sys_args[0], sys_args[1], sys_args[2], sys_args[3], sys_args[4], sys_args[5], sys_args[6]);
    }
    
    return ret;
}

static int ksurface_user_isatty(int fd)
{
    struct termios termios;
    return liveshim_syscall(SYS_ioctl, fd, TIOCGETA, &termios) == 0;
}

static int ksurface_user_tcgetattr(int fd,
                                   struct termios *t)
{
    return (int)liveshim_syscall(SYS_ioctl, fd, TIOCGETA, t);
}

static int ksurface_user_tcsetattr(int fd,
                                   int options,
                                   struct termios *t)
{
    unsigned long req;

    switch(options)
    {
        case TCSANOW:
            req = TIOCSETA;
            break;
        case TCSADRAIN:
            req = TIOCSETAW;
            break;
        case TCSAFLUSH:
            req = TIOCSETAF;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    
    return (int)liveshim_syscall(SYS_ioctl, fd, req, t);
}

static int ksurface_user_tcsetpgrp(int fd,
                                   pid_t pgrp)
{
    return (int)liveshim_syscall(SYS_ioctl, fd, TIOCSPGRP, &pgrp);
}

static int ksurface_user_tcgetpgrp(int fd)
{
    pid_t pgrp = 0;
    int ret = (int)liveshim_syscall(SYS_ioctl, fd, TIOCGPGRP, &pgrp);
    return (ret == 0) ? pgrp : -1;
}

#endif /* LIVESHIM_IOCTL_ENABLED */
