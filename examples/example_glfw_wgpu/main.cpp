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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h> // for wgpuDevicePoll
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

// Global WebGPU required states
static WGPUInstance      wgpu_instance = nullptr;
static WGPUDevice        wgpu_device = nullptr;
static WGPUSurface       wgpu_surface = nullptr;
static WGPUTextureFormat wgpu_preferred_fmt = WGPUTextureFormat_RGBA8Unorm;
static int               wgpu_surface_width = 1280;
static int               wgpu_surface_height = 720;

// Forward declarations
static bool InitWGPU(GLFWwindow* window);
static void ConfigureSurface(int width, int height);
static WGPUTextureView GetCurrentSurfaceTextureView();

static void glfw_error_callback(int error, const char* description)
{
    printf("GLFW Error %d: %s\n", error, description);
}

static void wgpu_error_callback(WGPUErrorType error_type, const char* message, void*)
{
    const char* error_type_lbl = "";
    switch (error_type)
    {
    case WGPUErrorType_Validation:  error_type_lbl = "Validation"; break;
    case WGPUErrorType_OutOfMemory: error_type_lbl = "Out of memory"; break;
    case WGPUErrorType_Unknown:     error_type_lbl = "Unknown"; break;
    case WGPUErrorType_DeviceLost:  error_type_lbl = "Device lost"; break;
    default:                        error_type_lbl = "Unknown";
    }
    printf("%s error: %s\n", error_type_lbl, message);
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
        if (window)
            glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    ConfigureSurface(wgpu_surface_width, wgpu_surface_height);
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
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
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

        // Poll WebGPU events
#if defined(WEBGPU_BACKEND_DAWN)
        // Tick needs to be called in Dawn to display validation errors
        wgpuDeviceTick(wgpu_device);
#elif defined(WEBGPU_BACKEND_WGPU)
        wgpuDevicePoll(wgpu_device, false, nullptr);
#elif defined(__EMSCRIPTEN__)
        // Nothing to do
#endif

        // React to changes in screen size
        int width, height;
        glfwGetFramebufferSize((GLFWwindow*)window, &width, &height);
        if (width != wgpu_surface_width || height != wgpu_surface_height)
        {
            ImGui_ImplWGPU_InvalidateDeviceObjects();
            ConfigureSurface(width, height);
            ImGui_ImplWGPU_CreateDeviceObjects();
        }

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

        WGPURenderPassColorAttachment color_attachments = {};
#ifndef WEBGPU_BACKEND_WGPU // not supported yet by wgpu-native
        color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif
        color_attachments.loadOp = WGPULoadOp_Clear;
        color_attachments.storeOp = WGPUStoreOp_Store;
        color_attachments.clearValue = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        color_attachments.view = GetCurrentSurfaceTextureView();

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
        WGPUQueue queue = wgpuDeviceGetQueue(wgpu_device);
        wgpuQueueSubmit(queue, 1, &cmd_buffer);

#ifndef __EMSCRIPTEN__
        wgpuSurfacePresent(wgpu_surface);
#endif

        wgpuTextureViewRelease(color_attachments.view);
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

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

#ifndef __EMSCRIPTEN__
static WGPUAdapter RequestAdapter(WGPUInstance instance)
{
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* pUserData)
    {
        if (status == WGPURequestAdapterStatus_Success)
            *(WGPUAdapter*)(pUserData) = adapter;
        else
            printf("Could not get WebGPU adapter: %s\n", message);
};
    WGPUAdapter adapter;
    wgpuInstanceRequestAdapter(instance, nullptr, onAdapterRequestEnded, (void*)&adapter);
    return adapter;
}

static WGPUDevice RequestDevice(WGPUAdapter& adapter)
{
    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* pUserData)
    {
        if (status == WGPURequestDeviceStatus_Success)
            *(WGPUDevice*)(pUserData) = device;
        else
            printf("Could not get WebGPU device: %s\n", message);
    };
    WGPUDevice device;
    wgpuAdapterRequestDevice(adapter, nullptr, onDeviceRequestEnded, (void*)&device);
    return device;
}
#endif

static bool InitWGPU(GLFWwindow* window)
{
    WGPUInstance instance = wgpuCreateInstance(nullptr);

#ifdef __EMSCRIPTEN__
    WGPUAdapter adapter = nullptr;
    wgpu_device = emscripten_webgpu_get_device();
#else
    WGPUAdapter adapter = RequestAdapter(instance);
    if (!adapter)
        return false;
    wgpu_device = RequestDevice(adapter);
    wgpuAdapterRelease(adapter);
#endif
    if (!wgpu_device)
        return false;

    wgpu_surface = glfwGetWGPUSurface(instance, window);
    if (!wgpu_surface)
        return false;
    wgpu_preferred_fmt = wgpuSurfaceGetPreferredFormat(wgpu_surface, adapter);

    wgpuDeviceSetUncapturedErrorCallback(wgpu_device, wgpu_error_callback, nullptr);

    wgpuInstanceRelease(instance);
    return true;
}

static void ConfigureSurface(int width, int height)
{
    wgpu_surface_width = width;
    wgpu_surface_height = height;
    WGPUSurfaceConfiguration surface_config = {};
    surface_config.device = wgpu_device;
    surface_config.usage = WGPUTextureUsage_RenderAttachment;
    surface_config.format = wgpu_preferred_fmt;
    surface_config.width = width;
    surface_config.height = height;
    surface_config.viewFormatCount = 0;
    surface_config.viewFormats = nullptr;
    surface_config.presentMode = WGPUPresentMode_Fifo;
    surface_config.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(wgpu_surface, &surface_config);
}

static WGPUTextureView GetCurrentSurfaceTextureView() {
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(wgpu_surface, &surfaceTexture);
    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        return nullptr;
    }
    WGPUTextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    viewDescriptor.dimension = WGPUTextureViewDimension_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    return wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
}
