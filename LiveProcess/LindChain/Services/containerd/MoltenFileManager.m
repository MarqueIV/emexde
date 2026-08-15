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
#import <LindChain/ProcEnvironment/PELaunchServiceRegistry.h>
#import <LindChain/ProcEnvironment/Surface/trust.h>
#import <LindChain/Utils/Swizzle.h>

static BOOL MoltenFileManagerIsPathInsideContainer(NSString *path)
{
    BOOL isInside = NO;
    NSString *containerRoot = [[[[PEContainer shared] getContainerRoot] path] stringByStandardizingPath];
    if(containerRoot != NULL)
    {
        path = [path stringByStandardizingPath];
        isInside = [path isEqualToString:containerRoot] || [path hasPrefix:[containerRoot stringByAppendingString:@"/"]];
    }
    return isInside;
}

static BOOL MoltenFileManagerIsURLInsideContainer(NSURL *url)
{
    BOOL isInside = NO;
    NSString *containerRoot = [[[[PEContainer shared] getContainerRoot] path] stringByStandardizingPath];
    if(containerRoot != NULL)
    {
        NSString *path = [[url path] stringByStandardizingPath];
        isInside = [path isEqualToString:containerRoot] || [path hasPrefix:[containerRoot stringByAppendingString:@"/"]];
    }
    return isInside;
}

static BOOL MoltenFileManagerTransfer(NSURL *srcURL,
                                      NSURL *destURL,
                                      BOOL move,
                                      NSError **error)
{
    if(MoltenFileManagerIsURLInsideContainer(srcURL) && MoltenFileManagerIsURLInsideContainer(destURL))
    {
        return [[PEContainer shared] moveItemAtURL:srcURL toURL:destURL error:error];
    }
    
    NSURL *containerURL = MoltenFileManagerIsURLInsideContainer(srcURL) ? srcURL : destURL;
    NSURL *hostURL = containerURL == srcURL ? destURL : srcURL;
    BOOL reverse = containerURL == destURL;
    
    if(!reverse)
    {
        BOOL isDirectory;
        if(![[PEContainer shared] fileExistsAtPath:[containerURL path] isDirectory:&isDirectory] || isDirectory)
        {
            return NO;
        }
        
        PEFileHandle *containerObject = [[PEContainer shared] fileHandleForItemAtPath:[containerURL path] withFlags:O_RDWR withMode:0];
        if(![containerObject writeOut:[hostURL path]])
        {
            return NO;
        }
        
        if(move)
        {
            [[PEContainer shared] removeItemAtURL:containerURL error:nil];
        }
    }
    else
    {
        BOOL isDirectory;
        if(![[NSFileManager defaultManager] fileExistsAtPath:[hostURL path] isDirectory:&isDirectory] || isDirectory)
        {
            return NO;
        }
        
        PEFileHandle *containerObject = [[PEContainer shared] fileHandleForItemAtPath:[containerURL path] withFlags:O_RDWR | O_CREAT | O_TRUNC withMode:0777];
        if(![containerObject writeIn:[hostURL path]])
        {
            return NO;
        }
        
        if(move)
        {
            [[NSFileManager defaultManager] removeItemAtPath:[hostURL path] error:nil];
        }
    }
    
    return YES;
}

@interface MoltenFileManager : NSObject

@end

@implementation MoltenFileManager

+ (void)load
{
    [super load];
    SwizzleObjCMethod(@selector(contentsOfDirectoryAtPath:error:), [NSFileManager class], @selector(hook_contentsOfDirectoryAtPath:error:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
    SwizzleObjCMethod(@selector(removeItemAtPath:error:), [NSFileManager class], @selector(hook_removeItemAtPath:error:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
    SwizzleObjCMethod(@selector(removeItemAtURL:error:), [NSFileManager class], @selector(hook_removeItemAtURL:error:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
    SwizzleObjCMethod(@selector(moveItemAtPath:toPath:error:), [NSFileManager class], @selector(hook_moveItemAtPath:toPath:error:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
    SwizzleObjCMethod(@selector(moveItemAtURL:toURL:error:), [NSFileManager class], @selector(hook_moveItemAtURL:toURL:error:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
    SwizzleObjCMethod(@selector(copyItemAtPath:toPath:error:), [NSFileManager class], @selector(hook_copyItemAtPath:toPath:error:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
    SwizzleObjCMethod(@selector(copyItemAtURL:toURL:error:), [NSFileManager class], @selector(hook_copyItemAtURL:toURL:error:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
    SwizzleObjCMethod(@selector(attributesOfItemAtPath:error:), [NSFileManager class], @selector(hook_attributesOfItemAtPath:error:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
    SwizzleObjCMethod(@selector(setAttributes:ofItemAtPath:error:), [NSFileManager class], @selector(hook_setAttributes:ofItemAtPath:error:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
    SwizzleObjCMethod(@selector(fileExistsAtPath:isDirectory:), [NSFileManager class], @selector(hook_fileExistsAtPath:isDirectory:), [MoltenFileManager class], kSwizzleMethodTypeInstance);
}

- (NSArray<NSString *> *)hook_contentsOfDirectoryAtPath:(NSString *)path
                                                  error:(NSError **)error
{
    return MoltenFileManagerIsPathInsideContainer(path) ? [[PEContainer shared] contentsOfDirectoryAtPath:path error:error] : [self hook_contentsOfDirectoryAtPath:path error:error];
}

- (BOOL)hook_removeItemAtPath:(NSString*)path
                        error:(NSError**)error
{
    return MoltenFileManagerIsPathInsideContainer(path) ? [[PEContainer shared] removeItemAtURL:[NSURL fileURLWithPath:path] error:error] : [self hook_removeItemAtPath:path error:error];
}

- (BOOL)hook_removeItemAtURL:(NSURL*)url
                       error:(NSError**)error
{
    return MoltenFileManagerIsURLInsideContainer(url) ? [[PEContainer shared] removeItemAtURL:url error:error] : [self hook_removeItemAtURL:url error:error];
}

- (BOOL)hook_moveItemAtPath:(NSString*)srcPath
                     toPath:(NSString*)destPath
                      error:(NSError**)error
{
    return (!MoltenFileManagerIsPathInsideContainer(srcPath) && !MoltenFileManagerIsPathInsideContainer(destPath)) ? [self hook_moveItemAtPath:srcPath toPath:destPath error:error] : MoltenFileManagerTransfer([NSURL fileURLWithPath:srcPath], [NSURL fileURLWithPath:destPath], YES, error);
}

- (BOOL)hook_moveItemAtURL:(NSURL*)srcURL
                     toURL:(NSURL*)destURL
                     error:(NSError**)error
{
    return (!MoltenFileManagerIsURLInsideContainer(srcURL) && !MoltenFileManagerIsURLInsideContainer(destURL)) ? [self hook_moveItemAtURL:srcURL toURL:destURL error:error] : MoltenFileManagerTransfer(srcURL, destURL, YES, error);
}

- (BOOL)hook_copyItemAtPath:(NSString*)srcPath
                     toPath:(NSString*)destPath
                      error:(NSError**)error
{
    return (!MoltenFileManagerIsPathInsideContainer(srcPath) && !MoltenFileManagerIsPathInsideContainer(destPath)) ? [self hook_copyItemAtPath:srcPath toPath:destPath error:error] : MoltenFileManagerTransfer([NSURL fileURLWithPath:srcPath], [NSURL fileURLWithPath:destPath], NO, error);
}

- (BOOL)hook_copyItemAtURL:(NSURL*)srcURL
                     toURL:(NSURL*)destURL
                     error:(NSError**)error
{
    return (!MoltenFileManagerIsURLInsideContainer(srcURL) && !MoltenFileManagerIsURLInsideContainer(destURL)) ? [self hook_copyItemAtURL:srcURL toURL:destURL error:error] : MoltenFileManagerTransfer(srcURL, destURL, NO, error);
}

- (NSDictionary<NSFileAttributeKey, id> *)hook_attributesOfItemAtPath:(NSString *)path
                                                                error:(NSError **)error
{
    return MoltenFileManagerIsPathInsideContainer(path) ? [[PEContainer shared] attributesOfItemAtPath:path error:error] : [self hook_attributesOfItemAtPath:path error:error];
}

- (BOOL)hook_setAttributes:(NSDictionary<NSFileAttributeKey, id> *)attributes
              ofItemAtPath:(NSString *)path
                     error:(NSError **)error
{
    return MoltenFileManagerIsPathInsideContainer(path) ? [[PEContainer shared] setAttributes:attributes ofItemAtPath:path error:error] : [self hook_setAttributes:attributes ofItemAtPath:path error:error];
}

- (BOOL)hook_fileExistsAtPath:(NSString*)path
                  isDirectory:(BOOL*)isDirectory
{
    return MoltenFileManagerIsPathInsideContainer(path) ? [[PEContainer shared] fileExistsAtPath:path isDirectory:isDirectory] : [self hook_fileExistsAtPath:path isDirectory:isDirectory];
}

@end
