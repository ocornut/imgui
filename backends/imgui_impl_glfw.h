// dear imgui: Platform Backend for GLFW
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan, WebGPU..)
// (Info: GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (Requires: GLFW 3.1+. Prefer GLFW 3.3+ for full feature support.)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy GLFW_KEY_* values will also be supported unless IMGUI_DISABLE_OBSOLETE_KEYIO is set]
//  [X] Platform: Gamepad support. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [x] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange' (note: the resizing cursors requires GLFW 3.4+).
//  [X] Platform: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.

// Issues:
//  [ ] Platform: Multi-viewport support: ParentViewportID not honored, and so io.ConfigViewportsNoDefaultParent has no effect (minor).

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// About GLSL version:
// The 'glsl_version' initialization parameter defaults to "#version 150" if NULL.
// Only override if your GL version doesn't handle this GLSL version. Keep NULL if unsure!

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

struct GLFWwindow;
struct GLFWmonitor;

IMGUI_IMPL_API bool     ImGui_ImplGlfw_InitForOpenGL(GLFWwindow* window, bool install_callbacks);
IMGUI_IMPL_API bool     ImGui_ImplGlfw_InitForVulkan(GLFWwindow* window, bool install_callbacks);
IMGUI_IMPL_API bool     ImGui_ImplGlfw_InitForOther(GLFWwindow* window, bool install_callbacks);
IMGUI_IMPL_API void     ImGui_ImplGlfw_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplGlfw_NewFrame();

// GLFW callbacks
// - When calling Init with 'install_callbacks=true': GLFW callbacks will be installed for you. They will call user's previously installed callbacks, if any.
// - When calling Init with 'install_callbacks=false': GLFW callbacks won't be installed. You will need to call those function yourself from your own GLFW callbacks.
IMGUI_IMPL_API void     ImGui_ImplGlfw_WindowFocusCallback(GLFWwindow* window, int focused);        // Since 1.84
IMGUI_IMPL_API void     ImGui_ImplGlfw_CursorEnterCallback(GLFWwindow* window, int entered);        // Since 1.84
IMGUI_IMPL_API void     ImGui_ImplGlfw_CursorPosCallback(GLFWwindow* window, double x, double y);   // Since 1.87
IMGUI_IMPL_API void     ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
IMGUI_IMPL_API void     ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
IMGUI_IMPL_API void     ImGui_ImplGlfw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
IMGUI_IMPL_API void     ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c);
IMGUI_IMPL_API void     ImGui_ImplGlfw_MonitorCallback(GLFWmonitor* monitor, int event);
