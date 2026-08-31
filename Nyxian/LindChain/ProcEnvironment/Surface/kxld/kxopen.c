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
#include <LindChain/ProcEnvironment/Surface/kxld/validation.h>
#include <LindChain/ProcEnvironment/Surface/kxld/mapper.h>
#include <LindChain/ProcEnvironment/Surface/kxld/fixup.h>
#include <LindChain/ProcEnvironment/Surface/kxld/reseal.h>
#include <LindChain/ProcEnvironment/Surface/kxld/image.h>
#include <LindChain/ProcEnvironment/Surface/kxld/kmod.h>
#include <LindChain/ProcEnvironment/Surface/kxld/export.h>
#include <LindChain/ProcEnvironment/Surface/kxld/init.h>
#include <LindChain/ProcEnvironment/Surface/kxld/objc.h>
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

static void kxdestroy_image(kxld_image_info_t *image_info)
{
    if(image_info->base != NULL)
    {
        munmap(image_info->base, image_info->len);
    }
    free(image_info);
}

kxld_image_info_t *kxopen(const char *path,
                          int mode)
{
    int fd = open(path, O_RDWR);
    if(fd < 0)
    {
        return NULL;
    }
    
    kxld_image_info_t *image_info = kxopen_with_fd(fd, mode);
    close(fd);
    return image_info;
}

kxld_image_info_t *kxopen_with_fd(int fd,
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
    
    /* validating header of kext */
    if(machO->header->filetype != MH_BUNDLE ||
       machO->header->cputype != CPU_TYPE_ARM64)
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
    
    /* checking if the kernel says(double meaning x3) this is signed */
    if(!KXValidateCodeSignature(machO))
    {
        /* sets errno */
        LCUnmapMachO(machO);
        return NULL;
    }
    
    kxld_image_info_t *image_info = calloc(1, sizeof(kxld_image_info_t));
    if(image_info == NULL)
    {
        errno = ENOMEM;
        LCUnmapMachO(machO);
        return NULL;
    }
    
    if(fcntl(machO->fd, F_GETPATH, image_info->path) != 0)
    {
        errno = ENOMEM;
        LCUnmapMachO(machO);
        return NULL;
    }
    
    bool success = KXMapMachOExecutable(machO, image_info);
    LCUnmapMachO(machO);
    if(!success)
    {
        /* sets errno */
        kxdestroy_image(image_info);
        return NULL;
    }
    
    /* now let the fixup */
    if(!KXApplyFixups(image_info))
    {
        kxdestroy_image(image_info);
        return NULL;
    }
    
    /* now we gotta get kmod */
    if(!KXLocateKmod(image_info))
    {
        kxdestroy_image(image_info);
        return NULL;
    }
    
    /* now the spicy port with the symbol exports */
    if(!KXRegisterKextExports(image_info))
    {
        kxdestroy_image(image_info);
        return NULL;
    }
    
    if(!KXRegisterObjCImage(image_info))
    {
        kxdestroy_image(image_info);
        return NULL;
    }
    
    /* now resealing */
    if(!KXResealDataConst(image_info))
    {
        kxdestroy_image(image_info);
        return NULL;
    }
    
    if(!KXRunInitializers(image_info))
    {
        kxdestroy_image(image_info);
        return NULL;
    }
    
    return image_info;
}

void kxclose(kxld_image_info_t *image_info)
{
    kxdestroy_image(image_info);
}
