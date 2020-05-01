// dear imgui: Platform Binding for OSX / Cocoa
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3, Vulkan, Metal..)
// [ALPHA] Early bindings, not well tested. If you want a portable application, prefer using the GLFW or SDL platform bindings on Mac.

// Implemented features:
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: OSX clipboard is supported within core Dear ImGui (no specific code in this back-end).
// Issues:
//  [ ] Platform: Keys are all generally very broken. Best using [event keycode] and not [event characters]..
//  [ ] Platform: Multi-viewport / platform windows.

#include "imgui.h"      // IMGUI_IMPL_API

@class NSEvent;
@class NSView;

IMGUI_IMPL_API bool     ImGui_ImplOSX_Init();
IMGUI_IMPL_API void     ImGui_ImplOSX_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplOSX_NewFrame(NSView *_Nullable view);
IMGUI_IMPL_API bool     ImGui_ImplOSX_HandleEvent(NSEvent *_Nonnull event, NSView *_Nullable view);
