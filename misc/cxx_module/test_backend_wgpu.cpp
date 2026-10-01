// dear imgui: compile-only test for the C++20 backend module 'imgui.impl.wgpu' (Dawn or wgpu-native headers, no library needed).

#include <webgpu/webgpu.h>      // application-side platform headers
import std;
import imgui;
import imgui.impl.wgpu;

static_assert(sizeof(ImGui_ImplWGPU_InitInfo) > 0);

void RenderFrameWGPU(WGPUDevice device, WGPURenderPassEncoder pass_encoder)
{
    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = device;
    init_info.RenderTargetFormat = WGPUTextureFormat_BGRA8Unorm;
    ImGui_ImplWGPU_Init(&init_info);
    ImGui_ImplWGPU_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass_encoder);
    std::println("{}", ImGui_ImplWGPU_GetBackendTypeName(WGPUBackendType_Vulkan));
}
