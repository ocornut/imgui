// Dear ImGui: standalone example application for using Emscripten + WebGPU
// - This uses imgui_impl_emscripten for platform inputs and imgui_impl_wgpu for rendering.
// - Emscripten is required to build this example. See https://emscripten.org.
// - Dawn is used as the WebGPU implementation via Emscripten's emdawnwebgpu port.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_emscripten.h"
#include "imgui_impl_wgpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <emscripten.h>
#include <emscripten/html5.h>
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#if !defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
#error This example requires IMGUI_IMPL_WEBGPU_BACKEND_DAWN.
#endif

// Data
static WGPUInstance             wgpu_instance{nullptr};
static WGPUDevice               wgpu_device{nullptr};
static WGPUSurface              wgpu_surface{nullptr};
static WGPUQueue                wgpu_queue{nullptr};
static WGPUSurfaceConfiguration wgpu_surface_configuration{};
static int                      wgpu_surface_width = 1280;
static int                      wgpu_surface_height = 800;

// Forward declarations
static bool InitWGPU();
static void GetFramebufferSizeFromDisplaySize(int width, int height, int* framebuffer_width, int* framebuffer_height);
static void ResizeSurface(int width, int height);
static WGPUAdapter RequestAdapter(wgpu::Instance& instance);
static WGPUDevice RequestDevice(wgpu::Instance& instance, wgpu::Adapter& adapter);

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
    wgpu::Future wait_adapter_func { instance.RequestAdapter(&adapter_options, wgpu::CallbackMode::WaitAnyOnly, onRequestAdapter) };
    wgpu::WaitStatus wait_status_adapter = instance.WaitAny(wait_adapter_func, UINT64_MAX);
    IM_ASSERT(acquired_adapter != nullptr && wait_status_adapter == wgpu::WaitStatus::Success && "Error on Adapter request");
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
            printf("Failed to get a device: %s\n", message.data);
            return;
        }
        acquired_device = std::move(local_device);
    };

    // Synchronously (wait until) get Device
    wgpu::Future wait_device_func { adapter.RequestDevice(&device_desc, wgpu::CallbackMode::WaitAnyOnly, onRequestDevice) };
    wgpu::WaitStatus wait_status_device = instance.WaitAny(wait_device_func, UINT64_MAX);
    IM_ASSERT(acquired_device != nullptr && wait_status_device == wgpu::WaitStatus::Success && "Error on Device request");
    return acquired_device.MoveToCHandle();
}

static bool InitWGPU()
{
    WGPUTextureFormat preferred_fmt{WGPUTextureFormat_Undefined};

    wgpu::InstanceDescriptor instance_desc{};
    static constexpr wgpu::InstanceFeatureName timedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    instance_desc.requiredFeatureCount = 1;
    instance_desc.requiredFeatures = &timedWaitAny;
    wgpu::Instance instance{wgpu::CreateInstance(&instance_desc)};

    wgpu::Adapter adapter = RequestAdapter(instance);
    ImGui_ImplWGPU_DebugPrintAdapterInfo(adapter.Get());

    wgpu_device = RequestDevice(instance, adapter);

    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvas_desc{};
    canvas_desc.selector = "#canvas";

    wgpu::SurfaceDescriptor surface_desc{};
    surface_desc.nextInChain = &canvas_desc;
    wgpu_surface = instance.CreateSurface(&surface_desc).MoveToCHandle();
    if (!wgpu_surface) return false;

    wgpu_instance = instance.MoveToCHandle();

    WGPUSurfaceCapabilities surface_capabilities{};
    wgpuSurfaceGetCapabilities(wgpu_surface, adapter.Get(), &surface_capabilities);
    preferred_fmt = surface_capabilities.formats[0];

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

static void GetFramebufferSizeFromDisplaySize(int width, int height, int* framebuffer_width, int* framebuffer_height)
{
    ImGuiIO& io{ImGui::GetIO()};
    *framebuffer_width = static_cast<int>(width * io.DisplayFramebufferScale.x + 0.5f);
    *framebuffer_height = static_cast<int>(height * io.DisplayFramebufferScale.y + 0.5f);
}

static void ResizeSurface(int width, int height)
{
    if (width <= 0 || height <= 0) return;

    int framebuffer_width{0};
    int framebuffer_height{0};
    GetFramebufferSizeFromDisplaySize(width, height, &framebuffer_width, &framebuffer_height);
    if (framebuffer_width <= 0 || framebuffer_height <= 0) return;

    emscripten_set_canvas_element_size("#canvas", framebuffer_width, framebuffer_height);
    wgpu_surface_configuration.width = wgpu_surface_width = framebuffer_width;
    wgpu_surface_configuration.height = wgpu_surface_height = framebuffer_height;
    wgpuSurfaceConfigure(wgpu_surface, &wgpu_surface_configuration);
}

// Main code
int main(int, char**)
{
    if (!InitWGPU()) return 1;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io{ImGui::GetIO()}; (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;                       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;                        // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplEmscripten_Init();
    if (io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f) ResizeSurface((int)io.DisplaySize.x, (int)io.DisplaySize.y);

    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = wgpu_device;
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = wgpu_surface_configuration.format;
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&init_info);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
#ifndef IMGUI_DISABLE_FILE_FUNCTIONS
    //io.Fonts->AddFontFromFileTTF("fonts/segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("fonts/ArialUni.ttf");
    //IM_ASSERT(font != nullptr);
#endif

    // Our state
    bool show_demo_window{true};
    bool show_another_window{false};
    ImVec4 clear_color{0.45f, 0.55f, 0.60f, 1.00f};

    // Main loop
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
    {
        // Input handling is callback-driven via imgui_impl_emscripten, so there is no event pump here.

        // React to changes in browser window size.
        int width = (int)io.DisplaySize.x;
        int height = (int)io.DisplaySize.y;
        if (width <= 0 || height <= 0)
            continue;

        int framebuffer_width{0};
        int framebuffer_height{0};
        GetFramebufferSizeFromDisplaySize(width, height, &framebuffer_width, &framebuffer_height);
        if (framebuffer_width != wgpu_surface_width || framebuffer_height != wgpu_surface_height) ResizeSurface(width, height);

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
            if (surface_texture.texture) wgpuTextureRelease(surface_texture.texture);
            ResizeSurface(width, height);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplEmscripten_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f{0.0f};
            static int counter{0};

            ImGui::Begin("Hello, world!");                                      // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");                           // Display some text (you can use a format strings too)
            ImGui::Text("Platform backend: imgui_impl_emscripten");
            ImGui::Checkbox("Demo Window", &show_demo_window);                  // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);                        // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color);             // Edit 3 floats representing a color

            if (ImGui::Button("Button")) ++counter;                             // Buttons return true when clicked (most widgets return true when edited/activated)
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);               // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me")) show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();

        WGPUTextureViewDescriptor view_desc{};
        view_desc.format = wgpu_surface_configuration.format;
        view_desc.dimension = WGPUTextureViewDimension_2D;
        view_desc.mipLevelCount = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
        view_desc.arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
        view_desc.aspect = WGPUTextureAspect_All;

        WGPUTextureView texture_view = wgpuTextureCreateView(surface_texture.texture, &view_desc);

        WGPURenderPassColorAttachment color_attachments{};
        color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        color_attachments.loadOp = WGPULoadOp_Clear;
        color_attachments.storeOp = WGPUStoreOp_Store;
        color_attachments.clearValue = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        color_attachments.view = texture_view;

        WGPURenderPassDescriptor render_pass_desc{};
        render_pass_desc.colorAttachmentCount = 1;
        render_pass_desc.colorAttachments = &color_attachments;
        render_pass_desc.depthStencilAttachment = nullptr;

        WGPUCommandEncoderDescriptor enc_desc{};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wgpu_device, &enc_desc);

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
        wgpuRenderPassEncoderEnd(pass);

        WGPUCommandBufferDescriptor cmd_buffer_desc{};
        WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        wgpuQueueSubmit(wgpu_queue, 1, &cmd_buffer);

        wgpuTextureViewRelease(texture_view);
        wgpuRenderPassEncoderRelease(pass);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(cmd_buffer);
    }
    EMSCRIPTEN_MAINLOOP_END;

    // Cleanup
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplEmscripten_Shutdown();
    ImGui::DestroyContext();

    wgpuSurfaceUnconfigure(wgpu_surface);
    wgpuSurfaceRelease(wgpu_surface);
    wgpuQueueRelease(wgpu_queue);
    wgpuDeviceRelease(wgpu_device);
    wgpuInstanceRelease(wgpu_instance);

    return 0;
}
