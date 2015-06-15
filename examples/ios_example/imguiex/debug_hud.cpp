//
//  debug_hud.cpp
//  imguiex
//
//  Created by Joel Davis on 6/14/15.
//  Copyright (c) 2015 Joel Davis. All rights reserved.
//

#include "debug_hud.h"
#include "imgui.h"

void DebugHUD_InitDefaults( DebugHUD *hud )
{
    hud->show_test_window = 1;
}

void DebugHUD_DoInterface( DebugHUD *hud )
{
//    if (hud->show_test_window)
//    {
        ImGui::SetNextWindowPos( ImVec2( 20, 20 ), ImGuiSetCond_FirstUseEver );
        bool show_test_window = hud->show_test_window;
        ImGui::ShowTestWindow( &show_test_window );
        hud->show_test_window = show_test_window;
//    }

}