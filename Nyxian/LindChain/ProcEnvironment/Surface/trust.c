/*
 SPDX-License-Identifier: AGPL-3.0-or-later

 Copyright (C) 2025 - 2026 emexlab
 Copyright (C) 2026 semvis123

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

#import <LindChain/ProcEnvironment/Surface/trust.h>
#import <LindChain/ProcEnvironment/Surface/entitlement.h>
#import <LindChain/ProcEnvironment/LiveContainer/LCMachOUtils.h>
#include <LindChain/ProcEnvironment/Surface/cdhash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <sys/stat.h>
#import <CommonCrypto/CommonCrypto.h>
#import <mach-o/loader.h>
#import <mach-o/fat.h>

#define APPEND_TAG "NXTR"

ssize_t read_at(int fd, off_t offset, void *buf, size_t len)
{
    if(lseek(fd, offset, SEEK_SET) < 0)
    {
        return -1;
    }
    
    return read(fd, buf, len);
}

int macho_after_sign(const char *path,
                     PEEntitlement entitlement)
{
    int fd = open(path, O_RDWR);
    if(fd < 0)
    {
        perror("open");
        return -1;
    }
    
    int retval = macho_after_sign_fd(fd, entitlement);
    fsync(fd);
    close(fd);
    
    return retval;
}

int macho_after_sign_fd(int fd, PEEntitlement entitlement)
{
    LCMachO *machO = LCMapMachOFromFDRO(dup(fd));
    if(machO == NULL)
    {
        return -1;
    }
    char *cdhash = cdhash_of_hdr((const uint8_t*)machO->header, machO->size);
    LCUnmapMachO(machO);
    
    ksurface_ent_blob_t token;
    if(entitlement_token_mach_gen(&token, cdhash, entitlement) != KERN_SUCCESS)
    {
        free(cdhash);
        return -1;
    }
    free(cdhash);

    char tag[4];
    off_t eof = lseek(fd, 0, SEEK_END);
    
    if(eof >= (off_t)(sizeof(ksurface_ent_blob_t) + sizeof(uint32_t) + 4))
    {
        read_at(fd, eof - 4, tag, 4);
        if(memcmp(tag, APPEND_TAG, 4) == 0)
        {
            uint32_t data_len;
            read_at(fd, eof - 4 - sizeof(uint32_t), &data_len, sizeof(uint32_t));
            eof -= (off_t)(data_len + sizeof(uint32_t) + 4);
            ftruncate(fd, eof);
        }
    }

    if(lseek(fd, eof, SEEK_SET) < 0)
    {
        return -1;
    }

    if(write(fd, &token, sizeof(ksurface_ent_blob_t)) != (ssize_t)sizeof(ksurface_ent_blob_t))
    {
        return -1;
    }

    size_t data_len = sizeof(ksurface_ent_blob_t);
    if(write(fd, &data_len, sizeof(uint32_t)) != sizeof(uint32_t))
    {
        return -1;
    }
    if(write(fd, APPEND_TAG, 4) != 4)
    {
        return -1;
    }

    return 0;
}

int macho_read_token(int fd,
                     ksurface_ent_result_t *mach)
{
    bzero(mach, sizeof(ksurface_ent_result_t));
    
    char tag[4];
    uint32_t len;
    
    if(lseek(fd, -4, SEEK_END) < 0)
    {
        return -1;
    }
    if(read(fd, tag, 4) != 4)
    {
        return -1;
    }
    
    if(memcmp(tag, APPEND_TAG, 4) != 0)
    {
        return -1;
    }
    
    if(lseek(fd, -8, SEEK_END) < 0)
    {
        return -1;
    }
    if(read(fd, &len, sizeof(uint32_t)) != sizeof(uint32_t))
    {
        return -1;
    }
    
    if(lseek(fd, -(off_t)(8 + len), SEEK_END) < 0)
    {
        return -1;
    }
    
    if(len != sizeof(ksurface_ent_blob_t))
    {
        return -1;
    }
    
    if(read(fd, &(mach->blob), len) != (ssize_t)len)
    {
        return -1;
    }
    
    LCMachO *machO = LCMapMachOFromFDRO(dup(fd));
    if(machO == NULL)
    {
        return -1;
    }
    char *hash = cdhash_of_hdr((const uint8_t*)machO->header, machO->size);
    LCUnmapMachO(machO);
    
    if(hash == NULL)
    {
        mach->cdhash_valid = false;
        goto out_no_cdhas;
    }
    else if(strncmp(hash, mach->blob.cdhash, USER_FSIGNATURES_CDHASH_LEN) == 0)
    {
        free(hash);
        mach->cdhash_valid = true;
    out_no_cdhas:
        return 0;
    }
    
    free(hash);
    return -1;
}
