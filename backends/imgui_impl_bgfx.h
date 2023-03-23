// dear imgui: Renderer Backend for BGFX with shaders / programmatic pipeline
// This needs to be used along with a Platform Backend (e.g. GLFW, SDL, Win32, custom...)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'bgfx::TextureHandle' library texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID!

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#pragma once
#include "imgui.h"
#include "bgfx/bgfx.h"

// Backend API
IMGUI_IMPL_API bool     ImGui_ImplBgfx_Init(bgfx::ViewId view_id);
IMGUI_IMPL_API void     ImGui_ImplBgfx_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplBgfx_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplBgfx_RenderDrawData(ImDrawData* draw_data);

// Extra
IMGUI_IMPL_API bool     ImGui_ImplBgfx_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplBgfx_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplBgfx_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplBgfx_DestroyDeviceObjects();