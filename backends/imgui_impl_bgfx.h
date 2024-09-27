// Derived from this Gist by Richard Gale:
//     https://gist.github.com/RichardGale/6e2b74bc42b3005e08397236e4be0fd0

// ImGui BGFX binding

// You can copy and use unmodified imgui_impl_* files in your project. See
// main.cpp for an example of using this. If you use this binding you'll need to
// call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(),
// ImGui::Render() and ImGui_ImplXXXX_Shutdown(). If you are new to ImGui, see
// examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#pragma once

#include <bgfx/bgfx.h>
#include <imgui.h>

void ImGui_Implbgfx_Init();
void ImGui_Implbgfx_Shutdown();
void ImGui_Implbgfx_NewFrame();
void ImGui_Implbgfx_RenderDrawLists(struct ImDrawData* draw_data, bgfx::ViewId view_id = 255);

/*                */
/* Multi Viewport */
/*                */
static void ImGui_Implbgfx_CreateWindow(ImGuiViewport* viewport);
static void ImGui_Implbgfx_DestroyWindow(ImGuiViewport* viewport);
static void ImGui_Implbgfx_SetWindowSize(ImGuiViewport* viewport, ImVec2 size);
static void ImGui_Implbgfx_RenderWindow(ImGuiViewport* viewport, void* renderArg);

// Use if you want to reset your rendering device without losing ImGui state.
void ImGui_Implbgfx_InvalidateDeviceObjects();
bool ImGui_Implbgfx_CreateDeviceObjects();
