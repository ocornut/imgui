// main.cpp
// Example program using imgui_impl_sdlsurface2 backend

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlsurface2.h"
#include <SDL.h>
#include <stdio.h>

int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window
    float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+Surface Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        (int)(1280 * main_scale), (int)(720 * main_scale), window_flags);
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

    int win_w = window_surface->w;
    int win_h = window_surface->h;
    SDL_Surface* framebuffer = SDL_CreateRGBSurfaceWithFormat(0, win_w, win_h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!framebuffer)
    {
        printf("Error creating framebuffer: %s\n", SDL_GetError());
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale.
    style.FontScaleDpi = main_scale;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOther(window);
    ImGui_ImplSDL2_SetGamepadMode(ImGui_ImplSDL2_GamepadMode_AutoFirst, nullptr, 0);
    ImGui_ImplSDLSurface2_Init(framebuffer);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;

            if (event.type == SDL_WINDOWEVENT)
            {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                    done = true;

                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    // Recreate framebuffer at new size
                    window_surface = SDL_GetWindowSurface(window);
                    int new_w = window_surface->w;
                    int new_h = window_surface->h;
                    if (new_w != win_w || new_h != win_h)
                    {
                        SDL_FreeSurface(framebuffer);
                        framebuffer = SDL_CreateRGBSurfaceWithFormat(0, new_w, new_h, 32, SDL_PIXELFORMAT_RGBA32);
                        if (!framebuffer) { printf("Error creating framebuffer after resize: %s\n", SDL_GetError()); return -1; }
                        win_w = new_w; win_h = new_h;
                        // Re-init the backend with the new framebuffer
                        ImGui_ImplSDLSurface2_Shutdown();
                        ImGui_ImplSDLSurface2_Init(framebuffer);
                    }
                }
            }
        }

        // Skip rendering when minimized to avoid busy-looping
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLSurface2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. Mirror the SDL_Renderer example.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();

        SDL_FillRect(framebuffer, nullptr, SDL_MapRGBA(framebuffer->format,
            (Uint8)(clear_color.x * 255),
            (Uint8)(clear_color.y * 255),
            (Uint8)(clear_color.z * 255),
            (Uint8)(clear_color.w * 255)));

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
