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

#import <LindChain/ProcEnvironment/Surface/cdhash.h>
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

#define CSMAGIC_EMBEDDED_SIGNATURE      0xfade0cc0
#define CSMAGIC_CODEDIRECTORY           0xfade0c02
#define CSSLOT_CODEDIRECTORY            0
#define CS_HASHTYPE_SHA256              2
#define CS_HASHTYPE_SHA256_TRUNCATED    3

typedef struct __BlobIndex {
    uint32_t type;
    uint32_t offset;
} CS_BlobIndex;

typedef struct __SuperBlob {
    uint32_t magic;
    uint32_t length;
    uint32_t count;
    CS_BlobIndex index[];
} CS_SuperBlob;

typedef struct __CodeDirectory {
    uint32_t magic;
    uint32_t length;
    uint32_t version;
    uint32_t flags;
    uint32_t hashOffset;
    uint32_t identOffset;
    uint32_t nSpecialSlots;
    uint32_t nCodeSlots;
    uint32_t codeLimit;
    uint8_t  hashSize;
    uint8_t  hashType;
    uint8_t  platform;
    uint8_t  pageSize;
    uint32_t spare2;
    // v0x20200+
    uint32_t scatterOffset;
    uint32_t teamOffset;
    // v0x20300+
    uint32_t spare3;
    uint64_t codeLimit64;
    // v0x20400+
    uint64_t execSegBase;
    uint64_t execSegLimit;
    uint64_t execSegFlags;
} CS_CodeDirectory;

char *cdhash_of_loaded_image(const struct mach_header *mh)
{
    if(!mh || mh->magic != MH_MAGIC_64)
    {
        return NULL;
    }

    uint32_t ncmds = ((const struct mach_header_64 *)mh)->ncmds;
    const uint8_t *p = (const uint8_t *)mh + sizeof(struct mach_header_64);

    uint64_t text_vmaddr = 0, le_vmaddr = 0, le_fileoff = 0, le_filesize = 0;
    uint32_t sig_off = 0, sig_size = 0;
    bool have_text = false, have_le = false, have_sig = false;

    for(uint32_t i = 0; i < ncmds; i++)
    {
        const struct load_command *lc = (const struct load_command *)p;

        if(lc->cmd == LC_SEGMENT_64)
        {
            const struct segment_command_64 *seg = (const struct segment_command_64 *)p;
            if(strncmp(seg->segname, SEG_TEXT, 16) == 0)
            {
                text_vmaddr = seg->vmaddr; have_text = true;
            }
            else if(strncmp(seg->segname, SEG_LINKEDIT, 16) == 0)
            {
                le_vmaddr = seg->vmaddr; le_fileoff = seg->fileoff;
                le_filesize = seg->filesize; have_le = true;
            }
        }
        else if(lc->cmd == LC_CODE_SIGNATURE)
        {
            const struct linkedit_data_command *sc = (const struct linkedit_data_command *)p;
            sig_off = sc->dataoff; sig_size = sc->datasize; have_sig = true;
        }
        p += lc->cmdsize;
    }
    if(!have_text || !have_le || !have_sig)
    {
        return NULL;
    }

    if(sig_off < le_fileoff || (uint64_t)sig_off + sig_size > le_fileoff + le_filesize)
    {
        return NULL;
    }

    uintptr_t slide = (uintptr_t)mh - (uintptr_t)text_vmaddr;
    const uint8_t *le_base = (const uint8_t *)(slide + le_vmaddr - le_fileoff);
    const CS_SuperBlob *sb = (const CS_SuperBlob *)(le_base + sig_off);

    if(OSSwapBigToHostInt32(sb->magic) != CSMAGIC_EMBEDDED_SIGNATURE)
    {
        return NULL;
    }

    uint32_t count = OSSwapBigToHostInt32(sb->count);
    for(uint32_t j = 0; j < count; j++)
    {
        if(OSSwapBigToHostInt32(sb->index[j].type) != CSSLOT_CODEDIRECTORY)
        {
            continue;
        }

        uint32_t off = OSSwapBigToHostInt32(sb->index[j].offset);
        const CS_CodeDirectory *cd = (const CS_CodeDirectory *)((const uint8_t *)sb + off);
        if(OSSwapBigToHostInt32(cd->magic) != CSMAGIC_CODEDIRECTORY)
        {
            return NULL;
        }

        uint32_t cd_len = OSSwapBigToHostInt32(cd->length);
        unsigned char digest[CC_SHA256_DIGEST_LENGTH];

        if(cd->hashType == CS_HASHTYPE_SHA256 || cd->hashType == CS_HASHTYPE_SHA256_TRUNCATED)
        {
            CC_SHA256(cd, cd_len, digest);
        }
        else
        {
            CC_SHA1(cd, cd_len, digest);
        }

        char *result = malloc(USER_FSIGNATURES_CDHASH_LEN);
        if(!result)
        {
            return NULL;
        }
        memcpy(result, digest, USER_FSIGNATURES_CDHASH_LEN);
        return result;
    }
    return NULL;
}
