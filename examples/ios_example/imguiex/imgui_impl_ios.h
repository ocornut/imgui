//
//  imgui_impl_ios.h
//  imguiex
//
//  Joel Davis (joeld42@gmail.com)
//

#pragma once

#include <Foundation/Foundation.h>
#include <UIKit/UIKit.h>

@interface ImGuiHelper : NSObject

- (id) initWithView: (UIView *)view;

- (void)connectServer: (NSString*)serverName;

- (void)render;
- (void)newFrame;

@end
