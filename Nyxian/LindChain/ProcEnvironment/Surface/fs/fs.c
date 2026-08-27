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

#include <LindChain/ProcEnvironment/Utils/klog.h>
#include <LindChain/ProcEnvironment/Surface/fs/fs.h>
#include <LindChain/ProcEnvironment/Surface/fs/preserver.h>
#include <mach/mach.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char ksurface_fs_mntfs_root[PATH_MAX];
char ksurface_fs_devfs_root[PATH_MAX];
char ksurface_fs_rootfs_root[PATH_MAX];
char ksurface_fs_rootfs_mount_root[PATH_MAX];
char ksurface_fs_rootfs_dev_mount_root[PATH_MAX];

kern_return_t ksurface_fs_init(void)
{
    const char *home = getenv("HOME");
    if(!home)
    {
        klog_log("ksurface:fs", "HOME unset");
        return KERN_FAILURE;
    }
    
    klog_log("ksurface:fs", "initializing mntfs");
    
    snprintf(ksurface_fs_mntfs_root, PATH_MAX, "%s/Documents/mntfs", home);
    snprintf(ksurface_fs_rootfs_root, PATH_MAX, "%s/Documents/rootfs", home);
    snprintf(ksurface_fs_devfs_root, PATH_MAX, "%s/devfs", ksurface_fs_mntfs_root);
    snprintf(ksurface_fs_rootfs_mount_root, PATH_MAX, "%s/rootfs", ksurface_fs_mntfs_root);
    snprintf(ksurface_fs_rootfs_dev_mount_root, PATH_MAX, "%s/dev", ksurface_fs_rootfs_mount_root);
    
    const FSPreserverDesc layout[] = {
        { kFSNodeTypeDirectory, ksurface_fs_mntfs_root, NULL },
        { kFSNodeTypeDirectory, ksurface_fs_devfs_root, NULL },
        { kFSNodeTypeDirectory, ksurface_fs_rootfs_root, NULL },
        { kFSNodeTypeSymbolicLink, ksurface_fs_rootfs_mount_root, ksurface_fs_rootfs_root },
        { kFSNodeTypeSymbolicLink, ksurface_fs_rootfs_dev_mount_root, ksurface_fs_devfs_root },
    };
    
    size_t bad;
    kern_return_t kr = ksurface_fs_preserver_add_nodes(layout, sizeof layout / sizeof layout[0], &bad);
    if(kr != KERN_SUCCESS)
    {
        klog_log("ksurface:fs", "layout entry %zu rejected (%s): %s", bad, mach_error_string(kr), layout[bad].name);
        return KERN_FAILURE;
    }
    klog_log("ksurface:fs", "layout registered [ok] (%zu nodes)", sizeof layout / sizeof layout[0]);
    
    kr = ksurface_fs_preserver_kickstart();
    if(kr != KERN_SUCCESS)
    {
        klog_log("ksurface:fs", "failed to start preserver");
        return KERN_FAILURE;
    }
    
    return kr == KERN_SUCCESS ? KERN_SUCCESS : KERN_FAILURE;
}
