// dear imgui: Renderer for WebGPU
// This needs to be used along with a Platform Binding (e.g. GLFW)
// (Please note that WebGPU is currently experimental, will not run on non-beta browsers, and may break.)

// Important note to dawn and/or wgpu users: when targeting native platforms (i.e. NOT emscripten),
// one of IMGUI_IMPL_WEBGPU_BACKEND_DAWN or IMGUI_IMPL_WEBGPU_BACKEND_WGPU must be provided.
// Add #define to your imconfig.h file, or as a compilation flag in your build system.
// This requirement will be removed once WebGPU stabilizes and backends converge on a unified interface.
//#define IMGUI_IMPL_WEBGPU_BACKEND_DAWN
//#define IMGUI_IMPL_WEBGPU_BACKEND_WGPU

// Implemented features:
//  [X] Renderer: User texture binding. Use 'WGPUTextureView' as ImTextureID. Read the FAQ about ImTextureID/ImTextureRef!
//  [X] Renderer: Large meshes support (64k+ vertices) even with 16-bit indices (ImGuiBackendFlags_RendererHasVtxOffset).
//  [X] Renderer: Expose selected render state for draw callbacks to use. Access in '(ImGui_ImplXXXX_RenderState*)GetPlatformIO().Renderer_RenderState'.
//  [X] Renderer: Texture updates support for dynamic font system (ImGuiBackendFlags_RendererHasTextures).
// Missing features or Issues:
//  [ ] Renderer: Multi-viewport support (multiple windows), useful for desktop.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#pragma once
#include "imgui.h"          // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

#include <webgpu/webgpu.h>

// Initialization data, for ImGui_ImplWGPU_Init()
struct ImGui_ImplWGPU_InitInfo
{
    WGPUDevice              Device;
    int                     NumFramesInFlight = 3;
    WGPUTextureFormat       RenderTargetFormat = WGPUTextureFormat_Undefined;
    WGPUTextureFormat       DepthStencilFormat = WGPUTextureFormat_Undefined;
    WGPUMultisampleState    PipelineMultisampleState = {};

    ImGui_ImplWGPU_InitInfo()
    {
        PipelineMultisampleState.count = 1;
        PipelineMultisampleState.mask = UINT32_MAX;
        PipelineMultisampleState.alphaToCoverageEnabled = false;
    }
};

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool ImGui_ImplWGPU_Init(ImGui_ImplWGPU_InitInfo* init_info);
IMGUI_IMPL_API void ImGui_ImplWGPU_Shutdown();
IMGUI_IMPL_API void ImGui_ImplWGPU_NewFrame();
IMGUI_IMPL_API void ImGui_ImplWGPU_RenderDrawData(ImDrawData* draw_data, WGPURenderPassEncoder pass_encoder);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API bool ImGui_ImplWGPU_CreateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplWGPU_InvalidateDeviceObjects();

// (Advanced) Use e.g. if you need to precisely control the timing of texture updates (e.g. for staged rendering), by setting ImDrawData::Textures = NULL to handle this manually.
IMGUI_IMPL_API void ImGui_ImplWGPU_UpdateTexture(ImTextureData* tex);

// [BETA] Selected render state data shared with callbacks.
// This is temporarily stored in GetPlatformIO().Renderer_RenderState during the ImGui_ImplWGPU_RenderDrawData() call.
// (Please open an issue if you feel you need access to more data)
struct ImGui_ImplWGPU_RenderState
{
    WGPUDevice                  Device;
    WGPURenderPassEncoder       RenderPassEncoder;
};

#endif // #ifndef IMGUI_DISABLE
