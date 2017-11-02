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
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them. 
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple. 
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'extra_fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (true)
    {
         if (s3eDeviceCheckQuitRequest())
             break;

         // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
         // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
         // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
         // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
         s3eKeyboardUpdate();
         s3ePointerUpdate();
         ImGui_Marmalade_NewFrame();

        // 1. Show a simple window.
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug".
        {
            static float f = 0.0f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }

        // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name the window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello from another window!");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow().
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
