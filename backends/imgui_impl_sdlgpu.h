#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

#include <SDL3/SDL_gpu.h>
#include <string>
struct ImGui_ImplSDLGPU_InitInfo
{
    SDL_GPUDevice*       GpuDevice          = nullptr;
    SDL_GPUTextureFormat ColorTargetFormat  = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUSampleCount   MSAASamples        = SDL_GPU_SAMPLECOUNT_1;
};

IMGUI_IMPL_API bool             ImGui_ImplSDLGPU_Init(ImGui_ImplSDLGPU_InitInfo* info);
IMGUI_IMPL_API void             ImGui_ImplSDLGPU_Shutdown();
IMGUI_IMPL_API void             ImGui_ImplSDLGPU_NewFrame();
IMGUI_IMPL_API void             Imgui_ImplSDLGPU_PrepareDrawData(ImDrawData* draw_data, SDL_GPUCommandBuffer* command_buffer);
IMGUI_IMPL_API void             ImGui_ImplSDLGPU_RenderDrawData(ImDrawData* draw_data, SDL_GPUCommandBuffer* command_buffer, SDL_GPURenderPass* render_pass);
IMGUI_IMPL_API bool             ImGui_ImplSDLGPU_CreateFontsTexture();
IMGUI_IMPL_API void             ImGui_ImplSDLGPU_DestroyFontsTexture();

#endif // #ifndef IMGUI_DISABLE