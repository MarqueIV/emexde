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

#ifndef SURFACE_VFS_VFS_H
#define SURFACE_VFS_VFS_H

#include <fcntl.h>

/* the mount where the root of our VFS will live */
#define VFS_MOUNT_PREFIX        "/Developer"
#define VFS_MOUNT_PREFIX_LEN    (sizeof(VFS_MOUNT_PREFIX) - 1)

#define KSURFACE_OPEN_FLAG_MASK                                     \
    (O_ACCMODE | O_APPEND | O_CREAT | O_TRUNC  | O_EXCL |           \
     O_NONBLOCK | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC |            \
     O_SYMLINK | O_EVTONLY)

const char *vfs_match_mount(const char *path);
bool vfs_resolve_rel(const char *in, char *out, size_t outsz);
int vfs_root_fd(void);

#endif /* SURFACE_VFS_VFS_H */
