// dear imgui: Platform Backend for Emscripten HTML5
//
// This is a platform back-end, similar to and offering an alternative to imgui_impl_glfw.
// The intended use-case is for applications built with Emscripten, running in the browser, but *not* using GLFW.
// It uses Emscripten's HTML5 interface to tie callbacks to imgui input, handling window resizing,
// focus, cursor, keyboard input, etc.  It does not attempt to handle rendering.
//
// A note about GLFW on Emscripten: Emscripten includes its own GLFW implementation, which wraps browser HTML5 callbacks to provide the standard GLFW input interface.  So there are two levels of indirection.
// This backend removes the middleman for input, providing a more efficient direct interface between Emscripten's functionality and imgui input.
//
// This is a useful accompaniment for WebGPU rendering (i.e. with imgui_impl_wgpu), where GLFW is not needed for rendering.
// In that case, this backend replaces all non-rendering-related functionality from GLFW, making it possible to avoid depending on GLFW altogether.
//
// For native cursor rendering, this includes a cut-down implementation of the Emscripten Browser Cursor library: https://github.com/Armchair-Software/emscripten-browser-cursor

// Supported features:
// - Keyboard input
// - Window resizing
// - Cursor position
// - Cursor enters and leaves the window
// - Application focus
// - Browser cursors

// TODO:
// - Touch events

// A note on gamepad input: This back-end does not attempt to handle gamepad events, for the simple
// reason that any time you intend to provide gamepad input to imgui, you will inevitably want to
// also use gamepad input in your own game logic, so duplicating this processing can add a lot of
// inefficiency.  For an example of how to handle Emscripten HTML5 gamepad events efficiently, and
// pass the relevant events to imgui, see https://github.com/Armchair-Software/webgpu-demo2
// Don't forget to set io.BackendFlags |= ImGuiBackendFlags_HasGamepad when a gamepad is connected.

#pragma once
#include "imgui.h"

#ifndef IMGUI_DISABLE

#ifndef __EMSCRIPTEN__
#error The imgui_impl_emscripten backend requires Emscripten.
#endif

// Controls how many device pixels Dear ImGui should target per Dear ImGui pixel.
// Default 1.0f gives 1:1 device-pixel rendering. Set before Init() if you want a different scaling policy.
extern IMGUI_IMPL_API float ImGui_ImplEmscripten_TargetDevicePixelRatio;

// Initialise the Emscripten backend, setting input callbacks.  This should be called after ImGui::CreateContext();
IMGUI_IMPL_API void ImGui_ImplEmscripten_Init();

// Shut down the Emscripten backend.  This unsets all Emscripten input callbacks set by Init.
// Note it'll also unset any Emscripten input callbacks set elsewhere in the program!
// Note also there is no obligation to ever call this, unless you intend to reset the backend (i.e. with ImGui::DestroyContext()).
// Otherwise, there is not necessarily any such concept as "shutting down" when running in the browser, and we have no resources to release.  The user can just close the tab, so you don't need to worry about exiting cleanly.
IMGUI_IMPL_API void ImGui_ImplEmscripten_Shutdown();

// Call every frame to synchronize Dear ImGui's cursor state with the browser's native cursors.
// If you are not using browser native cursor rendering (i.e. if Dear ImGui is rendering cursors internally), you don't need to call this.
IMGUI_IMPL_API void ImGui_ImplEmscripten_NewFrame();

#endif // IMGUI_DISABLE
