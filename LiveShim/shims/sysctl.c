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

#if LIVESHIM_SYSCTL_ENABLED

static int ksurface_user_sysctl(int *name, u_int namelen, void *__sized_by(*oldlenp) oldp, size_t *oldlenp, void *__sized_by(newlen) newp, size_t newlen);
static int ksurface_user_gethostname(char *name, size_t len);
static int ksurface_user_sethostname(char *name, size_t len);

INTERPOSE(ksurface_user_sysctl, sysctl);
INTERPOSE(ksurface_user_gethostname, gethostname);
INTERPOSE(ksurface_user_sethostname, sethostname);

static int ksurface_user_sysctl(int *name,
                                u_int namelen,
                                void *__sized_by(*oldlenp) oldp,
                                size_t *oldlenp,
                                void *__sized_by(newlen) newp,
                                size_t newlen)
{
    int ret = (int)liveshim_syscall(SYS_sysctl, name, namelen, oldp, oldlenp, newp, newlen);
    int (*darwin_user_sysctl)(int *name, u_int namelen, void *__sized_by(*oldlenp) oldp,size_t *oldlenp, void *__sized_by(newlen) newp, size_t newlen) = _interpose_sysctl.replacee;
    return (ret == -1 && errno == ENOSYS) ? darwin_user_sysctl(name, namelen, oldp, oldlenp, newp, newlen) : ret;
}

static int ksurface_user_gethostname(char *name,
                                     size_t len)
{
    int mib[2] = { CTL_KERN, KERN_HOSTNAME };
    int retval = (int)liveshim_syscall(SYS_sysctl, mib, 2, name, &len, NULL, NULL);
    name[len] = '\0';
    return retval;
}

static int ksurface_user_sethostname(char *name,
                                     size_t len)
{
    int mib[2] = { CTL_KERN, KERN_HOSTNAME };
    return (int)liveshim_syscall(SYS_sysctl, mib, 2, NULL, NULL, name, len);
}

#endif /* LIVESHIM_SYSCTL_ENABLED */
