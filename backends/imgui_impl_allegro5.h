// dear imgui: Renderer + Platform Backend for Allegro 5
// (Info: Allegro 5 is a cross-platform general purpose library for handling windows, inputs, graphics, etc.)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'ALLEGRO_BITMAP*' as texture identifier. Read the FAQ about ImTextureID/ImTextureRef!
//  [X] Renderer: Texture updates support for dynamic font atlas (ImGuiBackendFlags_RendererHasTextures).
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy ALLEGRO_KEY_* values are obsolete since 1.87 and not supported since 1.91.5]
//  [X] Platform: Clipboard support (from Allegro 5.1.12).
//  [X] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors). Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
// Missing features or Issues:
//  [ ] Renderer: The renderer is suboptimal as we need to unindex our buffers and convert vertices manually.
//  [ ] Platform: Missing gamepad support.
//  [ ] Renderer: Multi-viewport support (multiple windows).

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

struct ALLEGRO_DISPLAY;
union ALLEGRO_EVENT;

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool     ImGui_ImplAllegro5_Init(ALLEGRO_DISPLAY* display);
IMGUI_IMPL_API void     ImGui_ImplAllegro5_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplAllegro5_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplAllegro5_RenderDrawData(ImDrawData* draw_data);
IMGUI_IMPL_API bool     ImGui_ImplAllegro5_ProcessEvent(ALLEGRO_EVENT* event);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API bool     ImGui_ImplAllegro5_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplAllegro5_InvalidateDeviceObjects();

// (Advanced) Use e.g. if you need to precisely control the timing of texture updates (e.g. for staged rendering), by setting ImDrawData::Textures = NULL to handle this manually.
IMGUI_IMPL_API void     ImGui_ImplAllegro5_UpdateTexture(ImTextureData* tex);

#endif // #ifndef IMGUI_DISABLE
