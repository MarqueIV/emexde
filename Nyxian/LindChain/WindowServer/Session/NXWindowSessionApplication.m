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

#import <LindChain/WindowServer/Session/NXWindowSessionApplication.h>
#import <LindChain/ProcEnvironment/Surface/proc/proc.h>
#import <LindChain/WindowServer/NXWindowServer.h>
#import <LindChain/ProcEnvironment/Process/PEExtension.h>
#import <LindChain/Utils/Swizzle.h>

#import <LindChain/Services/applicationmgmtd/LDEApplicationWorkspace.h>
#import <LindChain/ProcEnvironment/Utils/klog.h>
#import <objc/runtime.h>
#import <os/lock.h>
#import <objc/message.h>

@implementation NXWindowSessionApplication {
    UIView *_contentView;
}

@dynamic contentView;

- (instancetype)initWithProcess:(PEProcess*)process;
{
    if(process == nil)
    {
        return nil;
    }
    
    self = [super init];
    if(self)
    {
        _process = process;
    }
    return self;
}

- (UIView*)contentView
{
    assert([NSThread isMainThread]);
    return _contentView;
}

- (void)setContentView:(UIView *)contentView
{
    assert([NSThread isMainThread]);
    if(_contentView != nil)
    {
        [_contentView removeFromSuperview];
    }
    _contentView = contentView;
    [self.view addSubview:contentView];
    
    contentView.translatesAutoresizingMaskIntoConstraints = NO;
    
    [NSLayoutConstraint activateConstraints:@[
        [contentView.topAnchor constraintEqualToAnchor:self.view.topAnchor],
        [contentView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
        [contentView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [contentView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
    ]];
}

+ (void)bringSessionToFrontWithBundleIdentifier:(NSString*)bundleIdentifier
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if(UIDevice.currentDevice.userInterfaceIdiom != UIUserInterfaceIdiomPad) return;
        NXWindowServer *windowServer = [NXWindowServer shared];
        assert(windowServer != nil);
        
        for(NSNumber *key in windowServer.windows)
        {
            NXWindow *window = windowServer.windows[key];
            
            if(window != nil &&
               [window.session isKindOfClass:[NXWindowSessionApplication class]] &&
               [((NXWindowSessionApplication*)(window.session)).process.bundleIdentifier isEqualToString:bundleIdentifier])
            {
                [window.view.superview bringSubviewToFront:window.view];
                [window focusWindow];
                break;
            }
        }
    });
}

- (BOOL)bindInApplicationWindow
{
    assert([NSThread isMainThread]);
    
    /* destroy existing window if there is one already */
    if(_scene != nil)
    {
        [self.windowScene _unregisterSettingsDiffActionArrayForKey:_scene.identifier];
    }
    if(self.scenePresenter)
    {
        [self.scenePresenter invalidate];
    }
    if(self.scene)
    {
        [[PrivClass(FBSceneManager) sharedInstance] destroyScene:self.scene withTransitionContext:nil];
    }
    
    /* create a new window using the new lifecycle */
    void (^updateSceneSettings)(id) = ^void(UIMutableApplicationSceneSettings *settings) {
        settings.canShowAlerts = YES;
        settings.cornerRadiusConfiguration = [[PrivClass(BSCornerRadiusConfiguration) alloc] initWithTopLeft:self.view.layer.cornerRadius bottomLeft:self.view.layer.cornerRadius bottomRight:self.view.layer.cornerRadius topRight:self.view.layer.cornerRadius];
        settings.displayConfiguration = UIScreen.mainScreen.displayConfiguration;
        settings.foreground = NO;
        settings.level = 1;
        settings.persistenceIdentifier = [NSString stringWithFormat:@"sceneID:%@-%@", @"LiveProcess", [NSUUID.UUID UUIDString]];
        settings.statusBarDisabled = true;
    };
    void (^updateSceneClientSettings)(id) = ^void(UIMutableApplicationSceneClientSettings *clientSettings) {
        clientSettings.interfaceOrientation = UIInterfaceOrientationPortrait;
        clientSettings.statusBarStyle = 0;
    };
    
    _UISceneHostingControllerAdvancedConfiguration *config = [[_UISceneHostingControllerAdvancedConfiguration alloc] initWithProcessIdentity:self.process.process.identity];
    config.sceneSpecification = [UIApplicationSceneSpecification specification];
    if(!@available(iOS 27.0, *))
    {
        /* on 27 manually adding this is not need, also setAdditionalExtensions: doesn't exist for some reason */
        config.additionalExtensions = [NSOrderedSet orderedSetWithArray:@[
            PrivClass(_UISceneHostingEventDeferringExtension),
        ]];
    }
    else
    {
        SEL settingsSelector = NSSelectorFromString(@"setInitialSettingsUpdater:");
        if([config respondsToSelector:settingsSelector])
        {
            void (*sendSettings)(id, SEL, id) = (void (*)(id, SEL, id))objc_msgSend;
            sendSettings(config, settingsSelector, updateSceneSettings);
        }
        
        SEL clientSettingsSelector = NSSelectorFromString(@"setInitialClientSettingsUpdater:");
        if([config respondsToSelector:clientSettingsSelector])
        {
            void (*sendClientSettings)(id, SEL, id) = (void (*)(id, SEL, id))objc_msgSend;
            sendClientSettings(config, clientSettingsSelector, updateSceneClientSettings);
        }
    }
    
    self.sceneHostingController = [[_UISceneHostingController alloc] initWithAdvancedConfiguration:config];
    
    if(@available(iOS 27.0, *))
    {
        Class deferringExtensionClass = NSClassFromString(@"_UISceneEventDeferringExtension"); // Or equivalent internal extension class
        if(deferringExtensionClass)
        {
            /* need to add extension, otherwise the instance can make caboom */
            [self.sceneHostingController addExtension:deferringExtensionClass];
        }
        [self.sceneHostingController configureScene];
        
        _UISceneEventDeferringHostComponent *deferringComponent = [self.sceneHostingController performSelector:@selector(_eventDeferringComponent)];
        if(deferringComponent)
        {
            [deferringComponent setValue:self forKey:@"_firstResponderTrackingSelectionPath"];
            [deferringComponent setGrantBehavior:2];
            [deferringComponent setSelectionRequestBehavior:2];
        }
        else
        {
            klog_log("NXWindowSessionApplication", "Unexpectedly nil _UISceneEventDeferringHostComponent");
        }
    }
    
    self.contentView = self.sceneHostingController.sceneViewController.view;
    self.contentView.clipsToBounds = NO;
    self.scenePresenter = [self.contentView valueForKey:@"_scenePresenter"];
    self.scene = self.scenePresenter.scene;
    
    /* gosh apple changed the behaviour of this API to death ;w; */
    if(!@available(iOS 27.0, *))
    {
        [self.scene updateSettingsWithBlock:updateSceneSettings];
    }
    
    /* register that shit */
    [self.windowScene _registerSettingsDiffActionArray:@[self] forKey:self.scene.identifier];
    
    /* FIXME: no way to update client settings so far */
    
    return YES;
}

- (BOOL)openWindow
{
    if(![super openWindow])
    {
        return NO;
    }
    
    return [self bindInApplicationWindow];
}

- (BOOL)closeWindow
{
    [super closeWindow];
    
    if(self.scene != nil)
    {
        /* bye bye presenter */
        [_scenePresenter invalidate];
        [self.windowScene _unregisterSettingsDiffActionArrayForKey:self.scene.identifier];
        [[PrivClass(FBSceneManager) sharedInstance] destroyScene:self.scene withTransitionContext:nil];
    }
    [_process terminate];
    
    return YES;
}

- (UIImage*)snapshotWindow
{
    if(_process == nil) return nil;
    return _process.snapshot;
}

- (BOOL)activateWindow
{
    assert([NSThread isMainThread]);
    
    /* set presenter to foreground */
    [_scene updateSettingsWithBlock:^(UIMutableApplicationSceneSettings *settings) {
        settings.foreground = YES;
    }];
    
    /* re-activate presenter */
    [_scenePresenter activate];
    
    return YES;
}

- (BOOL)deactivateWindow
{
    assert([NSThread isMainThread]);
    
    /* set presenter to background */
    [_scene updateSettingsWithBlock:^(UIMutableApplicationSceneSettings *settings) {
        settings.foreground = NO;
    }];
 
    /* TODO: implement the jailbreak way of getting a snapshot of a iOS app */
    [_process sendSignal:SIGUSR1];
    
    /* deactivate the presenter */
    [_scenePresenter deactivate];
    
    return YES;
}

- (void)windowRectChanged
{
    assert([NSThread isMainThread]);
    
    [super windowRectChanged];
    
    CGRect rect = self.view.frame;
    
    if(self.process.isSuspended)
    {
        return;
    }
    
    /* update window dimensions */
    [_scene updateSettingsWithBlock:^(UIMutableApplicationSceneSettings *settings) {
        UIEdgeInsets insets = (self.isFullscreen) ? NXWindowServer.shared.safeAreaInsets : UIEdgeInsetsZero;
        
        /* looks unnatural without */
        insets.top = 10;
        
        switch(settings.interfaceOrientation)
        {
            case UIInterfaceOrientationPortrait:
                settings.safeAreaInsetsPortrait = insets;
                break;
            case UIInterfaceOrientationPortraitUpsideDown:
                settings.safeAreaInsetsPortraitUpsideDown = insets;
                break;
            case UIInterfaceOrientationLandscapeLeft:
                settings.safeAreaInsetsLandscapeLeft = insets;
                break;
            case UIInterfaceOrientationLandscapeRight:
                settings.safeAreaInsetsLandscapeRight = insets;
            case UIInterfaceOrientationUnknown:
                break;
        }
    }];
}

- (void)_performActionsForUIScene:(UIScene *)scene
              withUpdatedFBSScene:(id)fbsScene
                     settingsDiff:(FBSSceneSettingsDiff *)diff
                     fromSettings:(id)settings
                transitionContext:(id)context
              lifecycleActionType:(uint32_t)actionType
{
    assert([NSThread isMainThread]);
    
    if(!self.process.process.running || self.process.isSuspended || !diff)
    {
        return;
    }
    
    UIMutableApplicationSceneSettings *baseSettings = [diff settingsByApplyingToMutableCopyOfSettings:settings];
    UIApplicationSceneTransitionContext *newContext = [context copy];
    newContext.actions = nil;
    
    UIMutableApplicationSceneSettings *newSettings = [_scene.settings mutableCopy];
    newSettings.userInterfaceStyle = baseSettings.userInterfaceStyle;
    
    [_scene updateSettings:newSettings withTransitionContext:newContext completion:nil];
    
    [self windowRectChanged];
}

- (BOOL)shouldUpdateFocusInContext:(nonnull UIFocusUpdateContext *)context
{
    return YES;
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
    assert([NSThread isMainThread]);
    
    [super traitCollectionDidChange:previousTraitCollection];
    
    if(!self.process.process.running || self.process.isSuspended)
    {
        return;
    }
    
    if(self.traitCollection.userInterfaceStyle != previousTraitCollection.userInterfaceStyle)
    {
        [_scene updateSettingsWithBlock:^(UIMutableApplicationSceneSettings *settings) {
            settings.userInterfaceStyle = self.traitCollection.userInterfaceStyle;
        }];
    }
}

- (NSString*)windowName
{
    return self.process.displayName;
}

- (void)prepareForInject
{
    /* making sure LDEProcess wont close this */
    self.process.wid = (id_t)-1;
    self.process.session = nil;
}

- (BOOL)injectProcess:(PEProcess*)process
{
    assert([NSThread isMainThread]);
    self.process = process;
    if(![self bindInApplicationWindow])
    {
        return NO;
    }
    [self activateWindow];
    [self windowRectChanged];
    return YES;
}

- (NSString*)getWindowName
{
    NSString *windowName = [super getWindowName];
    return windowName ?: self.process.displayName;
}

- (void)dealloc
{
    NSLog(@"deallocated %@", self);
}

@end
