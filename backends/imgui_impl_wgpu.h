// dear imgui: Renderer for WebGPU
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL2, SDL3)
// (Please note that WebGPU is a recent API, may not be supported by all browser, and its ecosystem is generally a mess)

// When targeting native platforms:
//  - One of IMGUI_IMPL_WEBGPU_BACKEND_DAWN or IMGUI_IMPL_WEBGPU_BACKEND_WGPU *must* be provided.
// When targeting Emscripten:
//  - We now defaults to IMGUI_IMPL_WEBGPU_BACKEND_DAWN is Emscripten version is 4.0.10+, which correspond to using Emscripten '--use-port=emdawnwebgpu'.
//  - We can still define IMGUI_IMPL_WEBGPU_BACKEND_WGPU to use Emscripten '-s USE_WEBGPU=1' which is marked as obsolete by Emscripten.
// Add #define to your imconfig.h file, or as a compilation flag in your build system.
// This requirement may be removed once WebGPU stabilizes and backends converge on a unified interface.
//#define IMGUI_IMPL_WEBGPU_BACKEND_DAWN
//#define IMGUI_IMPL_WEBGPU_BACKEND_WGPU

// Implemented features:
//  [X] Renderer: User texture binding. Use 'WGPUTextureView' as ImTextureID. Read the FAQ about ImTextureID/ImTextureRef!
//  [X] Renderer: Large meshes support (64k+ vertices) even with 16-bit indices (ImGuiBackendFlags_RendererHasVtxOffset).
//  [X] Renderer: Expose selected render state for draw callbacks to use. Access in '(ImGui_ImplXXXX_RenderState*)GetPlatformIO().Renderer_RenderState'.
//  [X] Renderer: Texture updates support for dynamic font system (ImGuiBackendFlags_RendererHasTextures).

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

// Setup Emscripten default if not specified.
#if defined(__EMSCRIPTEN__) && !defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN) && !defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
#include <emscripten/version.h>
#if (__EMSCRIPTEN_major__ >= 4) && (__EMSCRIPTEN_minor__ >= 0) && (__EMSCRIPTEN_tiny__ >= 10)
#define IMGUI_IMPL_WEBGPU_BACKEND_DAWN
#else
#define IMGUI_IMPL_WEBGPU_BACKEND_WGPU
#endif
#endif

#include <webgpu/webgpu.h>
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU) && !defined(__EMSCRIPTEN__)
#include <webgpu/wgpu.h>        // WGPULogLevel
#endif

// Initialization data, for ImGui_ImplWGPU_Init()
struct ImGui_ImplWGPU_InitInfo
{
    WGPUDevice              Device = nullptr;
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

//-------------------------------------------------------------------------
// Internal Helpers
// Those are currently used by our example applications.
//-------------------------------------------------------------------------

// (Optional) Helper to wrap some of the Dawn/WGPU/Emscripten quirks
bool        ImGui_ImplWGPU_IsSurfaceStatusError(WGPUSurfaceGetCurrentTextureStatus status);
bool        ImGui_ImplWGPU_IsSurfaceStatusSubOptimal(WGPUSurfaceGetCurrentTextureStatus status);    // Return whether the texture is suboptimal and may need to be recreated.

// (Optional) Helper for debugging/logging
void        ImGui_ImplWGPU_DebugPrintAdapterInfo(const WGPUAdapter& adapter);
const char* ImGui_ImplWGPU_GetBackendTypeName(WGPUBackendType type);
const char* ImGui_ImplWGPU_GetAdapterTypeName(WGPUAdapterType type);
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
const char* ImGui_ImplWGPU_GetDeviceLostReasonName(WGPUDeviceLostReason type);
const char* ImGui_ImplWGPU_GetErrorTypeName(WGPUErrorType type);
#elif defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU) && !defined(__EMSCRIPTEN__)
const char* ImGui_ImplWGPU_GetLogLevelName(WGPULogLevel level);
#endif

// (Optional) Helper to create a surface on macOS/Wayland/X11/Window
#ifndef __EMSCRIPTEN__
struct ImGui_ImplWGPU_CreateSurfaceInfo
{
    WGPUInstance    Instance;
    const char*     System;      // "cocoa"   | "wayland"   | "x11"     | "win32"
    void*           RawWindow;   // NSWindow* | 0           | Window    | HWND
    void*           RawDisplay;  // 0         | wl_display* | Display*  | 0
    void*           RawSurface;  //           | wl_surface* | 0         | 0
    void*           RawInstance; // 0         | 0           | 0         | HINSTANCE
};
WGPUSurface ImGui_ImplWGPU_CreateWGPUSurfaceHelper(ImGui_ImplWGPU_CreateSurfaceInfo* info);
#endif // #ifndef __EMSCRIPTEN__

#endif // #ifndef IMGUI_DISABLE
