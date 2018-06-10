//
//  debug_hud.h
//  imguiex

#pragma once

typedef struct DebugHUD
{
    bool    show_demo_window;
    bool    show_another_window;
    float   rotation_speed;
    float   cubeColor1[4];
    float   cubeColor2[4];
    float   clearColor[4];
} DebugHUD;

#if __cplusplus
extern "C" {
#endif

void DebugHUD_InitDefaults(DebugHUD *hud);
void DebugHUD_DoInterface(DebugHUD *hud);

#if __cplusplus
}
#endif
