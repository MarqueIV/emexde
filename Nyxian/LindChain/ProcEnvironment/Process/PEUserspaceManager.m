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

@implementation PEUserspaceManager {
    os_unfair_lock _lock;
}

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

- (instancetype)init
{
    self = [super init];
    if(self)
    {
        _lock = OS_UNFAIR_LOCK_INIT;
        _mode = kPEUserspaceModeDefault;
    }
    return self;
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

- (void)rebootUserspaceWithType_nolock:(PEUserspaceRebootType)type
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
    
    klog_log("PEUserspaceManager:reboot", "reloading daemons");
    switch(type)
    {
        case kPEUserspaceRebootTypeDefault:
            _mode = kPEUserspaceModeDefault;
            break;
        case kPEUserspaceRebootTypeMinimal:
            _mode = kPEUserspaceModeMinimal;
            break;
        default:
            _mode = kPEUserspaceModeEmpty;
            break;
    }
    [[PEUserspaceManager shared] reloadDaemons_nolock];
    
    klog_log("PEUserspaceManager:reboot", "userspace rebooted successfully");
}

- (void)rebootUserspaceWithType:(PEUserspaceRebootType)type
{
    os_unfair_lock_lock(&_lock);
    [self rebootUserspaceWithType_nolock:type];
    os_unfair_lock_unlock(&_lock);
}

- (void)rebootUserspace
{
    [self rebootUserspaceWithType:kPEUserspaceRebootTypeDefault];
}

- (void)restore
{
    os_unfair_lock_lock(&_lock);
    goto first;
    
retry:  /* a retry shall not happen, happens tho if something goes wrong */
    klog_log("PEUserspaceManager:restore", "failed to restore, reattempt restore");
    
first:
    {
        /* needs to be in minimal userspace boot mode to safely begin restoring the container through containerd */
        klog_log("PEUserspaceManager:restore", "rebooting userspace into minimal mode");
        [self rebootUserspaceWithType_nolock:kPEUserspaceRebootTypeMinimal];
        
        /* waiting till containerd is back */
        sleep(1);
        
        /* getting all directories needed */
        klog_log("PEUserspaceManager:restore", "gathering path intel");
        NSURL *containerRoot = [[PEContainer shared] getContainerRoot];
        if(containerRoot == NULL)
        {
            goto retry;
        }
        
        NSURL *containerData = [containerRoot URLByAppendingPathComponent:@"Documents"];
        NSURL *containerTmp = [containerRoot URLByAppendingPathComponent:@"tmp"];
        NSURL *containerLibrary = [containerRoot URLByAppendingPathComponent:@"Library"];
        if(containerData == NULL || containerTmp == NULL || containerLibrary == NULL)
        {
            goto retry;
        }
        
        /* getting contents of each */
        NSArray<NSString*> *containerHomeDirectories = [[PEContainer shared] contentsOfDirectoryAtPath:[containerData path] error:nil];
        NSArray<NSString*> *containerTmpDirectories = [[PEContainer shared] contentsOfDirectoryAtPath:[containerTmp path] error:nil];
        NSArray<NSString*> *containerLibraryDirectories = [[PEContainer shared] contentsOfDirectoryAtPath:[containerLibrary path] error:nil];
        klog_log("PEUserspaceManager:restore", "directories to tear down \ninside of %@: %@\n\ninside of %@: %@\ninside of %@: %@", containerData, containerHomeDirectories, containerTmp, containerTmpDirectories, containerLibrary, containerLibraryDirectories);
        
        /* deleting everything */
        klog_log("PEUserspaceManager:restore", "restoring container file system");
        for(NSString *pathComponent in containerHomeDirectories)
        {
            NSURL *itemURL = [containerData URLByAppendingPathComponent:pathComponent];
            if(![[PEContainer shared] removeItemAtURL:itemURL error:nil])
            {
                klog_log("PEUserspaceManager:restore", "tearing down %@ failed", itemURL);
                goto retry;
            }
        }
        for(NSString *pathComponent in containerTmpDirectories)
        {
            NSURL *itemURL = [containerTmp URLByAppendingPathComponent:pathComponent];
            if(![[PEContainer shared] removeItemAtURL:itemURL error:nil])
            {
                klog_log("PEUserspaceManager:restore", "tearing down %@ failed", itemURL);
                goto retry;
            }
        }
        for(NSString *pathComponent in containerLibraryDirectories)
        {
            NSURL *itemURL = [containerLibrary URLByAppendingPathComponent:pathComponent];
            if(![[PEContainer shared] removeItemAtURL:itemURL error:nil])
            {
                /* allowed to fail sometimes */
                klog_log("PEUserspaceManager:restore", "tearing down %@ failed", itemURL);
            }
        }
        
        /* now we have to restore the default hostname */
        klog_log("PEUserspaceManager:restore", "restoring hostname");
        ksurface_sethostname(@"localhost");
        
        /* rebooting into empty mode, to restore the private keys entirely safely */
        klog_log("PEUserspaceManager:restore", "rebooting userspace into empty mode");
        [self rebootUserspaceWithType_nolock:kPEUserspaceRebootTypeEmpty];
        
        klog_log("PEUserspaceManager:restore", "restoring code signature key pair");
        uint8_t *new_priv = NULL, *new_pub = NULL;
        size_t new_priv_len = 0, new_pub_len = 0;
        
        if(!get_kernel_ec_key(&new_priv, &new_priv_len, &new_pub, &new_pub_len))
        {
            goto retry;
        }
        
        int ret = store_kernel_key(new_priv, new_priv_len, new_pub, new_pub_len);
        free(new_priv);
        free(new_pub);
        if(ret != 0)
        {
            goto retry;
        }
        
        /* regather them */
        ksurface_kinit_get_keys();
        
        /* clearing app list TODO: make it a actual "client portal" instead */
        klog_log("PEUserspaceManager:restore", "restored successfully");
        [[ApplicationManagementViewController shared] removeAllApplications];
        
        /* we're done, now rebooting back into default mode */
        klog_log("PEUserspaceManager:restore", "bringing userspace back into normal mode");
        [self rebootUserspaceWithType_nolock:kPEUserspaceRebootTypeDefault];
        
        /* waiting till everything is back */
        sleep(1);
        
        /* TODO: make the entire reboot timing perfect */
    }
    os_unfair_lock_unlock(&_lock);
}

- (void)reloadDaemons_nolock
{
    [[PELaunchServiceRegistry shared] invalidateAllEntries];
    if(_mode == kPEUserspaceModeDefault)
    {
        [[PELaunchServiceRegistry shared] reloadAllEntries];
    }
    else if(_mode == kPEUserspaceModeMinimal)
    {
        [[PELaunchServiceRegistry shared] loadEntryWithFileName:@"org.emexlabs.containerd.plist"];
    }
}

- (void)reloadDaemons
{
    os_unfair_lock_lock(&_lock);
    [[PEUserspaceManager shared] reloadDaemons_nolock];
    os_unfair_lock_unlock(&_lock);
}

@end
