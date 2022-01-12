// dear imgui: Platform Backend for OSX / Cocoa
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3, Vulkan, Metal..)
// [ALPHA] Early backend, not well tested. If you want a portable application, prefer using the GLFW or SDL platform Backends on Mac.

// Implemented features:
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy kVK_* values will also be supported unless IMGUI_DISABLE_OBSOLETE_KEYIO is set]
//  [X] Platform: OSX clipboard is supported within core Dear ImGui (no specific code in this backend).
//  [X] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: IME support.
//
// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"      // IMGUI_IMPL_API

@class NSEvent;
@class NSView;

IMGUI_IMPL_API bool     ImGui_ImplOSX_Init(NSView* _Nonnull view);
IMGUI_IMPL_API void     ImGui_ImplOSX_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplOSX_NewFrame(NSView* _Nullable view);
IMGUI_IMPL_API bool     ImGui_ImplOSX_HandleEvent(NSEvent* _Nonnull event, NSView* _Nullable view);
