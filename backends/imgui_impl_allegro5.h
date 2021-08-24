// dear imgui: Renderer + Platform Backend for Allegro 5
// (Info: Allegro 5 is a cross-platform general purpose library for handling windows, inputs, graphics, etc.)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'ALLEGRO_BITMAP*' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Platform: Clipboard support (from Allegro 5.1.12)
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
// Issues:
//  [ ] Renderer: The renderer is suboptimal as we need to unindex our buffers and convert vertices manually.
//  [ ] Platform: Missing gamepad support.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this. 
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

struct ALLEGRO_DISPLAY;
union ALLEGRO_EVENT;

IMGUI_IMPL_API bool     ImGui_ImplAllegro5_Init(ALLEGRO_DISPLAY* display);
IMGUI_IMPL_API void     ImGui_ImplAllegro5_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplAllegro5_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplAllegro5_RenderDrawData(ImDrawData* draw_data);
IMGUI_IMPL_API bool     ImGui_ImplAllegro5_ProcessEvent(ALLEGRO_EVENT* event);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API bool     ImGui_ImplAllegro5_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplAllegro5_InvalidateDeviceObjects();
