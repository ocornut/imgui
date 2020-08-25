// dear imgui: Platform Binding for X11 using xcb
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan..)

// This works as is with Vulkan. For OpenGL using GLX, you need a hybrid
// of XLib and xcb using the X11/Xlib-xcb.h header. Use XLib to create the
// GLX context, then use functions in Xlib-xcb.h to convert the XLib
// structures to xcb, which you can then pass unmodified here.
// Requires libxcb, libxcb-xfixes, libxcb-xkb1 libxcb-cursor0, libxcb-keysyms1 and libxcb-randr0

// Implemented features:
//  [X] Platform: Keyboard arrays indexed using XK symbols, e.g. ImGui::IsKeyPressed(XK_space).
//  [X] Platform: Clipboard support
//  [X] Platform: Mouse cursor shape and visibility.
//  [/] Platform: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.
// Missing features:
//  [ ] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#include <stdint.h>
#include <xcb/xcb.h>

IMGUI_IMPL_API bool     ImGui_ImplX11_Init(xcb_window_t window);
IMGUI_IMPL_API void     ImGui_ImplX11_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplX11_NewFrame();
IMGUI_IMPL_API bool     ImGui_ImplX11_ProcessEvent(xcb_generic_event_t* event);
