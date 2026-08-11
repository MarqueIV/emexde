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

#import <LindChain/Services/containerd/PEContainer.h>
#import <LindChain/Services/containerd/PEContainerProtocol.h>
#import <LindChain/ProcEnvironment/Process/PELaunchServiceRegistry.h>
#import <LindChain/ProcEnvironment/Surface/trust.h>
#import <LindChain/Utils/Swizzle.h>

@interface MoltenFileManager : NSObject

@end

@implementation MoltenFileManager

+ (void)load
{
    [super load];
    SwizzleObjCMethod(@selector(contentsOfDirectoryAtPath:error:), [NSFileManager class], @selector(hook_contentsOfDirectoryAtPath:error:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
}

- (NSArray<NSString *> *)hook_contentsOfDirectoryAtPath:(NSString *)path
                                                  error:(NSError **)error
{
    BOOL isInside = NO;
    NSString *containerRoot = [[[[PEContainer shared] getContainerRoot] path] stringByStandardizingPath];
    if(containerRoot != NULL)
    {
        path = [path stringByStandardizingPath];
        isInside = [path isEqualToString:containerRoot] || [path hasPrefix:[containerRoot stringByAppendingString:@"/"]];
    }
    return isInside ? [[PEContainer shared] contentsOfDirectoryAtPath:path error:error] : [self hook_contentsOfDirectoryAtPath:path error:error];
}

@end

