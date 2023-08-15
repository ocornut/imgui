// dear imgui: Platform Backend for Universal Windows Platform
// This needs to be used along with a Renderer (e.g. DirectX11, DirectX12, SDL2..)

// Implemented features:
//  [X] Platform: Clipboard support (for Win32 & UWP this is actually part of core dear imgui)
//  [X] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen.
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy VK_* values will also be supported unless IMGUI_DISABLE_OBSOLETE_KEYIO is set]
//  [X] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.

// Configs:
//  #define IMGUI_DISABLE_UWP_DEFAULT_CLIPBOARD_FUNCTIONS     // Don't implement default WinRT clipboard handler.
//  #define IMGUI_IMPL_UWP_DISABLE_GAMEPAD                    // Don't implement default XInput gamepad handler.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

IMGUI_IMPL_API bool     ImGui_ImplUwp_Init(void* core_window); // ABI::Windows::UI::Core::ICoreWindow*
IMGUI_IMPL_API bool     ImGui_ImplUwp_InitForCurrentView();
IMGUI_IMPL_API void     ImGui_ImplUwp_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplUwp_NewFrame();
