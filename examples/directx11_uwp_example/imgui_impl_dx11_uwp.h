#pragma once

// ImGui UWP + DirectX11 binding
// In this binding, ImTextureID is used to store a 'ID3D11ShaderResourceView*' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "imgui.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

IMGUI_API bool        ImGui_ImplDX11_Init(ID3D11Device* device, ID3D11DeviceContext* device_context);
IMGUI_API void        ImGui_ImplDX11_Shutdown();
IMGUI_API void        ImGui_ImplDX11_NewFrame(float width, float height);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplDX11_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplDX11_CreateDeviceObjects();

// Handler for input events, update mouse/keyboard data.
// You may or not need this for your implementation, but it can serve as reference for handling inputs.

// If you start with the Visual Studio 2015 project wizard generated D3D11 UWP application you
// can add a *reference* to an ImGuiInputEventHandler in the App class and create the instance
// in the App::SetWindow() method.

using namespace Windows::UI::Core;

ref class ImGuiInputEventHandler sealed
{

public:

    ImGuiInputEventHandler(CoreWindow^ window);

private:

    void SetMouseButtonState(Windows::UI::Core::PointerEventArgs ^args);

    void OnPointerPressed(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::PointerEventArgs ^args);
    void OnPointerReleased(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::PointerEventArgs ^args);
    void OnPointerMoved(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::PointerEventArgs ^args);
    void OnPointerWheelChanged(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::PointerEventArgs ^args);
    void OnKeyDown(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args);
    void OnKeyUp(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args);
    void OnCharacterReceived(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::CharacterReceivedEventArgs ^args);

};

