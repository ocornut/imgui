// Dear ImGui: standalone example application for using GLFW + WebGPU
// - Emscripten is supported for publishing on web. See https://emscripten.org.
// - Dawn is used as a WebGPU implementation on desktop.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp


#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_wgpu.h"
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#else
#endif
#define SDL_MAIN_HANDLED
#include "sdl2wgpu.h"

#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

// Global WebGPU required states
static WGPUInstance             wgpu_instance = nullptr;
static WGPUDevice               wgpu_device = nullptr;
static WGPUSurface              wgpu_surface = nullptr;
static WGPUQueue                wgpu_queue = nullptr;
static WGPUTextureFormat        wgpu_preferred_fmt = WGPUTextureFormat_Undefined;  // acquired from SurfaceCapabilities
static WGPUSurfaceConfiguration wgpu_surface_configuration {};
static int                      wgpu_surface_width = 1280;
static int                      wgpu_surface_height = 720;

// Forward declarations
static bool InitWGPU(SDL_Window* window);
static void ResizeSurface(int width, int height);

#ifndef __EMSCRIPTEN__
static void wgpu_device_lost_callback(const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message)
{
    const char* reasonName = "";
    switch (reason)
    {
        case wgpu::DeviceLostReason::Unknown:           reasonName = "Unknown";         break;
        case wgpu::DeviceLostReason::Destroyed:         reasonName = "Destroyed";       break;
        case wgpu::DeviceLostReason::CallbackCancelled: reasonName = "InstanceDropped"; break;
        case wgpu::DeviceLostReason::FailedCreation:    reasonName = "FailedCreation";  break;
        default:                                        reasonName = "UNREACHABLE";     break;
    }
    printf("%s device message: %s\n", reasonName, message.data);
}

static void wgpu_error_callback(const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message)
{
    const char* errorTypeName = "";
    switch (type)
    {
        case wgpu::ErrorType::Validation:  errorTypeName = "Validation";      break;
        case wgpu::ErrorType::OutOfMemory: errorTypeName = "Out of memory";   break;
        case wgpu::ErrorType::Unknown:     errorTypeName = "Unknown";         break;
        case wgpu::ErrorType::Internal:    errorTypeName = "Internal";        break;
        default:                           errorTypeName = "UNREACHABLE";     break;
    }
    printf("%s error: %s\n", errorTypeName, message.data);
}
#endif

static WGPUTexture check_surface_texture_status(SDL_Window* window)
{
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(wgpu_surface, &surfaceTexture);

    switch ( surfaceTexture.status )
    {
#if !defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
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
        // if the status is NOT optimal let's try to reconfigure the surface
        {
#ifndef NDEBUG
            printf("Bad surface texture status: %d\n", surfaceTexture.status);
#endif
/*            if (surfaceTexture.texture)
                wgpuTextureRelease(surfaceTexture.texture);*/
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
        default:            // should never be reached
            assert(!"Unexpected Surface Texture status error\n");
            return nullptr;
    }
    return surfaceTexture.texture;
}

// Main code
int main(int, char**)
{

#if defined(__linux__)
    // it's necessary to specify "x11" or "wayland": default is "x11" it works also in wayland
    // or comment the follow line and export SDL_VIDEODRIVER environment variable:
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11"); // export SDL_VIDEODRIVER=wayland             (to set wayland session type)
                                              // export SDL_VIDEODRIVER=$XDG_SESSION_TYPE   (to get current session type: x11 | wayland)
#endif

    // Init SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Dear ImGui SDL+WebGPU example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, wgpu_surface_width, wgpu_surface_height, SDL_WINDOW_RESIZABLE);

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
    ImGui_ImplSDL2_InitForOther(window);

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
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT ||
               (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window)))
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
        ImGui_ImplSDL2_NewFrame();
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
        wgpuDeviceTick(wgpu_device);
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
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    wgpuSurfaceUnconfigure(wgpu_surface);
    wgpuSurfaceRelease(wgpu_surface);
    wgpuQueueRelease(wgpu_queue);
    wgpuDeviceRelease(wgpu_device);
    wgpuInstanceRelease(wgpu_instance);
#ifndef __EMSCRIPTEN__
    // wait to flush all WGPU operations before to continue
    wgpuDeviceTick(wgpu_device);
#endif

    // Terminate SDL
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

static bool InitWGPU(SDL_Window* window)
{
    static wgpu::Adapter localAdapter;

    wgpu_surface_configuration.presentMode = WGPUPresentMode_Fifo;
    wgpu_surface_configuration.alphaMode   = WGPUCompositeAlphaMode_Auto;
    wgpu_surface_configuration.usage       = WGPUTextureUsage_RenderAttachment;
    wgpu_surface_configuration.width       = wgpu_surface_width;
    wgpu_surface_configuration.height      = wgpu_surface_height;

#ifdef __EMSCRIPTEN__
    wgpu::Instance instance = wgpuCreateInstance(nullptr);
    wgpu_device = emscripten_webgpu_get_device();
    if (!wgpu_device)
        return false;
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
    html_surface_desc.selector = "#canvas";
    wgpu::SurfaceDescriptor surface_desc = {};
    surface_desc.nextInChain = &html_surface_desc;
    wgpu::Surface surface = instance.CreateSurface(&surface_desc);

    wgpu::Adapter adapter = {};
    wgpu_preferred_fmt = (WGPUTextureFormat)surface.GetPreferredFormat(adapter);

    wgpu_surface_configuration.device      = wgpu_device;
    wgpu_surface_configuration.format      = wgpu_preferred_fmt;
#else
    wgpu::InstanceDescriptor instanceDescriptor {};
    instanceDescriptor.capabilities.timedWaitAnyEnable = true;
    wgpu::Instance instance = wgpu::CreateInstance(&instanceDescriptor);

    wgpu::RequestAdapterOptions adapterOptions {};

    auto onRequestAdapter = [](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message)
    {
        if (status != wgpu::RequestAdapterStatus::Success)
        {
            printf("Failed to get an adapter: %s\n", message.data);
            return;
        }
        localAdapter = std::move(adapter);
    };

    // Synchronously (wait until) acquire Adapter
    auto waitedAdapterFunc { instance.RequestAdapter(&adapterOptions, wgpu::CallbackMode::WaitAnyOnly, onRequestAdapter) };
    instance.WaitAny(waitedAdapterFunc, UINT64_MAX);
    assert(localAdapter != nullptr);

#ifndef NDEBUG
    wgpu::AdapterInfo info;
    localAdapter.GetInfo(&info);
    printf("Using adapter: \" %s \"\n", info.device.data);
#endif

    // Set device callback functions
    wgpu::DeviceDescriptor deviceDesc {};
    deviceDesc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous, wgpu_device_lost_callback);
    deviceDesc.SetUncapturedErrorCallback(wgpu_error_callback);

    // get device Synchronously
    wgpu_device = localAdapter.CreateDevice(&deviceDesc).MoveToCHandle();
    assert(wgpu_device != nullptr);

    wgpu::Surface surface = SDL_getWGPUSurface(instance.Get(), window);
    if (!surface)
        return false;

    // Configure the surface.
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(localAdapter, &capabilities);
    wgpu_preferred_fmt = (WGPUTextureFormat) capabilities.formats[0];

    wgpu_surface_configuration.device      = wgpu_device;
    wgpu_surface_configuration.format      = wgpu_preferred_fmt;
    surface.Configure((const wgpu::SurfaceConfiguration *) &wgpu_surface_configuration);
#endif

    wgpu_instance = instance.MoveToCHandle();
    wgpu_surface  = surface.MoveToCHandle();
    wgpu_queue    = wgpuDeviceGetQueue(wgpu_device);

    return true;
}

void ResizeSurface(int width, int height)
{
    wgpu_surface_configuration.width  = wgpu_surface_width  = width;
    wgpu_surface_configuration.height = wgpu_surface_height = height;

    wgpuSurfaceConfigure( wgpu_surface, (WGPUSurfaceConfiguration *) &wgpu_surface_configuration );
}