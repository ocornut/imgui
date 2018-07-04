// ImGui Renderer + Platform Binding for: Allegro 5
// (Info: Allegro 5 is a cross-platform general purpose library for handling windows, inputs, graphics, etc.)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'ALLEGRO_BITMAP*' as ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
// Issues:
//  [ ] Renderer: The renderer is suboptimal as we need to convert vertices.
//  [ ] Platform: Missing clipboard support via al_set_clipboard_text/al_clipboard_has_text.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui, Original code by @birthggd

#pragma once

struct ALLEGRO_DISPLAY;
union ALLEGRO_EVENT;

IMGUI_IMPL_API bool     ImGui_ImplAllegro5_Init(ALLEGRO_DISPLAY* display);
IMGUI_IMPL_API void     ImGui_ImplAllegro5_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplAllegro5_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplAllegro5_RenderDrawData(ImDrawData* draw_data);
IMGUI_IMPL_API bool     ImGui_ImplAllegro5_ProcessEvent(ALLEGRO_EVENT* event);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_IMPL_API bool     ImGui_ImplAllegro5_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplAllegro5_InvalidateDeviceObjects();
