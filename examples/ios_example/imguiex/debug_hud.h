//
//  debug_hud.h
//  imguiex
//
//  Created by Joel Davis on 6/14/15.
//  Copyright (c) 2015 Joel Davis. All rights reserved.
//

#ifndef __imguiex__debug_hud__
#define __imguiex__debug_hud__

typedef struct DebugHUD
{
    int show_test_window;
    int show_example_window;
    float rotation_speed;
    float cubeColor1[4];
    float cubeColor2[4];
} DebugHUD;

#if __cplusplus
extern "C" {
#endif

void DebugHUD_InitDefaults( DebugHUD *hud );
void DebugHUD_DoInterface( DebugHUD *hud );

#if __cplusplus
}
#endif

#endif /* defined(__imguiex__debug_hud__) */
