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

#include <CoreFoundation/CoreFoundation.h>
#include <LindChain/ProcEnvironment/Surface/vfs/vfs.h>
#include <string.h>

const char *vfs_match_mount(const char *path)
{
    if(strncmp(path, VFS_MOUNT_PREFIX, VFS_MOUNT_PREFIX_LEN) != 0)
    {
        return NULL;
    }
    
    char c = path[VFS_MOUNT_PREFIX_LEN];
    if(c != '\0' && c != '/')
    {
        return NULL;
    }
    
    return path + VFS_MOUNT_PREFIX_LEN;
}

bool vfs_resolve_rel(const char *in,
                     char *out,
                     size_t outsz)
{
    size_t off = 0;
    if(outsz == 0)
    {
        return false;
    }
    out[0] = '\0';
    
    for(const char *p = in; *p != '\0'; )
    {
        while(*p == '/')
        {
            p++;
        }
        if(*p == '\0')
        {
            break;
        }
        
        const char *start = p;
        while(*p != '\0' && *p != '/')
        {
            p++;
        }
        size_t len = (size_t)(p - start);
        
        if(len == 1 && start[0] == '.')
        {
            continue;
        }
        
        if(len == 2 && start[0] == '.' && start[1] == '.')
        {
            if(off == 0)
            {
                return false;
            }
            while(off > 0 && out[off - 1] != '/')
            {
                off--;
            }
            if(off > 0) off--;
            out[off] = '\0';
            continue;
        }
        
        size_t need = off + (off != 0) + len + 1;
        if(need > outsz)
        {
            return false;
        }
        if(off != 0)
        {
            out[off++] = '/';
        }
        memcpy(out + off, start, len);
        off += len;
        out[off] = '\0';
    }
    
    if(off == 0)
    {
        if(outsz < 2)
        {
            return false;
        }
        out[0] = '.';
        out[1] = '\0';
    }
    return true;
}

int vfs_root_fd(void)
{
    static int rootfd = -1;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        const char *root = getenv("VFSROOT");
        if(root != NULL)
        {
            rootfd = open(root, O_DIRECTORY | O_RDONLY | O_CLOEXEC);
        }
    });
    return rootfd;
}
