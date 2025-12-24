// main.cpp
// Example program using imgui_impl_sdlsurface2 backend

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlsurface2.h"
#include <SDL.h>
#include <stdio.h>

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+Surface Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720, SDL_WINDOW_SHOWN);
    if (!window)
    {
        printf("Error creating window: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Surface* window_surface = SDL_GetWindowSurface(window);
    if (!window_surface)
    {
        printf("Error getting window surface: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Surface* framebuffer = SDL_CreateRGBSurfaceWithFormat(0, 1280, 720, 32, SDL_PIXELFORMAT_RGBA32);
    if (!framebuffer)
    {
        printf("Error creating framebuffer: %s\n", SDL_GetError());
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window, nullptr);
    ImGui_ImplSDL2_SetGamepadMode(ImGui_ImplSDL2_GamepadMode_AutoFirst, nullptr, 0);
    ImGui_ImplSDLSurface2_Init(framebuffer);

    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        ImGui_ImplSDLSurface2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::Render();

        SDL_FillRect(framebuffer, nullptr, SDL_MapRGB(framebuffer->format,
            (Uint8)(clear_color.x * 255),
            (Uint8)(clear_color.y * 255),
            (Uint8)(clear_color.z * 255)));

        ImGui_ImplSDLSurface2_RenderDrawData(ImGui::GetDrawData());

        SDL_BlitSurface(framebuffer, nullptr, window_surface, nullptr);
        SDL_UpdateWindowSurface(window);
    }

    ImGui_ImplSDLSurface2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_FreeSurface(framebuffer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
