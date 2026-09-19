// dear imgui: Platform Backend for Emscripten
// This needs to be used along with a Renderer (e.g. OpenGL3)

// Implemented features:
//  [X] Platform: Clipboard support (with IMGUI_IMPL_EMSCRIPTEN_ENABLE_CLIPBOARD).
//  [X] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen (with IMGUI_IMPL_EMSCRIPTEN_POINTER_EVENTS).
//  [X] Platform: Keyboard support.
//  [X] Platform: Gamepad support.
//  [X] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors). Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.

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

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool     ImGui_ImplEmscripten_Init(const char* target_id);
IMGUI_IMPL_API void     ImGui_ImplEmscripten_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplEmscripten_NewFrame();

IMGUI_IMPL_API void     ImGui_ImplEmscripten_UpdateCanvasSize(int width, int height);

#endif // #ifndef IMGUI_DISABLE
