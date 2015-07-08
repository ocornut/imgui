// ImGui Allegro 5 bindings
// https://github.com/ocornut/imgui
// by @birthggd

#pragma once

struct ALLEGRO_DISPLAY;
union ALLEGRO_EVENT;

bool    ImGui_ImplA5_Init(ALLEGRO_DISPLAY* display);
void    ImGui_ImplA5_Shutdown();
void    ImGui_ImplA5_NewFrame();

bool    ImGui_ImplA5_ProcessEvent(ALLEGRO_EVENT* event);

// Use if you want to reset your rendering device without losing ImGui state.
bool    Imgui_ImplA5_CreateDeviceObjects();
void    ImGui_ImplA5_InvalidateDeviceObjects();
