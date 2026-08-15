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

#include <LindChain/ProcEnvironment/Surface/sys/fs/open.h>
#include <LindChain/Private/mach/fileport.h>

DEFINE_SYSCALL_HANDLER(open)
{
    userspace_pointer_t user_path = (userspace_pointer_t)args[0];
    int flags = (int)args[1];
    mode_t mode = (mode_t)args[2];
    
    char *path = mach_syscall_copy_str_in(sys_task_, user_path, MAXPATHLEN);
    if(path == NULL)
    {
        printf("illegal access = %p\n", user_path);
        sys_return_failure(EFAULT);
    }
    
    if(strncmp(path, "/Developer", MAXPATHLEN) != 0)
    {
        sys_return_failure(ENOSYS);
    }
    
    printf("[ksurface:open] triggered /Developer virtual mount\n");
    int fd = open(getenv("VFSROOT"), O_DIRECTORY | O_RDONLY);
    if(fd < 0)
    {
        sys_return_failure(ENOSYS);
    }
    
    fileport_t fileport;
    if(fileport_makeport(fd, &fileport) != 0)
    {
        close(fd);
        sys_return_failure(ENOSYS);
    }
    close(fd);
    
    kern_return_t kr = mach_syscall_payload_create(NULL, sizeof(mach_port_t), (vm_address_t*)out_ports);
    if(kr != KERN_SUCCESS)
    {
        mach_port_deallocate(mach_task_self(), fileport);
        sys_return_failure(ENOSYS);
    }
    
    (*out_ports)[0] = fileport;
    *out_ports_cnt = 1;
    
    sys_return;
}
