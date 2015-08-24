// ImGui Allegro 5 bindings
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
