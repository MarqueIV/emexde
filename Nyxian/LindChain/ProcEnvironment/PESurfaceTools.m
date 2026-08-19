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

#if DEBUG

#import <LindChain/ProcEnvironment/PESurfaceTools.h>
#import <LindChain/ProcEnvironment/Surface/proc/list.h>

@implementation PESurfaceProcDescriptor

- (instancetype)initWithProc:(ksurface_proc_t*)proc
{
    self = [super init];
    if(self)
    {
        _rawProc = proc;
    }
    return self;
}

@end

void pesurface_proc_radix_walker_callback(uint64_t ident,
                                          void *value,
                                          void *ctx)
{
    NSMutableArray *array = (__bridge NSMutableArray*)ctx;
    ksurface_proc_t *proc = value;
    [array addObject:[[PESurfaceProcDescriptor alloc] initWithProc:proc]];
}

@implementation PESurfaceStatic

+ (NSArray<PESurfaceProcDescriptor*>*)allProcesses
{
    proc_table_rdlock();
    NSMutableArray *array = [[NSMutableArray alloc] initWithCapacity:ksurface->proc_info.proc_count];
    radix_walk(&(ksurface->proc_info.tree), pesurface_proc_radix_walker_callback, (void*)(__bridge CFMutableArrayRef)array);
    proc_table_unlock();
    return array;
}

@end

#endif /* DEBUG */
