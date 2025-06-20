// Dear ImGui: standalone example application for using GLFW + WebGPU
// - Emscripten is supported for publishing on web. See https://emscripten.org.
// - Dawn is used as a WebGPU implementation on desktop.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp


#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_wgpu.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
    #if defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
        #include <emscripten/html5_webgpu.h>
    #endif

#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

#include <SDL3/SDL.h>

#include <webgpu/webgpu.h>
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
    #include <webgpu/webgpu_cpp.h>
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#endif

// Global WebGPU required states
WGPUInstance             wgpu_instance = nullptr;
WGPUDevice               wgpu_device   = nullptr;
WGPUSurface              wgpu_surface  = nullptr;
WGPUQueue                wgpu_queue    = nullptr;
WGPUTextureFormat        wgpu_preferred_fmt = WGPUTextureFormat_Undefined;  // acquired from SurfaceCapabilities
WGPUSurfaceConfiguration wgpu_surface_configuration {};
int                      wgpu_surface_width = 1280;
int                      wgpu_surface_height = 720;

// Forward declarations
bool InitWGPU(void* window);

static WGPUTexture check_surface_texture_status(SDL_Window * window)
{
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(wgpu_surface, &surfaceTexture);

    switch ( surfaceTexture.status )
    {
#if defined(__EMSCRIPTEN__) && !defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
        case WGPUSurfaceGetCurrentTextureStatus_Success:
            break;
#else
        case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
            break;
        case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
#endif
        case WGPUSurfaceGetCurrentTextureStatus_Timeout:
        case WGPUSurfaceGetCurrentTextureStatus_Outdated:
        case WGPUSurfaceGetCurrentTextureStatus_Lost:
        // if the status is NOT Optimal let's try to reconfigure the surface
        {
            if (surfaceTexture.texture)
                wgpuTextureRelease(surfaceTexture.texture);
            int width, height;
            SDL_GetWindowSize(window, &width, &height);
            if ( width > 0 && height > 0 )
            {
                wgpu_surface_configuration.width  = width;
                wgpu_surface_configuration.height = height;

                wgpuSurfaceConfigure(wgpu_surface, &wgpu_surface_configuration);
            }
            return nullptr;
        }
        // Unrecoverable errors
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
        case WGPUSurfaceGetCurrentTextureStatus_Error:
#else   // IMGUI_IMPL_WEBGPU_BACKEND_WGPU
        case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
        case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
#endif
        case WGPUSurfaceGetCurrentTextureStatus_Force32:
            // Fatal error
            printf("Unrecoverable Error check_surface_texture status=%#.8x\n", surfaceTexture.status);
            abort();

        default:            // should never be reached
            printf("Unexpected Error check_surface_texture status=%#.8x\n", surfaceTexture.status);
            abort();
    }
    return surfaceTexture.texture;
}

void ResizeSurface(int width, int height)
{
    wgpu_surface_configuration.width  = wgpu_surface_width  = width;
    wgpu_surface_configuration.height = wgpu_surface_height = height;

    wgpuSurfaceConfigure( wgpu_surface, (WGPUSurfaceConfiguration *) &wgpu_surface_configuration );
}

// Main code
int main(int, char**)
{
#if defined(__linux__)
    // SDL3 default is "x11" (it works also in "wayland"), uncomment the line below to use "wayland"
    // SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");
#endif

    // Init SDL
    if (!SDL_Init(SDL_INIT_VIDEO ))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+WebGPU example", wgpu_surface_width, wgpu_surface_height, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Initialize WGPU
    InitWGPU(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOther(window);

    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = wgpu_device;
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = wgpu_preferred_fmt;
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&init_info);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Emscripten allows preloading a file or folder to be accessible at runtime. See Makefile for details.
    //io.Fonts->AddFontDefault();
#ifndef IMGUI_DISABLE_FILE_FUNCTIONS
    //io.Fonts->AddFontFromFileTTF("fonts/segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("fonts/ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);
#endif

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    SDL_Event event;
    bool canCloseWindow = false;
    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!canCloseWindow) 
#endif
    {
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT ||
               (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)))
                    canCloseWindow = true;
        }
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.

        // React to changes in screen size
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        if (width != wgpu_surface_width || height != wgpu_surface_height)
        {
            ImGui_ImplWGPU_InvalidateDeviceObjects();
            ResizeSurface(width, height);
            ImGui_ImplWGPU_CreateDeviceObjects();
            //continue;
        }

        // Check surface texture status
        WGPUTexture texture = check_surface_texture_status(window);
        if(!texture) continue;

        // Start the Dear ImGui frame
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                                // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");                     // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);            // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);                  // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color);       // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                                  // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);         // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();

        WGPUTextureViewDescriptor viewDescriptor {};
        viewDescriptor.format          = wgpu_preferred_fmt;
        viewDescriptor.dimension       = WGPUTextureViewDimension_2D;
        viewDescriptor.mipLevelCount   = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
        viewDescriptor.arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
        viewDescriptor.aspect          = WGPUTextureAspect_All;

        WGPUTextureView textureView    = wgpuTextureCreateView(texture, &viewDescriptor);

        WGPURenderPassColorAttachment color_attachments {};
        color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        color_attachments.loadOp     = WGPULoadOp_Clear;
        color_attachments.storeOp    = WGPUStoreOp_Store;
        color_attachments.clearValue = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        color_attachments.view       = textureView;

        WGPURenderPassDescriptor render_pass_desc {};
        render_pass_desc.colorAttachmentCount   = 1;
        render_pass_desc.colorAttachments       = &color_attachments;
        render_pass_desc.depthStencilAttachment = nullptr;

        WGPUCommandEncoderDescriptor enc_desc {};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wgpu_device, &enc_desc);

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
        wgpuRenderPassEncoderEnd(pass);

        WGPUCommandBufferDescriptor cmd_buffer_desc {};
        WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        wgpuQueueSubmit(wgpu_queue, 1, &cmd_buffer);

#ifndef __EMSCRIPTEN__
        wgpuSurfacePresent(wgpu_surface);
        // Tick needs to be called in Dawn to display validation errors
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
        wgpuDeviceTick(wgpu_device);
#endif
#endif
        wgpuTextureViewRelease(textureView);
        wgpuRenderPassEncoderRelease(pass);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(cmd_buffer);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    wgpuSurfaceUnconfigure(wgpu_surface);
    wgpuSurfaceRelease(wgpu_surface);
    wgpuQueueRelease(wgpu_queue);
    wgpuDeviceRelease(wgpu_device);
    wgpuInstanceRelease(wgpu_instance);

    // Terminate SDL
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}

