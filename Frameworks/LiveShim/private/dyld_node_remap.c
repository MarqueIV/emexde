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

#include <LiveShim/dyld_node_remap.h>
#include <pthread.h>
#include <string.h>

static DyldInodeEntry g_bank[INODE_BANK_CAPACITY];
static pthread_rwlock_t g_bank_lock = PTHREAD_RWLOCK_INITIALIZER;
static bool g_initialized = false;

static inline uint32_t hash_ino(ino_t ino)
{
    uint64_t x = (uint64_t)ino;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (uint32_t)(x % INODE_BANK_CAPACITY);
}

ino_t inode_for_fd(int fd)
{
    struct stat buf;
    if(fstat(fd, &buf) != 0)
    {
        return 0x0;
    }
    return buf.st_ino;
}

void inode_bank_init(void)
{
    pthread_rwlock_wrlock(&g_bank_lock);
    if(!g_initialized)
    {
        memset(g_bank, 0, sizeof(g_bank));
        g_initialized = true;
    }
    pthread_rwlock_unlock(&g_bank_lock);
}

void inode_bank_put(ino_t ino,
                    const char *real_path)
{
    if(!ino || !real_path)
    {
        return;
    }
    
    pthread_rwlock_wrlock(&g_bank_lock);
    
    uint32_t idx = hash_ino(ino);
    uint32_t start_idx = idx;
    
    while(g_bank[idx].in_use)
    {
        if(g_bank[idx].ino == ino)
        {
            strncpy(g_bank[idx].real_path, real_path, PATH_MAX - 1);
            pthread_rwlock_unlock(&g_bank_lock);
            return;
        }
        idx = (idx + 1) % INODE_BANK_CAPACITY;
        if(idx == start_idx)
        {
            pthread_rwlock_unlock(&g_bank_lock);
            return;
        }
    }
    
    g_bank[idx].ino = ino;
    g_bank[idx].in_use = true;
    strncpy(g_bank[idx].real_path, real_path, PATH_MAX - 1);
    g_bank[idx].redirect_path[0] = '\0';
    
    pthread_rwlock_unlock(&g_bank_lock);
}

void inode_bank_set_redirect(ino_t ino,
                             const char *redirect_path)
{
    if(!ino || !redirect_path)
    {
        return;
    }
    
    pthread_rwlock_wrlock(&g_bank_lock);
    
    uint32_t idx = hash_ino(ino);
    uint32_t start_idx = idx;
    
    while(g_bank[idx].in_use)
    {
        if(g_bank[idx].ino == ino)
        {
            strncpy(g_bank[idx].redirect_path, redirect_path, PATH_MAX - 1);
            pthread_rwlock_unlock(&g_bank_lock);
            return;
        }
        idx = (idx + 1) % INODE_BANK_CAPACITY;
        if(idx == start_idx)
        {
            break;
        }
    }
    
    pthread_rwlock_unlock(&g_bank_lock);
}

bool inode_bank_get_path(ino_t ino,
                         char *out_path,
                         size_t max_len)
{
    if(!ino || !out_path)
    {
        return false;
    }
    
    pthread_rwlock_rdlock(&g_bank_lock);
    
    uint32_t idx = hash_ino(ino);
    uint32_t start_idx = idx;
    
    while(g_bank[idx].in_use)
    {
        if(g_bank[idx].ino == ino)
        {
            const char *target = (g_bank[idx].redirect_path[0] != '\0') ? g_bank[idx].redirect_path : g_bank[idx].real_path;
            strncpy(out_path, target, max_len - 1);
            out_path[max_len - 1] = '\0';
            pthread_rwlock_unlock(&g_bank_lock);
            return true;
        }
        idx = (idx + 1) % INODE_BANK_CAPACITY;
        if(idx == start_idx)
        {
            break;
        }
    }
    
    pthread_rwlock_unlock(&g_bank_lock);
    return false;
}

bool inode_bank_get_ino_by_path(const char *path,
                                ino_t *out_ino)
{
    if(!path || !out_ino)
    {
        return false;
    }
    
    pthread_rwlock_rdlock(&g_bank_lock);
    
    for(int i = 0; i < INODE_BANK_CAPACITY; i++)
    {
        if(g_bank[i].in_use)
        {
            if(strcmp(g_bank[i].real_path, path) == 0 ||
               strcmp(g_bank[i].redirect_path, path) == 0)
            {
                *out_ino = g_bank[i].ino;
                pthread_rwlock_unlock(&g_bank_lock);
                return true;
            }
        }
    }
    
    pthread_rwlock_unlock(&g_bank_lock);
    return false;
}
