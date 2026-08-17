/*
 SPDX-License-Identifier: AGPL-3.0-or-later

 Copyright (C) 2023 - 2025 LiveContainer
 Copyright (C) 2025 - 2026 mach-port

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

#import <LindChain/WindowServer/Window/NXResizeHandle.h>

@implementation NXResizeHandle {
    UIView *_backgroundView;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if(self)
    {
        self.layer.masksToBounds = YES;
        self.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleTopMargin;
        
        _backgroundView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, frame.size.width*sqrt(2), frame.size.height*sqrt(2))];
        if(_backgroundView == nil)
        {
            return nil;
        }
        
        _backgroundView.backgroundColor = [UIColor colorWithWhite:1 alpha:0.2]; // Normal state
        _backgroundView.center = CGPointMake(frame.size.width, frame.size.height);
        _backgroundView.transform = CGAffineTransformMakeRotation(M_PI_4);
        _backgroundView.layer.cornerRadius = 8;
        
        [self addSubview:_backgroundView];
    }
    return self;
}

- (void)setGlowing:(BOOL)glowing
{
    [UIView animateWithDuration:0.15 delay:0.0 options:UIViewAnimationOptionBeginFromCurrentState | UIViewAnimationOptionAllowUserInteraction animations:^{
        if(glowing)
        {
            self->_backgroundView.backgroundColor = [UIColor colorWithWhite:1 alpha:0.6];
        }
        else
        {
            self->_backgroundView.backgroundColor = [UIColor colorWithWhite:1 alpha:0.2];
        }
    } completion:nil];
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [super touchesBegan:touches withEvent:event];
    [self setGlowing:YES];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];
    [self setGlowing:NO];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
    [self setGlowing:NO];
}

#if DEBUG

- (void)dealloc
{
    NSLog(@"deallocated %@", self);
}

#endif /* DEBUG */

@end
