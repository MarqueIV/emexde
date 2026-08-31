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

#include <LindChain/ProcEnvironment/Surface/kxld/kxopen.h>
#include <LindChain/ProcEnvironment/Surface/trust/signing.h>
#include <LindChain/ProcEnvironment/LiveContainer/LCMachOUtils.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <mach-o/loader.h>
#include <mach-o/ldsyms.h>

extern struct code_signature_command* findSignatureCommand(struct mach_header_64* header);

struct code_signature_command {
    uint32_t    cmd;
    uint32_t    cmdsize;
    uint32_t    dataoff;
    uint32_t    datasize;
};

// from zsign
struct ui_CS_BlobIndex {
    uint32_t type;                    /* type of entry */
    uint32_t offset;                /* offset of entry */
};

struct ui_CS_SuperBlob {
    uint32_t magic;                    /* magic number */
    uint32_t length;                /* total length of SuperBlob */
    uint32_t count;                    /* number of index entries following */
    //CS_BlobIndex index[];            /* (count) entries */
    /* followed by Blobs in no particular order as indicated by offsets in index */
};

struct ui_CS_blob {
    uint32_t magic;
    uint32_t length;
};

void *kxopen(const char *path,
             int mode)
{
    int fd = open(path, O_RDWR);
    if(fd < 0)
    {
        return NULL;
    }
    
    void *handle = kxopen_with_fd(fd, mode);
    close(fd);
    return handle;
}

void *kxopen_with_fd(int fd,
                     int mode)
{
    if(fd < 0)
    {
        errno = EINVAL;
        return NULL;
    }
    
    /* map machO */
    LCMachO *machO = LCMapMachOFromFDRO(fd);
    if(machO == NULL)
    {
        return NULL;
    }
    
    if(machO->header->filetype != MH_BUNDLE)
    {
        errno = ENOEXEC;
        LCUnmapMachO(machO);
        return NULL;
    }
    
    /* XNU code signature validation (does the kernel like us ^^) */
    if(machO->header->cputype != CPU_TYPE_ARM64)
    {
        errno = ENOEXEC;
        LCUnmapMachO(machO);
        return NULL;
    }
    
    /* binary must be signed, otherwise no execution */
    struct code_signature_command* codeSignatureCommand = findSignatureCommand(machO->header);
    if(!codeSignatureCommand)
    {
        errno = EPERM;
        LCUnmapMachO(machO);
        return NULL;
    }
    
    /* checking if the kernel says this is signed */
    off_t sliceOffset = (void*)machO->header - machO->map;
    fsignatures_t siginfo;
    siginfo.fs_file_start = sliceOffset;
    siginfo.fs_blob_start = (void*)(long)(codeSignatureCommand->dataoff);
    siginfo.fs_blob_size = codeSignatureCommand->datasize;
    int addFileSigsReault = fcntl(machO->fd, F_ADDFILESIGS_RETURN, &siginfo);
    if(addFileSigsReault == -1)
    {
        errno = EPERM;
        LCUnmapMachO(machO);
        return NULL;
    }
    
    /* checking if this can be executed by us */
    fchecklv_t checkInfo;
    checkInfo.lv_error_message_size = 0;
    checkInfo.lv_error_message = NULL;
    checkInfo.lv_file_start= sliceOffset;
    int checkLVresult = fcntl(machO->fd, F_CHECK_LV, &checkInfo);
    if(checkLVresult != 0)
    {
        errno = EPERM;
        LCUnmapMachO(machO);
        return NULL;
    }
    
    /* how much memory does this kext need? */
    uintptr_t vmStart = UINT64_MAX;
    uintptr_t vmEnd = 0;
    const uint8_t *ptr = ((const uint8_t *)machO->header) + sizeof(struct mach_header_64);
    uint64_t ncmds = machO->header->ncmds;
    for(uint32_t i = 0; i < ncmds; i++)
    {
        const struct segment_command_64 *sc = (const struct segment_command_64 *)ptr;
        if(sc->cmd == LC_SEGMENT_64)
        {
            if(sc->vmsize == 0)
            {
                continue;
            }
            vmStart = MIN(vmStart, sc->vmaddr);
            vmEnd = MAX(vmEnd, sc->vmaddr + sc->vmsize);
        }
        ptr += sc->cmdsize;
    }
    size_t totalSize = vmEnd - vmStart;
    
    /* allocating the memory needed by the segments of the kext (aka address space reservation) */
    void *base = mmap(NULL, totalSize, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if(base == MAP_FAILED)
    {
        LCUnmapMachO(machO);
        return NULL;
    }
    
    /* calculating slide of kext */
    intptr_t kext_slide = (intptr_t)base - (intptr_t)vmStart;
    
    /* now mapping executable memory on iOS the valid way */
    ptr = ((const uint8_t *)machO->header) + sizeof(struct mach_header_64);
    for(uint32_t i = 0; i < ncmds; i++)
    {
        const struct segment_command_64 *sc = (const struct segment_command_64 *)ptr;
        if(sc->cmd == LC_SEGMENT_64)
        {
            if(sc->vmsize == 0)
            {
                continue;
            }
            
            /* now a lot of math ^^ */
            void *addr = (void *)(sc->vmaddr + kext_slide);
            off_t fileOff = sliceOffset + sc->fileoff;
            int prot = 0;
            if(sc->initprot & VM_PROT_READ)
            {
                prot |= PROT_READ;
            }
            if(sc->initprot & VM_PROT_WRITE)
            {
                prot |= PROT_WRITE;
            }
            if(sc->initprot & VM_PROT_EXECUTE)
            {
                prot |= PROT_EXEC;
            }
            
            int flags = (sc->initprot & VM_PROT_WRITE) ? (MAP_PRIVATE | MAP_FIXED) : (MAP_SHARED  | MAP_FIXED);
            
            /* the everything part */
            if(sc->filesize > 0)
            {
                void *r = mmap(addr, sc->filesize, prot, flags, machO->fd, fileOff);
                if(r == MAP_FAILED)
                {
                    LCUnmapMachO(machO);
                    return NULL;
                }
            }
            
            /* the bss part */
            size_t pageSize = vm_page_size;
            if(sc->vmsize > sc->filesize)
            {
                /* I love tails >~< */
                uintptr_t fileEnd = (uintptr_t)addr + sc->filesize;
                uintptr_t bssStart = (fileEnd + pageSize - 1) & ~(pageSize - 1);
                uintptr_t bssEnd = (uintptr_t)addr + sc->vmsize;
                bssEnd = (bssEnd + pageSize - 1) & ~(pageSize - 1);
                
                if(bssEnd > bssStart)
                {
                    void *r = mmap((void *)bssStart, bssEnd - bssStart, prot, MAP_PRIVATE | MAP_FIXED | MAP_ANON, -1, 0);
                    if(r == MAP_FAILED)
                    {
                        LCUnmapMachO(machO);
                        return NULL;
                    }
                }
            }
        }
        ptr += sc->cmdsize;
    }
    
    /* own linker is WIP */
    errno = ENOTSUP;
    return NULL;
}
