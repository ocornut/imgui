// ImGui ORX binding by Denis Brachet (aka ainvar)
// In this binding, ImTextureID is used to store an orxBITMAP pointer. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#ifndef __IMGUI_ORX_HEADER__
#define __IMGUI_ORX_HEADER__

#include "../imgui/imgui.h"

// Initializes ImGui Orx system
IMGUI_API bool        ImGui_Orx_Init();

// Deinitializes ImGui Orx system
IMGUI_API void        ImGui_Orx_Shutdown();

// Initializes a new frame data. To call when starting rendering a new frame, before creating contents.
IMGUI_API void        ImGui_Orx_NewFrame();

// Renders the current frame To call after contents has been created
IMGUI_API void        ImGui_Orx_Render(void * pvViewport, ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_Orx_InvalidateDeviceObjects();

#endif //__IMGUI_ORX_HEADER__
