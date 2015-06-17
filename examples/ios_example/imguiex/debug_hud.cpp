//
//  debug_hud.cpp
//  imguiex
//
//  Created by Joel Davis on 6/14/15.
//  Copyright (c) 2015 Joel Davis. All rights reserved.
//

#include <stdio.h>

#include "debug_hud.h"
#include "imgui.h"

void DebugHUD_InitDefaults( DebugHUD *hud )
{
    hud->show_test_window = 1;
    hud->show_example_window = 1;
    hud->rotation_speed = 15.0;
    
    hud->cubeColor1[0] = 0.4;
    hud->cubeColor1[1] = 0.4;
    hud->cubeColor1[2] = 1.0;
    hud->cubeColor1[3] = 1.0;
    
    hud->cubeColor2[0] = 1.0;
    hud->cubeColor2[1] = 0.4;
    hud->cubeColor2[2] = 0.4;
    hud->cubeColor2[3] = 1.0;
    
}

void DebugHUD_DoInterface( DebugHUD *hud )
{
    if (hud->show_test_window)
    {
        ImGui::SetNextWindowPos( ImVec2( 400, 20 ), ImGuiSetCond_FirstUseEver );
        bool show_test_window = hud->show_test_window;
        ImGui::ShowTestWindow( &show_test_window );
        hud->show_test_window = show_test_window;
    }
    
    if (hud->show_example_window)
    {
        bool show_window = hud->show_example_window;
        ImGui::SetNextWindowPos( ImVec2( 20, 20 ), ImGuiSetCond_FirstUseEver );
        ImGui::SetNextWindowSize( ImVec2( 350, 200 ), ImGuiSetCond_FirstUseEver );
        ImGui::Begin("Another Window", &show_window);
        hud->show_example_window = show_window;
        
        ImGui::ColorEdit3("Cube 1 Color", hud->cubeColor1);
        ImGui::ColorEdit3("Cube 2 Color", hud->cubeColor2);
        ImGui::SliderFloat("Rotation Speed", &(hud->rotation_speed), 0.0f, 200.0f);

        ImGui::End();
    }

}