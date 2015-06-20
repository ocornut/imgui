//
//  imgui_impl_ios.h
//  imguiex
//
//  Joel Davis (joeld42@gmail.com)
//

#ifndef __imguiex__imgui_impl_ios__
#define __imguiex__imgui_impl_ios__

#include <Foundation/Foundation.h>
#include <UIKit/UIKit.h>

@interface ImGuiHelper : NSObject

- (id) initWithView: (UIView *)view;

- (void)connectServer: (NSString*)serverName;

- (void)render;
- (void)newFrame;

@end


#endif /* defined(__imguiex__imgui_impl_ios__) */
