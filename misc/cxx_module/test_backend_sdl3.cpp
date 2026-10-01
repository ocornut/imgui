// dear imgui: run test for the C++20 backend modules 'imgui.impl.sdl3' + 'imgui.impl.sdlrenderer3' + 'imgui.impl.sdlgpu3'.
// Runs sdl3 + sdlrenderer3 headless using SDL's "dummy" video driver and the software renderer. sdlgpu3 is compiled/linked only.
// Differential test (see test_diff.h): the header build (IMGUI_TEST_HEADER) and the module build must print the same output.

#include <SDL3/SDL.h>   // SDL API + macros for the application (modules don't export SDL)
#ifdef IMGUI_TEST_HEADER
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_impl_sdlgpu3.h"
#include <stdio.h>
#else
import std.compat;
import imgui;
import imgui.impl.sdl3;
import imgui.impl.sdlrenderer3;
import imgui.impl.sdlgpu3;
#endif
#include "test_diff.h"

static_assert(sizeof(ImGui_ImplSDLGPU3_InitInfo) > 0);

void RenderFrameSDLGPU3(SDL_Window* window, SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer, SDL_GPURenderPass* render_pass)
{
    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info;
    init_info.Device = device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
    ImGui_ImplSDLGPU3_Init(&init_info);
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), command_buffer);
    ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), command_buffer, render_pass);
}

int main(int, char**)
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("SDL_Init() failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("imgui module test", 1280, 720, 0);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, SDL_SOFTWARE_RENDERER) : nullptr;
    if (renderer == nullptr)
    {
        printf("SDL_CreateWindow()/SDL_CreateRenderer() failed: %s\n", SDL_GetError());
        return 1;
    }

    ImGui::CreateContext();
    TestDiff_Init();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode_AutoFirst);

    TestHash hash;
    for (int frame = 0; frame < TestDiff_FrameCount; frame++)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
            ImGui_ImplSDL3_ProcessEvent(&event);
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        TestDiff_NewFrame(frame);
        ImGui::NewFrame();
        TestDiff_ShowWindows();
        ImGui::Render();
        TestDiff_HashDrawData(hash, ImGui::GetDrawData());
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    }

    // Rendered image of the last frame
    if (SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr))
    {
        hash.Add(surface->pixels, (size_t)(surface->pitch * surface->h));
        SDL_DestroySurface(surface);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    hash.Print("imgui.impl.sdl3 + imgui.impl.sdlrenderer3");
    return hash.VtxCount > 0 ? 0 : 1;
}
