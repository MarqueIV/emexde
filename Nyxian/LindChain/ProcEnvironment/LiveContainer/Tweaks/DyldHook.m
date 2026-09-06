/*
 SPDX-License-Identifier: AGPL-3.0-or-later

 Copyright (C) 2023 - 2026 LiveContainer
 Copyright (C) 2026 emexlab

 This file is part of LiveContainer.

 LiveContainer is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 LiveContainer is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with Nyxian. If not, see <https://www.gnu.org/licenses/>.
*/

#import <LindChain/ProcEnvironment/LiveContainer/Tweaks/DyldHook.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <assert.h>
#include <LindChain/ProcEnvironment/LiveContainer/utils.h>
#include <LindChain/ProcEnvironment/litehook/litehook.h>
#include <LiveShim/ptrcache.h>
#include <mach/mach.h>

bool performHookDyldApiFast(DyldHookData index,
                            void** origFunction,
                            void* hookFunction)
{
    if(!load_ptrcache())
    {
        return false;
    }
    
    dyld_hook_data_t data = ptrcache_get_dyld_hook_data(index);
    
    assert(data.gdyldPtr != 0);
    assert(*(void**)data.gdyldPtr != 0);
    void* vtablePtr = **(void***)data.gdyldPtr;
    
    void* vtableFunctionPtr = 0;
    uint32_t* movInstPtr = data.adrpInstPtr + 6;

    if((*movInstPtr & 0x7F800000) == 0x52800000)
    {
        // arm64e, mov imm + add + ldr
        uint32_t imm16 = (*movInstPtr & 0x1FFFE0) >> 5;
        vtableFunctionPtr = vtablePtr + imm16;
    }
    else if ((*movInstPtr & 0xFFE00C00) == 0xF8400C00)
    {
        // arm64e, ldr immediate Pre-index 64bit
        uint32_t imm9 = (*movInstPtr & 0x1FF000) >> 12;
        vtableFunctionPtr = vtablePtr + imm9;
    }
    else
    {
        // arm64
        uint32_t* ldrInstPtr2 = data.adrpInstPtr + 3;
        assert((*ldrInstPtr2 & 0xBFC00000) == 0xB9400000);
        uint32_t size2 = (*ldrInstPtr2 & 0xC0000000) >> 30;
        uint32_t imm12_2 = (*ldrInstPtr2 & 0x3FFC00) >> 10;
        vtableFunctionPtr = vtablePtr + (imm12_2 << size2);
    }
    
    kern_return_t ret = builtin_vm_protect(mach_task_self(), (mach_vm_address_t)vtableFunctionPtr, sizeof(uintptr_t), false, PROT_READ | PROT_WRITE | VM_PROT_COPY);
    if(ret != KERN_SUCCESS)
    {
        assert(os_tpro_is_supported());
        os_thread_self_restrict_tpro_to_rw();
    }
    
    if(origFunction != NULL)
    {
        *origFunction = (void*)*(void**)vtableFunctionPtr;
    }
    
    *(uint64_t*)vtableFunctionPtr = (uint64_t)hookFunction;
    builtin_vm_protect(mach_task_self(), (mach_vm_address_t)vtableFunctionPtr, sizeof(uintptr_t), false, PROT_READ);
    if(ret != KERN_SUCCESS)
    {
        assert(os_tpro_is_supported());
        os_thread_self_restrict_tpro_to_ro();
    }
    return true;
}
