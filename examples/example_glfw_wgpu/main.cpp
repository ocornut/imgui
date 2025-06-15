// Dear ImGui: standalone example application for using GLFW + WebGPU
// - Emscripten is supported for publishing on web. See https://emscripten.org.
// - Dawn is used as a WebGPU implementation on desktop.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"
#include <stdio.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5.h>

    #if defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
        #include <emscripten/html5_webgpu.h>
    #endif
#else
    #if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
        #include <webgpu/webgpu_glfw.h>
    #else
        #include <webgpu/wgpu.h>
        #ifdef __cplusplus
            extern "C" {
        #endif
            WGPUSurface GLFW_getWGPUSurface(WGPUInstance instance, GLFWwindow* window);
        #ifdef __cplusplus
            }
        #endif
    #endif
#endif


// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

// Global WebGPU required states
static WGPUInstance             wgpu_instance = nullptr;
static WGPUDevice               wgpu_device   = nullptr;
static WGPUSurface              wgpu_surface  = nullptr;
static WGPUQueue                wgpu_queue    = nullptr;
static WGPUTextureFormat        wgpu_preferred_fmt = WGPUTextureFormat_Undefined;  // acquired from SurfaceCapabilities
static WGPUSurfaceConfiguration wgpu_surface_configuration {};
static int                      wgpu_surface_width = 1280;
static int                      wgpu_surface_height = 720;

// Forward declarations
static bool InitWGPU(GLFWwindow* window);
static void ResizeSurface(int width, int height);

static void glfw_error_callback(int error, const char* description)
{
    printf("GLFW Error %d: %s\n", error, description);
}

static WGPUTexture check_surface_texture_status(GLFWwindow * window)
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
            glfwGetFramebufferSize(window, &width, &height);
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

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Make sure GLFW does not initialize any graphics context.
    // This needs to be done explicitly later.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(wgpu_surface_width, wgpu_surface_height, "Dear ImGui GLFW+WebGPU example", nullptr, nullptr);
    if (window == nullptr)
        return 1;

    // Initialize the WebGPU environment
    if (!InitWGPU(window))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwShowWindow(window);

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
    ImGui_ImplGlfw_InitForOther(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
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

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // React to changes in screen size
        int width, height;
        glfwGetFramebufferSize((GLFWwindow*)window, &width, &height);
        if (width != wgpu_surface_width || height != wgpu_surface_height)
        {
            ImGui_ImplWGPU_InvalidateDeviceObjects();
            ResizeSurface(width, height);
            ImGui_ImplWGPU_CreateDeviceObjects();
        }

        // Check surface texture status
        WGPUTexture texture = check_surface_texture_status(window);
        if (!texture) continue;

        // Start the Dear ImGui frame
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
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
        viewDescriptor.dimension       = WGPUTextureViewDimension_2D ;
        viewDescriptor.mipLevelCount   = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
        viewDescriptor.arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
        viewDescriptor.aspect          = WGPUTextureAspect_All;

        WGPUTextureView textureView = wgpuTextureCreateView(texture, &viewDescriptor);


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

        WGPUCommandEncoderDescriptor enc_desc = {};
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
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    wgpuSurfaceUnconfigure(wgpu_surface);
    wgpuSurfaceRelease(wgpu_surface);
    wgpuQueueRelease(wgpu_queue);
    wgpuDeviceRelease(wgpu_device);
    wgpuInstanceRelease(wgpu_instance);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void ResizeSurface(int width, int height)
{
    wgpu_surface_configuration.width  = wgpu_surface_width  = width;
    wgpu_surface_configuration.height = wgpu_surface_height = height;

    wgpuSurfaceConfigure(wgpu_surface, (WGPUSurfaceConfiguration *) &wgpu_surface_configuration);
}

// WebGPU initialization using Google DAWN (Native and WEB via EMSCRIPTEN)

#if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
static void wgpu_device_lost_callback(const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message)
{
    const char* reasonName = "";
    switch (reason) {
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
    switch (type) {
        case wgpu::ErrorType::Validation:  errorTypeName = "Validation";      break;
        case wgpu::ErrorType::OutOfMemory: errorTypeName = "Out of memory";   break;
        case wgpu::ErrorType::Unknown:     errorTypeName = "Unknown";         break;
        case wgpu::ErrorType::Internal:    errorTypeName = "Internal";        break;
        default:                           errorTypeName = "UNREACHABLE";     break;
    }
    printf("%s error: %s\n", errorTypeName, message.data);
}

static bool InitWGPU(GLFWwindow* window)
{
    wgpu::InstanceDescriptor instanceDescriptor  = {};
    instanceDescriptor.capabilities.timedWaitAnyEnable = true;
    wgpu::Instance instance = wgpu::CreateInstance(&instanceDescriptor);

    static wgpu::Adapter localAdapter;
    wgpu::RequestAdapterOptions adapterOptions;


    auto onRequestAdapter = [](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
        if (status != wgpu::RequestAdapterStatus::Success) {
            printf("Failed to get an adapter: %s\n", message.data);
            return;
        }
        localAdapter = std::move(adapter);
    };

    // Synchronously (wait until) acquire Adapter
    wgpu::Future  waitAdapterFunc { instance.RequestAdapter(&adapterOptions, wgpu::CallbackMode::WaitAnyOnly, onRequestAdapter) };
    wgpu::WaitStatus waitStatusAdapter = instance.WaitAny(waitAdapterFunc, UINT64_MAX);
    assert(localAdapter != nullptr && waitStatusAdapter == wgpu::WaitStatus::Success && "Error on Adapter request");

#ifndef NDEBUG
    wgpu::AdapterInfo info;
    localAdapter.GetInfo(&info);
    printf("Using adapter: \" %s \"\n", info.device.data);
#endif

    // Set device callback functions
    wgpu::DeviceDescriptor deviceDesc;
    deviceDesc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous, wgpu_device_lost_callback);
    deviceDesc.SetUncapturedErrorCallback(wgpu_error_callback);

    static wgpu::Device acquireddevice;
// get device Synchronously
    auto onRequestDevice = [](wgpu::RequestDeviceStatus status, wgpu::Device localDevice, wgpu::StringView message) {
        if (status != wgpu::RequestDeviceStatus::Success) {
            printf("Failed to get an device: %s\n", message.data);
            return;
        }
        acquireddevice = std::move(localDevice);
    };

    wgpu::Future waitDeviceFunc { localAdapter.RequestDevice(&deviceDesc, wgpu::CallbackMode::WaitAnyOnly, onRequestDevice) };
    wgpu::WaitStatus waitStatusDevice = instance.WaitAny(waitDeviceFunc, UINT64_MAX);
    assert(acquireddevice != nullptr && waitStatusDevice == wgpu::WaitStatus::Success && "Error on Device request");

    wgpu_device = acquireddevice.MoveToCHandle();

    // now DAWN has also a member class func to create device: adapter.CreateDevice (only native)
    //device = localAdapter.CreateDevice(&deviceDesc);
    //assert(device != nullptr && "Error creating the Device");
#ifdef __EMSCRIPTEN__
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
    canvasDesc.selector = "#canvas";

    wgpu::SurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = &canvasDesc;
    wgpu::Surface surface = instance.CreateSurface(&surfaceDesc);
#else
    // Create the surface.
    wgpu::Surface surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
#endif
    if (!surface)
        return false;

    // Configure the surface.
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(localAdapter, &capabilities);
    wgpu_preferred_fmt = (WGPUTextureFormat) capabilities.formats[0];

    wgpu_surface_configuration.presentMode = WGPUPresentMode_Fifo;
    wgpu_surface_configuration.alphaMode   = WGPUCompositeAlphaMode_Auto;
    wgpu_surface_configuration.usage       = WGPUTextureUsage_RenderAttachment;
    wgpu_surface_configuration.width       = wgpu_surface_width;
    wgpu_surface_configuration.height      = wgpu_surface_height;
    wgpu_surface_configuration.device      = wgpu_device;
    wgpu_surface_configuration.format      = wgpu_preferred_fmt;

    surface.Configure(reinterpret_cast<const wgpu::SurfaceConfiguration *>(&wgpu_surface_configuration));

    wgpu_instance = instance.MoveToCHandle();
    wgpu_surface  = surface.MoveToCHandle();
    wgpu_queue    = wgpuDeviceGetQueue(wgpu_device);

    return true;
}
#else

// WebGPU initialization using WGPU (Native and WEB via EMSCRIPTEN)

#ifdef __EMSCRIPTEN__
// Adapter and device initialization via JS
EM_ASYNC_JS( void, getAdapterAndDeviceViaJS, (),
{
    if (!navigator.gpu) throw Error("WebGPU not supported.");

    const adapter = await navigator.gpu.requestAdapter();
    const device = await adapter.requestDevice();
    Module.preinitializedWebGPUDevice = device;
} );
#else
static void handle_request_adapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *userdata1, void *userdata2)
{
    if (status == WGPURequestAdapterStatus_Success)
    {
        WGPUAdapter *extAdapter = (WGPUAdapter *) userdata1;
        *extAdapter = adapter;
    }
    else
        printf("Request_adapter status=%#.8x message=%.*s\n", status, (int) message.length, message.data);
}

static void handle_request_device(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata1, void *userdata2)
{
    if (status == WGPURequestDeviceStatus_Success)
    {
        WGPUDevice *extDevice = (WGPUDevice *) userdata1;
        *extDevice = device;
    }
    else
        printf("Request_device status=%#.8x message=%.*s\n", status, (int) message.length, message.data);
}

static void log_callback(WGPULogLevel level, WGPUStringView message, void *userdata)
{
    const char *level_str = "";
    switch (level) {
        case WGPULogLevel_Error: level_str = "error"; break;
        case WGPULogLevel_Warn:  level_str = "warn";  break;
        case WGPULogLevel_Info:  level_str = "info";  break;
        case WGPULogLevel_Debug: level_str = "debug"; break;
        case WGPULogLevel_Trace: level_str = "trace"; break;
        default:                 level_str = "unknown_level";
    }
    fprintf(stderr, "[wgpu] [%s] %.*s\n", level_str, (int) message.length, message.data);
}
#endif

static bool InitWGPU(GLFWwindow* window)
{
    wgpu_instance = wgpuCreateInstance(nullptr);

#ifdef __EMSCRIPTEN__    
    getAdapterAndDeviceViaJS();

    wgpu_device   = emscripten_webgpu_get_device();
    assert(wgpu_device != nullptr && "Error creating the Device");
    WGPUSurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
    html_surface_desc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
    html_surface_desc.selector    = "#canvas";
    WGPUSurfaceDescriptor surface_desc = {};
    surface_desc.nextInChain   = &html_surface_desc.chain;

    wgpu_surface       = wgpuInstanceCreateSurface(wgpu_instance, &surface_desc);
    wgpu_preferred_fmt = wgpuSurfaceGetPreferredFormat(wgpu_surface, {} /* adapter */);
#else
    wgpuSetLogCallback(log_callback, NULL);
    wgpuSetLogLevel(WGPULogLevel_Warn);


    WGPURequestAdapterOptions adapterOptions = {};
    adapterOptions.compatibleSurface = wgpu_surface;

    static WGPUAdapter localAdapter;
    WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
    adapterCallbackInfo.callback = handle_request_adapter;
    adapterCallbackInfo.userdata1 = &localAdapter;

    wgpuInstanceRequestAdapter(wgpu_instance, &adapterOptions, adapterCallbackInfo);
    assert(localAdapter);

#ifndef NDEBUG
    WGPUAdapterInfo info = {0};
    wgpuAdapterGetInfo(localAdapter, &info);
    printf("device: %.*s - driver: %.*s\n", (int) info.device.length, info.device.data, (int) info.description.length, info.description.data);
#endif

    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.callback = handle_request_device;
    deviceCallbackInfo.userdata1 = &wgpu_device;

    wgpuAdapterRequestDevice(localAdapter, NULL, deviceCallbackInfo);
    assert(wgpu_device);

    wgpu_surface = GLFW_getWGPUSurface( wgpu_instance, window);
    if (!wgpu_surface)
        return false;

    WGPUSurfaceCapabilities surface_capabilities = {0};
    wgpuSurfaceGetCapabilities(wgpu_surface, localAdapter, &surface_capabilities);

    wgpu_preferred_fmt = surface_capabilities.formats[0];
#endif

    wgpu_surface_configuration.presentMode = WGPUPresentMode_Fifo;
    wgpu_surface_configuration.alphaMode   = WGPUCompositeAlphaMode_Auto;
    wgpu_surface_configuration.usage       = WGPUTextureUsage_RenderAttachment;
    wgpu_surface_configuration.width       = wgpu_surface_width;
    wgpu_surface_configuration.height      = wgpu_surface_height;
    wgpu_surface_configuration.device      = wgpu_device;
    wgpu_surface_configuration.format      = wgpu_preferred_fmt;

    wgpuSurfaceConfigure(wgpu_surface, &wgpu_surface_configuration);

    wgpu_queue    = wgpuDeviceGetQueue(wgpu_device);

    return true;
}
#endif

