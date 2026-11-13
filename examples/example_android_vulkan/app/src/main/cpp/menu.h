#pragma once
// ═════════════════════════════════════════════════════════════════════════════
//  menu.h — ImGui UI layout
//
//  Edit this file to change what's drawn. Called once per frame from main.cpp.
// ═════════════════════════════════════════════════════════════════════════════

#include "imgui.h"

namespace menu
{
    static bool  showDemo    = true;
    static ImVec4 clearColor = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);

    inline void Draw()
    {
        if (showDemo)
            ImGui::ShowDemoWindow(&showDemo);

        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, Vulkan!");
            ImGui::Text("Standalone Vulkan + ImGui app.");
            ImGui::Checkbox("Demo Window", &showDemo);
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clearColor);
            if (ImGui::Button("Button")) counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }
    }

} // namespace menu
