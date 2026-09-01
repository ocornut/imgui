// dear imgui: Null Platform+Renderer Backends
// This is designed if you need to use a blind Dear Imgui context with no input and no output.

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

// Null = NullPlatform + NullRender
IMGUI_IMPL_API bool     ImGui_ImplNull_Init();
IMGUI_IMPL_API void     ImGui_ImplNull_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplNull_NewFrame();

// Null platform only (single screen, fixed timestep, no inputs)
IMGUI_IMPL_API bool     ImGui_ImplNullPlatform_Init();
IMGUI_IMPL_API void     ImGui_ImplNullPlatform_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplNullPlatform_NewFrame();

// Null renderer only (no output)
IMGUI_IMPL_API bool     ImGui_ImplNullRender_Init();
IMGUI_IMPL_API void     ImGui_ImplNullRender_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplNullRender_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplNullRender_RenderDrawData(ImDrawData* draw_data);

#endif // #ifndef IMGUI_DISABLE
