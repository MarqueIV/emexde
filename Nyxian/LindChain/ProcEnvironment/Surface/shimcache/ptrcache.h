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

#ifndef PTRCACHE_H
#define PTRCACHE_H

#include <stdint.h>
#include <mach/kern_return.h>

#define NYX_DYLD_BLOB_MAGIC 0x4E595844594C4431ULL   /* "NYXDYLD1" */

enum {
    kDyldPtrOpen = 0,
    kDyldPtrFcntl,
    kDyldPtrFstat64,
    kDyldPtrStat64,
    kDyldPtrOpenat,
    kDyldPtrCount
};

typedef struct {
    uint64_t magic;
    
    uint32_t version;
    uint32_t size;
    
    uint64_t open;
    uint64_t fcntl;
    uint64_t fstat64;
    uint64_t stat64;
    uint64_t openat;
} nyx_ptr_cache_blob_t;

kern_return_t ksurface_ptrcache_emit(void);

#endif /* PTRCACHE_H */
