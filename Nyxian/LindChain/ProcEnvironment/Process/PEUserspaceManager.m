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
#import <LindChain/ProcEnvironment/Utils/klog.h>

@implementation PEUserspaceManager

+ (void)rebootUserspace
{
    /* TODO: prevent spawns from happening, deny any new spawns too */
    klog_log("PEUserspaceManager:reboot", "aquiring proctil lock");
    if(proctil(kProctilActionLock) != KERN_SUCCESS)
    {
        klog_log("PEUserspaceManager:Reboot", "userspace reboot failed, lock couldn't be claimed");
        return;
    }
    
    klog_log("PEUserspaceManager:reboot", "invalidating all launch service entries in registry");
    [[PELaunchServiceRegistry shared] invalidateAllEntries];    /* causes reignition to fail in launch services, so killing will not automatically restart them */
    
    klog_log("PEUserspaceManager:reboot", "killing all running processes");
    [[PEProcessManager shared] killAllRunningProcesses];
    
    klog_log("PEUserspaceManager:reboot", "releasing proctil lock");
    proctil(kProctilActionUnlock);
    
    klog_log("PEUserspaceManager:reboot", "reloading all launch service entries in registry");
    klog_log("PEUserspaceManager:reboot", "starting all launch services");
    [[PELaunchServiceRegistry shared] reloadAllEntries];
    
    klog_log("PEUserspaceManager:reboot", "userspace rebooted successfully");
}

@end
