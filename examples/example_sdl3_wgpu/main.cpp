// Dear ImGui: standalone example application for using SDL3 + WebGPU
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
#include <SDL3/SDL.h>

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
#include <emscripten/html5_webgpu.h>
#endif
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
#include <webgpu/webgpu_cpp.h>
#endif

// Data
static WGPUInstance             wgpu_instance = nullptr;
static WGPUDevice               wgpu_device = nullptr;
static WGPUSurface              wgpu_surface = nullptr;
static WGPUQueue                wgpu_queue = nullptr;
static WGPUSurfaceConfiguration wgpu_surface_configuration = {};
static int                      wgpu_surface_width = 1280;
static int                      wgpu_surface_height = 800;

// Forward declarations
static bool         InitWGPU(SDL_Window* window);
static WGPUSurface  CreateWGPUSurface(const WGPUInstance& instance, SDL_Window* window);

static void ResizeSurface(int width, int height)
{
    wgpu_surface_configuration.width  = wgpu_surface_width  = width;
    wgpu_surface_configuration.height = wgpu_surface_height = height;
    wgpuSurfaceConfigure( wgpu_surface, (WGPUSurfaceConfiguration*)&wgpu_surface_configuration );
}

// Main code
int main(int, char**)
{
    // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // Create SDL window graphics context
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+WebGPU example", wgpu_surface_width, wgpu_surface_height, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
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

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOther(window);

    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = wgpu_device;
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = wgpu_surface_configuration.format;
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&init_info);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
#ifndef IMGUI_DISABLE_FILE_FUNCTIONS
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);
#endif

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        // [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
        // React to changes in screen size
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        if (width != wgpu_surface_width || height != wgpu_surface_height)
            ResizeSurface(width, height);

        // Check surface status for error. If texture is not optimal, try to reconfigure the surface.
        WGPUSurfaceTexture surface_texture;
        wgpuSurfaceGetCurrentTexture(wgpu_surface, &surface_texture);
        if (ImGui_ImplWGPU_IsSurfaceStatusError(surface_texture.status))
        {
            fprintf(stderr, "Unrecoverable Surface Texture status=%#.8x\n", surface_texture.status);
            abort();
        }
        if (ImGui_ImplWGPU_IsSurfaceStatusSubOptimal(surface_texture.status))
        {
            if (surface_texture.texture)
                wgpuTextureRelease(surface_texture.texture);
            if (width > 0 && height > 0)
                ResizeSurface(width, height);
            continue;
        }

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

        WGPUTextureViewDescriptor view_desc = {};
        view_desc.format = wgpu_surface_configuration.format;
        view_desc.dimension = WGPUTextureViewDimension_2D;
        view_desc.mipLevelCount = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
        view_desc.arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
        view_desc.aspect = WGPUTextureAspect_All;

        WGPUTextureView texture_view = wgpuTextureCreateView(surface_texture.texture, &view_desc);

        WGPURenderPassColorAttachment color_attachments = {};
        color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        color_attachments.loadOp = WGPULoadOp_Clear;
        color_attachments.storeOp = WGPUStoreOp_Store;
        color_attachments.clearValue = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        color_attachments.view = texture_view;

        WGPURenderPassDescriptor render_pass_desc = {};
        render_pass_desc.colorAttachmentCount = 1;
        render_pass_desc.colorAttachments = &color_attachments;
        render_pass_desc.depthStencilAttachment = nullptr;

        WGPUCommandEncoderDescriptor enc_desc = {};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wgpu_device, &enc_desc);

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
        wgpuRenderPassEncoderEnd(pass);

        WGPUCommandBufferDescriptor cmd_buffer_desc = {};
        WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        wgpuQueueSubmit(wgpu_queue, 1, &cmd_buffer);

#ifndef __EMSCRIPTEN__
        wgpuSurfacePresent(wgpu_surface);
        // Tick needs to be called in Dawn to display validation errors
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
        wgpuDeviceTick(wgpu_device);
#endif
#endif
        wgpuTextureViewRelease(texture_view);
        wgpuRenderPassEncoderRelease(pass);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(cmd_buffer);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    wgpuSurfaceUnconfigure(wgpu_surface);
    wgpuSurfaceRelease(wgpu_surface);
    wgpuQueueRelease(wgpu_queue);
    wgpuDeviceRelease(wgpu_device);
    wgpuInstanceRelease(wgpu_instance);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
static WGPUAdapter RequestAdapter(wgpu::Instance& instance)
{
    wgpu::Adapter acquired_adapter;
    wgpu::RequestAdapterOptions adapter_options;
    auto onRequestAdapter = [&](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message)
    {
        if (status != wgpu::RequestAdapterStatus::Success)
        {
            printf("Failed to get an adapter: %s\n", message.data);
            return;
        }
        acquired_adapter = std::move(adapter);
    };

    // Synchronously (wait until) acquire Adapter
    wgpu::Future waitAdapterFunc { instance.RequestAdapter(&adapter_options, wgpu::CallbackMode::WaitAnyOnly, onRequestAdapter) };
    wgpu::WaitStatus waitStatusAdapter = instance.WaitAny(waitAdapterFunc, UINT64_MAX);
    IM_ASSERT(acquired_adapter != nullptr && waitStatusAdapter == wgpu::WaitStatus::Success && "Error on Adapter request");
    return acquired_adapter.MoveToCHandle();
}

static WGPUDevice RequestDevice(wgpu::Instance& instance, wgpu::Adapter& adapter)
{
    // Set device callback functions
    wgpu::DeviceDescriptor device_desc;
    device_desc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous,
        [](const wgpu::Device&, wgpu::DeviceLostReason type, wgpu::StringView msg) { fprintf(stderr, "%s error: %s\n", ImGui_ImplWGPU_GetDeviceLostReasonName((WGPUDeviceLostReason)type), msg.data); }
    );
    device_desc.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView msg) { fprintf(stderr, "%s error: %s\n", ImGui_ImplWGPU_GetErrorTypeName((WGPUErrorType)type), msg.data); }
    );

    wgpu::Device acquired_device;
    auto onRequestDevice = [&](wgpu::RequestDeviceStatus status, wgpu::Device local_device, wgpu::StringView message)
    {
        if (status != wgpu::RequestDeviceStatus::Success)
        {
            printf("Failed to get an device: %s\n", message.data);
            return;
        }
        acquired_device = std::move(local_device);
    };

    // Synchronously (wait until) get Device
    wgpu::Future waitDeviceFunc { adapter.RequestDevice(&device_desc, wgpu::CallbackMode::WaitAnyOnly, onRequestDevice) };
    wgpu::WaitStatus waitStatusDevice = instance.WaitAny(waitDeviceFunc, UINT64_MAX);
    IM_ASSERT(acquired_device != nullptr && waitStatusDevice == wgpu::WaitStatus::Success && "Error on Device request");
    return acquired_device.MoveToCHandle();
}
#elif defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
#ifdef __EMSCRIPTEN__
// Adapter and device initialization via JS
EM_ASYNC_JS( void, getAdapterAndDeviceViaJS, (),
{
    if (!navigator.gpu)
        throw Error("WebGPU not supported.");
    const adapter = await navigator.gpu.requestAdapter();
    const device = await adapter.requestDevice();
    Module.preinitializedWebGPUDevice = device;
} );
#else // __EMSCRIPTEN__
static void handle_request_adapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2)
{
    if (status == WGPURequestAdapterStatus_Success)
    {
        WGPUAdapter* extAdapter = (WGPUAdapter*)userdata1;
        *extAdapter = adapter;
    }
    else
    {
        printf("Request_adapter status=%#.8x message=%.*s\n", status, (int)message.length, message.data);
    }
}

static void handle_request_device(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* userdata2)
{
    if (status == WGPURequestDeviceStatus_Success)
    {
        WGPUDevice* extDevice = (WGPUDevice*)userdata1;
        *extDevice = device;
    }
    else
    {
        printf("Request_device status=%#.8x message=%.*s\n", status, (int)message.length, message.data);
    }
}

static WGPUAdapter RequestAdapter(WGPUInstance& instance)
{
    WGPURequestAdapterOptions adapter_options = {};

    WGPUAdapter local_adapter;
    WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
    adapterCallbackInfo.callback = handle_request_adapter;
    adapterCallbackInfo.userdata1 = &local_adapter;

    wgpuInstanceRequestAdapter(instance, &adapter_options, adapterCallbackInfo);
    IM_ASSERT(local_adapter && "Error on Adapter request");
    return local_adapter;
}

static WGPUDevice RequestDevice(WGPUAdapter& adapter)
{
    WGPUDevice local_device;
    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.callback = handle_request_device;
    deviceCallbackInfo.userdata1 = &local_device;
    wgpuAdapterRequestDevice(adapter, nullptr, deviceCallbackInfo);
    IM_ASSERT(local_device && "Error on Device request");
    return local_device;
}
#endif // __EMSCRIPTEN__
#endif // IMGUI_IMPL_WEBGPU_BACKEND_WGPU

static bool InitWGPU(SDL_Window* window)
{
    WGPUTextureFormat preferred_fmt = WGPUTextureFormat_Undefined;  // acquired from SurfaceCapabilities

    // Google DAWN backend: Adapter and Device acquisition, Surface creation
#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
    wgpu::InstanceDescriptor instance_desc = {};
    static constexpr wgpu::InstanceFeatureName timedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    instance_desc.requiredFeatureCount = 1;
    instance_desc.requiredFeatures = &timedWaitAny;
    wgpu::Instance instance = wgpu::CreateInstance(&instance_desc);

    wgpu::Adapter adapter = RequestAdapter(instance);
    ImGui_ImplWGPU_DebugPrintAdapterInfo(adapter.Get());

    wgpu_device = RequestDevice(instance, adapter);

    // Create the surface.
#ifdef __EMSCRIPTEN__
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvas_desc = {};
    canvas_desc.selector = "#canvas";

    wgpu::SurfaceDescriptor surface_desc = {};
    surface_desc.nextInChain = &canvas_desc;
    wgpu::Surface surface = instance.CreateSurface(&surface_desc);
#else
    wgpu::Surface surface = CreateWGPUSurface(instance.Get(), window);
#endif
    if (!surface)
        return false;

    // Moving Dawn objects into WGPU handles
    wgpu_instance = instance.MoveToCHandle();
    wgpu_surface = surface.MoveToCHandle();

    WGPUSurfaceCapabilities surface_capabilities = {};
    wgpuSurfaceGetCapabilities(wgpu_surface, adapter.Get(), &surface_capabilities);

    preferred_fmt = surface_capabilities.formats[0];

    // WGPU backend: Adapter and Device acquisition, Surface creation
#elif defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
    wgpu_instance = wgpuCreateInstance(nullptr);

#ifdef __EMSCRIPTEN__
    getAdapterAndDeviceViaJS();

    wgpu_device = emscripten_webgpu_get_device();
    IM_ASSERT(wgpu_device != nullptr && "Error creating the Device");

    WGPUSurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
    html_surface_desc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
    html_surface_desc.selector = "#canvas";

    WGPUSurfaceDescriptor surface_desc = {};
    surface_desc.nextInChain = &html_surface_desc.chain;

    // Create the surface.
    wgpu_surface = wgpuInstanceCreateSurface(wgpu_instance, &surface_desc);
    preferred_fmt = wgpuSurfaceGetPreferredFormat(wgpu_surface, {} /* adapter */);
#else // __EMSCRIPTEN__
    wgpuSetLogCallback(
        [](WGPULogLevel level, WGPUStringView msg, void* userdata) { fprintf(stderr, "%s: %.*s\n", ImGui_ImplWGPU_GetLogLevelName(level), (int)msg.length, msg.data); }, nullptr
    );
    wgpuSetLogLevel(WGPULogLevel_Warn);

    WGPUAdapter adapter = RequestAdapter(wgpu_instance);
    ImGui_ImplWGPU_DebugPrintAdapterInfo(adapter);

    wgpu_device = RequestDevice(adapter);

    // Create the surface.
    wgpu_surface = CreateWGPUSurface(wgpu_instance, window);
    if (!wgpu_surface)
        return false;

    WGPUSurfaceCapabilities surface_capabilities = {};
    wgpuSurfaceGetCapabilities(wgpu_surface, adapter, &surface_capabilities);

    preferred_fmt = surface_capabilities.formats[0];
#endif // __EMSCRIPTEN__
#endif // IMGUI_IMPL_WEBGPU_BACKEND_WGPU

    wgpu_surface_configuration.presentMode = WGPUPresentMode_Fifo;
    wgpu_surface_configuration.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpu_surface_configuration.usage = WGPUTextureUsage_RenderAttachment;
    wgpu_surface_configuration.width = wgpu_surface_width;
    wgpu_surface_configuration.height = wgpu_surface_height;
    wgpu_surface_configuration.device = wgpu_device;
    wgpu_surface_configuration.format = preferred_fmt;

    wgpuSurfaceConfigure(wgpu_surface, &wgpu_surface_configuration);
    wgpu_queue = wgpuDeviceGetQueue(wgpu_device);

    return true;
}

// SDL3 helper to create a WebGPU surface (exclusively!) for Native/Desktop applications: available only together with WebGPU/WGPU backend
// As of today (2025/10) there is no "official" support in SDL3 to create a surface for WebGPU backend
// This stub uses "low level" SDL3 calls to acquire information from a specific Window Manager.
// Currently supported platforms: Windows / Linux (X11 and Wayland) / MacOS. Not necessary nor available with EMSCRIPTEN.
#ifndef __EMSCRIPTEN__

#if defined(SDL_PLATFORM_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#endif

static WGPUSurface CreateWGPUSurface(const WGPUInstance& instance, SDL_Window* window)
{
    SDL_PropertiesID propertiesID = SDL_GetWindowProperties(window);

    ImGui_ImplWGPU_CreateSurfaceInfo create_info = {};
    create_info.Instance = instance;
#if defined(SDL_PLATFORM_MACOS)
    {
        create_info.System = "cocoa";
        create_info.RawWindow = (void*)SDL_GetPointerProperty(propertiesID, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
        return ImGui_ImplWGPU_CreateWGPUSurfaceHelper(&create_info);
    }
#elif defined(SDL_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0)
    {
        create_info.System = "wayland";
        create_info.RawDisplay = (void*)SDL_GetPointerProperty(propertiesID, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        create_info.RawSurface = (void*)SDL_GetPointerProperty(propertiesID, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
        return ImGui_ImplWGPU_CreateWGPUSurfaceHelper(&create_info);
    }
    else if (!SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11"))
    {
        create_info.System = "x11";
        create_info.RawWindow = (void*)SDL_GetNumberProperty(propertiesID, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        create_info.RawDisplay = (void*)SDL_GetPointerProperty(propertiesID, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        return ImGui_ImplWGPU_CreateWGPUSurfaceHelper(&create_info);
    }
#elif defined(SDL_PLATFORM_WIN32)
    {
        create_info.System = "win32";
        create_info.RawWindow = (void*)SDL_GetPointerProperty(propertiesID, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        create_info.RawInstance = (void*)::GetModuleHandle(NULL);
        return ImGui_ImplWGPU_CreateWGPUSurfaceHelper(&create_info);
    }
#else
#error "Unsupported WebGPU native platform!"
#endif
    return nullptr;
}
#endif // #ifndef __EMSCRIPTEN__
