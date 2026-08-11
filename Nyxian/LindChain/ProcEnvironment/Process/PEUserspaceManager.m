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

#import <LindChain/ProcEnvironment/Process/PEUserspaceManager.h>
#import <LindChain/ProcEnvironment/Process/PELaunchServiceRegistry.h>
#import <LindChain/ProcEnvironment/Process/PEProcessManager.h>
#import <LindChain/ProcEnvironment/Process/PEExtension.h>
#import <LindChain/Services/containerd/PEContainer.h>
#import <LindChain/ProcEnvironment/Utils/klog.h>
#import <Nyxian-Swift.h>

@implementation PEUserspaceManager

+ (void)load
{
    [[PEUserspaceManager shared] boot];
}

+ (instancetype)shared
{
    static PEUserspaceManager *shared;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        shared = [[PEUserspaceManager alloc] init];
    });
    return shared;
}

- (void)boot
{
    static atomic_flag flag = ATOMIC_FLAG_INIT;
    assert(!atomic_flag_test_and_set(&flag));
    
    if(PEGetLiveProcessBundle() != NULL)
    {
        klog_log("PEUserspaceManager:boot", "spinning up micro kernel");
        ksurface_kinit();
        
        klog_log("PEUserspaceManager:boot", "spinning up launch services");
        [PELaunchServiceRegistry shared];
    }
}

- (void)rebootUserspaceWithType:(PEUserspaceRebootType)type
{
    /* TODO: prevent spawns from happening, deny any new spawns too */
    klog_log("PEUserspaceManager:reboot", "aquiring proctil lock");
    if(proctil(kProctilActionLock) != KERN_SUCCESS)
    {
        klog_log("PEUserspaceManager:reboot", "userspace reboot failed, lock couldn't be claimed");
        return;
    }
    
    klog_log("PEUserspaceManager:reboot", "invalidating all launch service entries in registry");
    [[PELaunchServiceRegistry shared] invalidateAllEntries];    /* causes reignition to fail in launch services, so killing will not automatically restart them */
    
    klog_log("PEUserspaceManager:reboot", "killing all running processes");
    [[PEProcessManager shared] killAllRunningProcesses];
    
    klog_log("PEUserspaceManager:reboot", "releasing proctil lock");
    proctil(kProctilActionUnlock);
    
    if(type == kPEUserspaceRebootTypeDefault)
    {
        klog_log("PEUserspaceManager:reboot", "reloading all launch service entries to registry");
        [[PELaunchServiceRegistry shared] reloadAllEntries];
    }
    else if(type == kPEUserspaceRebootTypeMinimal)
    {
        klog_log("PEUserspaceManager:reboot", "reloading containerd launch service entry to registry");
        [[PELaunchServiceRegistry shared] loadEntryWithFileName:@"org.emexlabs.containerd.plist"];
        /* somehow prevent it from booting installd TM */
    }
    
    klog_log("PEUserspaceManager:reboot", "userspace rebooted successfully");
}

- (void)rebootUserspace
{
    [self rebootUserspaceWithType:kPEUserspaceRebootTypeDefault];
}

- (void)restore
{
    /* needs to be in minimal userspace boot mode to safely begin restoring the container through containerd */
    klog_log("PEUserspaceManager:restore", "userspace rebooting into minimal mode");
    [self rebootUserspaceWithType:kPEUserspaceRebootTypeMinimal];
    
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        /* getting all directories needed */
        klog_log("PEUserspaceManager:restore", "gathering path intel");
        NSURL *containerRoot = [[PEContainer shared] getContainerRoot];
        NSURL *containerData = [containerRoot URLByAppendingPathComponent:@"Documents"];
        NSURL *containerTmp = [containerRoot URLByAppendingPathComponent:@"tmp"];
        
        /* getting contents of each */
        NSArray<NSString*> *containerHomeDirectories = [[PEContainer shared] contentsOfDirectoryAtPath:[containerData path] error:nil];
        NSArray<NSString*> *containerTmpDirectories = [[PEContainer shared] contentsOfDirectoryAtPath:[containerTmp path] error:nil];
        klog_log("PEUserspaceManager:restore", "directories to tear down \ninside of %@: %@\n\ninside of %@: %@", containerData, containerHomeDirectories, containerTmp, containerTmpDirectories);
        
        /* deleting everything */
        for(NSString *pathComponent in containerHomeDirectories)
        {
            NSURL *itemURL = [containerData URLByAppendingPathComponent:pathComponent];
            if(![[PEContainer shared] removeItemAtURL:itemURL error:nil])
            {
                klog_log("PEUserspaceManager:restore", "tearing down %@ failed", itemURL);
            }
        }
        for(NSString *pathComponent in containerTmpDirectories)
        {
            NSURL *itemURL = [containerTmp URLByAppendingPathComponent:pathComponent];
            if(![[PEContainer shared] removeItemAtURL:itemURL error:nil])
            {
                klog_log("PEUserspaceManager:restore", "tearing down %@ failed", itemURL);
            }
        }
        
        /* now we have to restore the default hostname */
        klog_log("PEUserspaceManager:restore", "resetting hostname");
        ksurface_sethostname(@"localhost");
        
        /* clearing app list TODO: make it a actual "client portal" instead */
        klog_log("PEUserspaceManager:restore", "restored successfully");
        [[ApplicationManagementViewController shared] removeAllApplications];
        
        /* we're done, now rebooting back into default mode */
        klog_log("PEUserspaceManager:restore", "bringing userspace back into normal mode");
        [self rebootUserspaceWithType:kPEUserspaceRebootTypeDefault];
    });
}

@end
