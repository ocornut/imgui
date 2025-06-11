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
#else
#include <webgpu/webgpu_glfw.h>
#endif

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

// Global WebGPU required states
static wgpu::Instance wgpu_instance;
static wgpu::Device wgpu_device;
static wgpu::TextureFormat wgpu_preferred_fmt = wgpu::TextureFormat::Undefined;
static wgpu::Surface wgpu_surface;

static uint32_t wgpu_surface_width = 1280;
static uint32_t wgpu_surface_height = 720;

// Forward declarations
static bool InitWGPU(GLFWwindow *window);

static void glfw_error_callback(int error, const char *description)
{
    printf("GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char **)
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
    init_info.Device = wgpu_device.Get();
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = static_cast<WGPUTextureFormat>(wgpu_preferred_fmt);
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
#ifndef __EMSCRIPTEN__
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }
#endif
        // React to changes in screen size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width != wgpu_surface_width || height != wgpu_surface_height)
        {
            ImGui_ImplWGPU_InvalidateDeviceObjects();

            wgpu_surface_width = width;
            wgpu_surface_height = height;
            wgpu::SurfaceConfiguration config{
                .device = wgpu_device,
                .format = wgpu_preferred_fmt,
                .width = wgpu_surface_width,
                .height = wgpu_surface_height,
                .presentMode = wgpu::PresentMode::Fifo};
            wgpu_surface.Configure(&config);

            ImGui_ImplWGPU_CreateDeviceObjects();
        }

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

        ImGui::Render();

        wgpu::SurfaceTexture surface_texture{};
        wgpu_surface.GetCurrentTexture(&surface_texture);

        wgpu::RenderPassColorAttachment color_attachment{
            .view = surface_texture.texture.CreateView(),
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = wgpu::Color{
                clear_color.x * clear_color.w,
                clear_color.y * clear_color.w,
                clear_color.z * clear_color.w,
                clear_color.w}};

        wgpu::RenderPassDescriptor render_pass{
            .colorAttachmentCount = 1,
            .colorAttachments = &color_attachment};

        wgpu::CommandEncoder encoder = wgpu_device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass);
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
        pass.End();
        wgpu::CommandBuffer commands = encoder.Finish();
        wgpu_device.GetQueue().Submit(1, &commands);

#ifndef __EMSCRIPTEN__
        wgpu_surface.Present();
        wgpu_instance.ProcessEvents();
#endif
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

static bool InitWGPU(GLFWwindow *window)
{
    wgpu::InstanceDescriptor instance_desc{.capabilities = {.timedWaitAnyEnable = true}};
    wgpu_instance = wgpu::CreateInstance(&instance_desc);

    wgpu::Adapter wgpu_adapter{};
    wgpu::Future f1 = wgpu_instance.RequestAdapter(nullptr, wgpu::CallbackMode::WaitAnyOnly,
        [&wgpu_adapter](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message)
        {
            if (status != wgpu::RequestAdapterStatus::Success)
            {
                fprintf(stderr, "RequestAdapter: %s\n", message.data);
                return;
            }
            wgpu_adapter = adapter;
        });
    wgpu_instance.WaitAny(f1, UINT64_MAX);
    if (!wgpu_adapter.Get())
        return 1;

    wgpu::DeviceDescriptor device_desc{};
    device_desc.SetDeviceLostCallback(wgpu::CallbackMode::WaitAnyOnly,
        [](const wgpu::Device &, wgpu::DeviceLostReason reason, wgpu::StringView message)
        {
        fprintf(stderr, "Lost device (%d) %s\n", reason, message.data);
        });
    device_desc.SetUncapturedErrorCallback(
        [](const wgpu::Device &, wgpu::ErrorType errorType, wgpu::StringView message)
        {
            fprintf(stderr, "Uncaptured device error (%d) %s\n", errorType, message.data);
        });

    wgpu::Future f2 = wgpu_adapter.RequestDevice(&device_desc, wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message)
        {
            if (status != wgpu::RequestDeviceStatus::Success)
            {
                fprintf(stderr, "RequestDevice: %s\n", message.data);
                return;
            }
            wgpu_device = std::move(device);
        });
    wgpu_instance.WaitAny(f2, UINT64_MAX);
    if (!wgpu_device.Get())
        return 1;

#ifdef __EMSCRIPTEN__
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector src{{.selector = "#canvas"}};
    wgpu::SurfaceDescriptor surfaceDesc{.nextInChain = &src};
    wgpu_surface = wgpu_instance.CreateSurface(&surfaceDesc);
#else
    wgpu_surface = wgpu::glfw::CreateSurfaceForWindow(wgpu_instance, window);
#endif

    wgpu::SurfaceCapabilities capabilities{};
    wgpu_surface.GetCapabilities(wgpu_adapter, &capabilities);
    wgpu_preferred_fmt = capabilities.formats[0];

    wgpu::SurfaceConfiguration config{
        .device = wgpu_device,
        .format = wgpu_preferred_fmt,
        .width = wgpu_surface_width,
        .height = wgpu_surface_height,
        .presentMode = wgpu::PresentMode::Fifo};
    wgpu_surface.Configure(&config);

    return true;
}
