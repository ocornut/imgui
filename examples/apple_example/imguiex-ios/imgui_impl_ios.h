// ImGui iOS+OpenGL+Synergy binding
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.
// Providing a standalone iOS application with Synergy integration makes this sample more verbose than others. It also hasn't been tested as much.
// Refer to other examples to get an easier understanding of how to integrate ImGui into your existing application.

// by Joel Davis (joeld42@gmail.com)

#pragma once

#include <Foundation/Foundation.h>
#include <UIKit/UIKit.h>

@interface ImGuiHelper : NSObject

- (id) initWithView: (UIView *)view;

- (void)connectServer: (NSString*)serverName;

- (void)render;
- (void)newFrame;

@end
