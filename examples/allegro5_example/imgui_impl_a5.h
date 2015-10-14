// ImGui Allegro 5 bindings
// You can copy and use unmodified imgui_impl_* files in your project. 
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// See main.cpp for an example of using this.
// https://github.com/ocornut/imgui
// by @birthggd

#pragma once

struct ALLEGRO_DISPLAY;
union ALLEGRO_EVENT;

IMGUI_API bool    ImGui_ImplA5_Init(ALLEGRO_DISPLAY* display);
IMGUI_API void    ImGui_ImplA5_Shutdown();
IMGUI_API void    ImGui_ImplA5_NewFrame();
IMGUI_API bool    ImGui_ImplA5_ProcessEvent(ALLEGRO_EVENT* event);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API bool    Imgui_ImplA5_CreateDeviceObjects();
IMGUI_API void    ImGui_ImplA5_InvalidateDeviceObjects();
