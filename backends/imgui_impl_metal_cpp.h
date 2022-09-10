#pragma once

// dear imgui: Renderer backend for metal-cpp
// this needs to be used along with a platform backend (e.g. MacOS)
// ------------------------------------------------------------------------------------------------
// NOTE
// -----
// This is a slightly modified version of the objc metal backend.
// This is expected to be used with metal-cpp
// metal-cpp is a low overhead (headers only) C++ interface for metal
// Download the headers and find more info at https://developer.apple.com/metal/cpp/
// ------------------------------------------------------------------------------------------------
// EXAMPLE
// --------
// void onStart() {
//      ImGui_ImplMetal_Init(device);
//      ImGui_ImplSDL2_InitForMetal(window);
// }
// void onMainLoop() {
//      ImGui_ImplMetal_NewFrame(renderPassDescriptor);
//      ImGui_ImplSDL2_NewFrame();
//      ImGui::NewFrame();
//
//      if (ImGui::Begin("Example", (bool*)0, 0)) {
//          ImGui::Text("Hello world");
//          ...
//      }
//      ImGui::End();
//
//      ImGui::EndFrame();
//      ImGui::Render();
//      ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, encoder);
// }
// void onEnd() {
//      ImGui_ImplMetal_Shutdown();
//      ImGui_ImplSDL2_Shutdown();
//      ImGui::DestroyContext();
// }
// ------------------------------------------------------------------------------------------------

#include "imgui.h"

#if defined(__APPLE__)

namespace MTL {
class Device;
class RenderPassDescriptor;
class CommandBuffer;
class RenderCommandEncoder;
}

IMGUI_IMPL_API bool
ImGui_ImplMetal_Init(MTL::Device* device);

IMGUI_IMPL_API void
ImGui_ImplMetal_Shutdown();

IMGUI_IMPL_API void
ImGui_ImplMetal_NewFrame(MTL::RenderPassDescriptor* renderPassDescriptor);

IMGUI_IMPL_API void
ImGui_ImplMetal_RenderDrawData(ImDrawData*                drawData,
                               MTL::CommandBuffer*        commandBuffer,
                               MTL::RenderCommandEncoder* commandEncoder);

#endif // #if defined(__APPLE__)
