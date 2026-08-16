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

#if LIVESHIM_UCRED_ENABLED

static uid_t ksurface_user_getuid(void);
static gid_t ksurface_user_getgid(void);
static uid_t ksurface_user_geteuid(void);
static gid_t ksurface_user_getegid(void);
static pid_t ksurface_user_getppid(void);
static int ksurface_user_setuid(uid_t uid);
static int ksurface_user_seteuid(uid_t euid);
static int ksurface_user_setruid(uid_t uid);
static int ksurface_user_setreuid(uid_t ruid, uid_t euid);
static int ksurface_user_setgid(gid_t gid);
static int ksurface_user_setegid(gid_t gid);
static int ksurface_user_setrgid(gid_t gid);
static int ksurface_user_setregid(gid_t egid, gid_t rgid);
static pid_t ksurface_user_getsid(pid_t sid);
static int ksurface_user_setsid(void);

INTERPOSE(ksurface_user_getuid, getuid);
INTERPOSE(ksurface_user_getgid, getgid);
INTERPOSE(ksurface_user_geteuid, geteuid);
INTERPOSE(ksurface_user_getegid, getegid);
INTERPOSE(ksurface_user_getppid, getppid);
INTERPOSE(ksurface_user_setuid, setuid);
INTERPOSE(ksurface_user_seteuid, seteuid);
INTERPOSE(ksurface_user_setruid, setruid);
INTERPOSE(ksurface_user_setreuid, setreuid);
INTERPOSE(ksurface_user_setgid, setgid);
INTERPOSE(ksurface_user_setegid, setegid);
INTERPOSE(ksurface_user_setrgid, setrgid);
INTERPOSE(ksurface_user_setregid, setregid);
INTERPOSE(ksurface_user_getsid, getsid);
INTERPOSE(ksurface_user_setsid, setsid);

static uid_t ksurface_user_getuid(void)
{
    return (uid_t)liveshim_syscall(SYS_getuid);
}

static gid_t ksurface_user_getgid(void)
{
    return (gid_t)liveshim_syscall(SYS_getgid);
}

static uid_t ksurface_user_geteuid(void)
{
    return (uid_t)liveshim_syscall(SYS_geteuid);
}

static gid_t ksurface_user_getegid(void)
{
    return (gid_t)liveshim_syscall(SYS_getegid);
}

static pid_t ksurface_user_getppid(void)
{
    return (pid_t)liveshim_syscall(SYS_getppid);
}

static int ksurface_user_setuid(uid_t uid)
{
    return (int)liveshim_syscall(SYS_setuid, uid);
}

static int ksurface_user_seteuid(uid_t euid)
{
    return (int)liveshim_syscall(SYS_seteuid, euid);
}

static int ksurface_user_setruid(uid_t uid)
{
    return (int)liveshim_syscall(SYS_setreuid, uid, -1);
}

static int ksurface_user_setreuid(uid_t ruid, uid_t euid)
{
    return (int)liveshim_syscall(SYS_setreuid, ruid, euid);
}

static int ksurface_user_setgid(gid_t gid)
{
    return (int)liveshim_syscall(SYS_setgid, gid);
}

static int ksurface_user_setegid(gid_t gid)
{
    return (int)liveshim_syscall(SYS_setegid, gid);
}

static int ksurface_user_setrgid(gid_t gid)
{
    return (int)liveshim_syscall(SYS_setregid, gid, -1);
}

static int ksurface_user_setregid(gid_t egid, gid_t rgid)
{
    return (int)liveshim_syscall(SYS_setregid, egid, rgid);
}

static pid_t ksurface_user_getsid(pid_t sid)
{
    return (pid_t)liveshim_syscall(SYS_getsid, sid);
}

static int ksurface_user_setsid(void)
{
    return (int)liveshim_syscall(SYS_setsid);
}

#endif /* LIVESHIM_UCRED_ENABLED */
