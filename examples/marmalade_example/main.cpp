// ImGui - standalone example application for Marmalade
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

// Copyright (C) 2015 by Giovanni Zito
// This file is part of ImGui

#include <imgui.h>
#include "imgui_impl_marmalade.h"
#include <stdio.h>

#include <s3eKeyboard.h>
#include <s3ePointer.h>
#include <IwGx.h>

int main(int, char**)
{
    // Setup ImGui binding
    ImGui_Marmalade_Init(true);

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);

    // Main loop
    while (true)
    {
         if (s3eDeviceCheckQuitRequest())
             break;

         s3eKeyboardUpdate();
         s3ePointerUpdate();
         ImGui_Marmalade_NewFrame();

        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        {
            static float f = 0.0f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello from another window!");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

        // Rendering
        IwGxSetColClear(clear_color.x*255,clear_color.y*255,clear_color.z*255,clear_color.w*255) ;
        IwGxClear();
        ImGui::Render();
        IwGxSwapBuffers();

        s3eDeviceYield(0);
    }

    // Cleanup
    ImGui_Marmalade_Shutdown();

    return 0;
}
