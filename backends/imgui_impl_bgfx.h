// dear imgui: Renderer Backend for BgfX
// This needs to be used along with a Platform Backend (e.g. GLFW, SDL, Win32, custom..)

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#include <stdint.h>

#include <bgfx/bgfx.h>

// Backend API
IMGUI_IMPL_API bool     ImGui_ImplBgfx_Init(uint16_t viewId);
IMGUI_IMPL_API void     ImGui_ImplBgfx_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplBgfx_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplBgfx_RenderDrawData(ImDrawData* draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplBgfx_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplBgfx_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplBgfx_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplBgfx_DestroyDeviceObjects();

namespace ImGui
{
    IMGUI_IMPL_API void     Image(bgfx::TextureHandle handle, const ImVec2& size, const ImVec2& uv0 = ImVec2(0.0f, 0.0f), const ImVec2& uv1 = ImVec2(1.0f, 1.0f), const ImVec4& tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), const ImVec4& borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
}
