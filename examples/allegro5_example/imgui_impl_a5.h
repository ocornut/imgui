// public domain

#pragma once

struct ALLEGRO_DISPLAY;
union ALLEGRO_EVENT;

bool ImGui_ImplA5_Init(ALLEGRO_DISPLAY *disp);
void ImGui_ImplA5_ProcessEvent(ALLEGRO_EVENT *ev);
void ImGui_ImplA5_Shutdown();

void ImGui_ImplA5_NewFrame();

bool Imgui_ImplA5_CreateDeviceObjects();
void ImGui_ImplA5_InvalidateDeviceObjects();

