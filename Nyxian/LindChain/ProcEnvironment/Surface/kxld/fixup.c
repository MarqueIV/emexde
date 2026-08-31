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

#include <LindChain/ProcEnvironment/Surface/kxld/fixup.h>
#include <mach-o/fixup-chains.h>
#include <dlfcn.h>

bool KXApplyChainedFixups(kxld_image_info_t *image_info)
{
    /* look for fixup chains TODO: shall be one command walk */
    const struct linkedit_data_command *chainedFixupsCmd = NULL;
    const uint8_t *ptr = (const uint8_t *)image_info->header + sizeof(struct mach_header_64);
    uint64_t ncmds = image_info->header->ncmds;
    for(uint32_t i = 0; i < ncmds; i++)
    {
        const struct load_command *lc = (const struct load_command *)ptr;
        if(lc->cmd == LC_DYLD_CHAINED_FIXUPS)
        {
            chainedFixupsCmd = (const struct linkedit_data_command *)ptr;
            break;
        }
        ptr += lc->cmdsize;
    }
    
    if(!chainedFixupsCmd)
    {
        /* nothing to fixup? */
        return true;
    }
    
    /* nice found the fixup chains */
    const uint8_t *fileBase = image_info->base;
    const struct dyld_chained_fixups_header *hdr = (const void *)(fileBase + image_info->sliceOffset + chainedFixupsCmd->dataoff);
    const struct dyld_chained_starts_in_image *starts = (const void *)((const uint8_t *)hdr + hdr->starts_offset);
    
    const struct dyld_chained_import *imports = (const void *)((const uint8_t *)hdr + hdr->imports_offset);
    const char *symbolPool = (const char *)((const uint8_t *)hdr + hdr->symbols_offset);
    
    for(uint32_t segIdx = 0; segIdx < starts->seg_count; segIdx++)
    {
        uint32_t segInfoOff = starts->seg_info_offset[segIdx];
        if(segInfoOff == 0)
        {
            continue;
        }
        
        const struct dyld_chained_starts_in_segment *seg = (const void *)((const uint8_t *)starts + segInfoOff);
        for(uint16_t pageIdx = 0; pageIdx < seg->page_count; pageIdx++)
        {
            uint16_t start = seg->page_start[pageIdx];
            if(start == DYLD_CHAINED_PTR_START_NONE)
            {
                continue;
            }
            
            uintptr_t pageAddr = (uintptr_t)image_info->slide + seg->segment_offset + (uintptr_t)pageIdx * seg->page_size;
            uintptr_t cursor = pageAddr + start;
            
            for(;;)
            {
                uint64_t *slot = (uint64_t *)cursor;
                uint64_t raw = *slot;
                
                struct dyld_chained_ptr_64_bind *b = (void *)&raw;
                struct dyld_chained_ptr_64_rebase *r = (void *)&raw;
                
                if(b->bind)
                {
                    /* external symbol to resolve ^^ */
                    const struct dyld_chained_import *imp = &imports[b->ordinal];
                    const char *name = symbolPool + imp->name_offset;
                    printf("trying to fixup: %s", name);
                    void *addr = dlsym(RTLD_DEFAULT, name);
                    if(addr != NULL)
                    {
                        goto fill_slot;
                    }
                    
                    char buf[NAME_MAX];
                    strlcpy(buf, name + 1, NAME_MAX);
                    addr = dlsym(RTLD_DEFAULT, buf);
                    
                    /*void *addr = SomeResolverIHaveToWriteSomeDayTMlol(name, imp->lib_ordinal, imp->weak_import);
                    if(!addr && !imp->weak_import)
                    {
                    }
                    *slot = (uint64_t)addr + b->addend;*/
                fill_slot:
                    printf(" with addr 0x%lx\n", (uintptr_t)addr);
                    *slot = (uintptr_t)addr + b->addend;
                }
                else
                {
                    *slot = (uint64_t)r->target + image_info->slide | ((uint64_t)r->high8 << 56);
                }
                
                if(r->next == 0)
                {
                    break;
                }
                cursor += (uintptr_t)r->next * 4;
            }
        }
    }
    
    return true;
}
