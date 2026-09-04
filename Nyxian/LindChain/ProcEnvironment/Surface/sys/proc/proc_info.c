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

#include <LindChain/ProcEnvironment/Surface/sys/proc/proc_info.h>

DEFINE_SYSCALL_HANDLER(proc_info_listpids)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_pidinfo)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_pidfdinfo)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_kernmsgbuf)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_setcontrol)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_pidfileportinfo)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_terminate)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_dirtycontrol)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_pidrusage)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_pidoriginatorinfo)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_listcoalitions)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_canusefghw)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_piddynkqueueinfo)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info_udata_info)
{
    sys_return_failure_with_errno(ENOSYS);
}

DEFINE_SYSCALL_HANDLER(proc_info)
{
    /* parse arguments */
    int32_t u_callnum = (int32_t)args[0];
    pid_t u_pid = (pid_t)args[1];
    uint32_t u_flavour = (uint32_t)args[2];
    uint64_t u_arg = (uint64_t)args[3];
    userspace_pointer_t u_buffer = (userspace_pointer_t)args[4];
    int32_t buffersize = (int32_t)args[5];
    
    switch(u_callnum)
    {
        case PROC_INFO_CALL_LISTPIDS:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_listpids);
        case PROC_INFO_CALL_PIDINFO:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_pidinfo);
        case PROC_INFO_CALL_PIDFDINFO:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_pidfdinfo);
        case PROC_INFO_CALL_KERNMSGBUF:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_kernmsgbuf);
        case PROC_INFO_CALL_SETCONTROL:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_setcontrol);
        case PROC_INFO_CALL_PIDFILEPORTINFO:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_pidfileportinfo);
        case PROC_INFO_CALL_TERMINATE:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_terminate);
        case PROC_INFO_CALL_DIRTYCONTROL:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_dirtycontrol);
        case PROC_INFO_CALL_PIDRUSAGE:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_pidrusage);
        case PROC_INFO_CALL_PIDORIGINATORINFO:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_pidoriginatorinfo);
        case PROC_INFO_CALL_LISTCOALITIONS:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_listcoalitions);
        case PROC_INFO_CALL_CANUSEFGHW:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_canusefghw);
        case PROC_INFO_CALL_PIDDYNKQUEUEINFO:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_piddynkqueueinfo);
        case PROC_INFO_CALL_UDATA_INFO:
            return SYSCALL_HANDLER_REDIRECT_TO_HANDLER(proc_info_udata_info);
        default:
            sys_return_failure_with_errno(EINVAL);
    }
    
    sys_return;
}
