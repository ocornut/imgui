// dear imgui: Renderer Backend for Windows GDI
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [x] Renderer: Basic Implementation

// The aim of imgui_impl_gdi.h/.cpp is to be usable in your engine without any modification.
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

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
IMGUI_IMPL_API bool     ImGui_ImplGDI_Init();
IMGUI_IMPL_API void     ImGui_ImplGDI_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplGDI_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplGDI_RenderDrawData(ImDrawData* draw_data, void* fb_dev_ctx_handle, ImVec4* clear_color); // HDC fb_dev_ctx_handle

#endif // #ifndef IMGUI_DISABLE
