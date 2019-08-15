// dear imgui: Platform Binding for SFML
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3..)
// (Info: SFML is a cross-platform general purpose library for handling windows, inputs, OpenGL graphics context creation, etc.)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Gamepad support. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [x] Platform: Mouse cursor shape (with SFML > 2.5) and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'. FIXME: 3 cursors types are missing from GLFW.
//  [X] Platform: Keyboard arrays indexed using sf::Keyboard::* codes, e.g. ImGui::IsKeyPressed(sf::Keyboard::A).

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#pragma once

namespace sf
{
class Event;
class Window;
}

IMGUI_IMPL_API bool     ImGui_ImplSfml_Init(sf::Window* window, unsigned int joystickIndex = 0);
IMGUI_IMPL_API void     ImGui_ImplSfml_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplSfml_NewFrame();

IMGUI_IMPL_API void     ImGui_ImplSfml_ProcessEvent(const sf::Event &event);
