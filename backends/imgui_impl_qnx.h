// dear imgui: Platform Backend for QNX Screen
// This needs to be used along with a Renderer Backend (e.g. OpenGL3, Vulkan).

// Implemented features:
//  [X] Platform: Keyboard support. Uses io.AddKeyEvent() and native QNX key symbols.
//  [X] Platform: Mouse support. Handles position, three buttons and horizontal/vertical wheels.
//  [X] Platform: Touch support. Maps the first active touch contact to the primary mouse button.
//  [X] Platform: Focus and window-size updates.
// Missing features or Issues:
//  [ ] Platform: Clipboard support.
//  [ ] Platform: Gamepad support.
//  [ ] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors).

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

#include <screen/screen.h>

// The application owns the Screen context, window and event object.
// Poll events with screen_get_event() and pass each non-NONE event to ProcessEvent().
IMGUI_IMPL_API bool     ImGui_ImplQNX_Init(screen_context_t context, screen_window_t window);
IMGUI_IMPL_API void     ImGui_ImplQNX_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplQNX_NewFrame();
IMGUI_IMPL_API bool     ImGui_ImplQNX_ProcessEvent(screen_event_t event);

#endif // #ifndef IMGUI_DISABLE
