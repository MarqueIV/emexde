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

#import <Foundation/Foundation.h>
#import <LindChain/ProcEnvironment/Shims/environment.h>
#import <LiveShim/LiveShimSyscall.h>
#import <LindChain/ProcEnvironment/Shims/proxy.h>
#import <LindChain/ProcEnvironment/Shims/libproc.h>
#import <LindChain/ProcEnvironment/litehook/litehook.h>
#include <LindChain/ProcEnvironment/Surface/extra/xnubits/proc_info.h>
#import <LindChain/ProcEnvironment/Surface/surface.h>
#import <LindChain/ProcEnvironment/Surface/proc/proc.h>
#import <LindChain/ProcEnvironment/Surface/extra/xnubits/proc_info.h>
#import <ksurface_config.h>
#import <ksurface_abi.h>

#if KSURFACE_SYS_PROC_ENABLED

DEFINE_HOOK(proc_pidinfo, int, (pid_t pid,
                                int flavor,
                                uint64_t arg,
                                void * buffer,
                                int buffersize))
{
    errno = 0;
    int ret = (int)liveshim_syscall(SYS_proc_info, PROC_INFO_CALL_PIDINFO, pid, flavor, 0, buffer, buffersize);
    if(errno == ENOSYS)
    {
        return ORIG_FUNC(proc_pidinfo)(pid, flavor, arg, buffer, buffersize);
    }
    return ret;
}

DEFINE_HOOK(proc_name, int, (pid_t pid,
                             void *buffer,
                             uint32_t buffersize))
{
    struct proc_bsdinfo pbsd;
    if(buffersize < sizeof(pbsd.pbi_name))
    {
        errno = ENOMEM;
        return 0;
    }
    
    int retval = HOOK_FUNC(proc_pidinfo)(pid, PROC_PIDTBSDINFO, 0,  &pbsd, sizeof(pbsd));
    if(retval != 0)
    {
        if(pbsd.pbi_name[0])
        {
            bcopy(&pbsd.pbi_name, buffer, sizeof(pbsd.pbi_name));
        }
        else
        {
            bcopy(&pbsd.pbi_comm, buffer, sizeof(pbsd.pbi_comm));
        }
        return (int)strlen(buffer);
    }
    return 0;
}

DEFINE_HOOK(proc_pidpath, int, (pid_t pid,
                                void *buffer,
                                uint32_t buffersize))
{
    /* sanity check */
    if(buffersize == 0 || buffer == NULL)
    {
        return 0;
    }
    
    /* syscall with SYS_PROCPATH */
    int retval = HOOK_FUNC(proc_pidinfo)(pid, PROC_PIDPATHINFO, 0, buffer, buffersize);
    if(retval != 0)
    {
        return 0;
    }
    
    /* final return of lenght */
    return (int)strlen((char*)buffer);
}

DEFINE_HOOK(proc_listallpids, int, (void *buffer,
                                    int buffersize))
{
    if(buffersize < 0)
    {
        errno = EINVAL;
        return -1;
    }
    
    kinfo_proc_t kp[PROC_MAX];
    uint32_t len = sizeof(kp);
    
    int mib[3] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
    liveshim_syscall(SYS_sysctl, mib, 3, &kp, &len);
    
    size_t count = (uint32_t)(len / sizeof(kinfo_proc_t));
    
    size_t n = 0;
    size_t needed_bytes = 0;
    
    needed_bytes = (size_t)count * sizeof(pid_t);
    
    if(buffer != NULL && buffersize > 0)
    {
        size_t capacity = (size_t)buffersize / sizeof(pid_t);
        n = count < capacity ? count : capacity;
        
        pid_t *pids = (pid_t *)buffer;
        
        for(size_t i = 0; i < n; i++)
        {
            pids[i] = kp[i].kp_proc.p_pid;
        }
    }
    
    if(buffer == NULL || buffersize == 0)
    {
        return (int)needed_bytes;
    }
    
    return (int)(n * sizeof(pid_t));
}

DEFINE_HOOK(proc_pid_rusage, int, (int pid,
                                   int flavor,
                                   rusage_info_t *buffer))
{
    int retval = (int)liveshim_syscall(SYS_proc_info, PROC_INFO_CALL_PIDRUSAGE, pid, (uint32_t)flavor, (uint64_t)0, buffer, 0);
    if(retval != 0)
    {
        return proc_pid_rusage(pid, flavor, (struct rusage_info_v2*)buffer);
    }
    return retval;
}

DEFINE_HOOK(kill, int, (pid_t pid, int sig))
{
    return (int)liveshim_syscall(SYS_kill, pid, sig);
}

DEFINE_HOOK(raise, int, (int sig))
{
    return HOOK_FUNC(kill)(getpid(), sig);
}

void environment_libproc_init(void)
{
    DO_HOOK_GLOBAL(proc_pidinfo);
    DO_HOOK_GLOBAL(proc_name);
    DO_HOOK_GLOBAL(proc_pidpath);
    DO_HOOK_GLOBAL(proc_listallpids);
    DO_HOOK_GLOBAL(proc_pid_rusage);
    DO_HOOK_GLOBAL(kill);
    DO_HOOK_GLOBAL(raise);
}

#endif /* KSURFACE_SYS_PROC_ENABLED */
