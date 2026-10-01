// dear imgui: run test for the C++20 backend modules 'imgui.impl.sdl2' + 'imgui.impl.sdlrenderer2'.
// Runs headless using SDL's "dummy" video driver and the software renderer.
// Differential test (see test_diff.h): the header build (IMGUI_TEST_HEADER) and the module build must print the same output.

#include <SDL.h>        // SDL API + macros for the application (modules don't export SDL)
#ifdef IMGUI_TEST_HEADER
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <stdio.h>
#else
import std.compat;
import imgui;
import imgui.impl.sdl2;
import imgui.impl.sdlrenderer2;
#endif
#include "test_diff.h"

int main(int, char**)
{
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init() failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("imgui module test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, 0);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE) : nullptr;
    if (renderer == nullptr)
    {
        printf("SDL_CreateWindow()/SDL_CreateRenderer() failed: %s\n", SDL_GetError());
        return 1;
    }

    ImGui::CreateContext();
    TestDiff_Init();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    ImGui_ImplSDL2_SetGamepadMode(ImGui_ImplSDL2_GamepadMode_AutoFirst);

    TestHash hash;
    for (int frame = 0; frame < TestDiff_FrameCount; frame++)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
            ImGui_ImplSDL2_ProcessEvent(&event);
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        TestDiff_NewFrame(frame);
        ImGui::NewFrame();
        TestDiff_ShowWindows();
        ImGui::Render();
        TestDiff_HashDrawData(hash, ImGui::GetDrawData());
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    }

    // Rendered image of the last frame
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    ImVector<unsigned char> pixels;
    pixels.resize(w * h * 4);
    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, pixels.Data, w * 4);
    hash.Add(pixels.Data, (size_t)pixels.Size);

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    hash.Print("imgui.impl.sdl2 + imgui.impl.sdlrenderer2");
    return hash.VtxCount > 0 ? 0 : 1;
}
