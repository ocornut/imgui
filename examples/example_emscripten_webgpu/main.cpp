// dear imgui: standalone example application for Emscripten, using GLFW + WebGPU
// It is possible to combine both code into a single source file that will compile properly on Desktop and using Emscripten.
//
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (Emscripten is a C++-to-javascript compiler, used to publish executables for the web. See https://emscripten.org/)

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"
#include <stdio.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

static WGPUDevice    wgpu_device = NULL;
static WGPUSurface   wgpu_surface = NULL;
static WGPUSwapChain wgpu_swap_chain = NULL;

static int           wgpu_swap_chain_width = 0;
static int           wgpu_swap_chain_height = 0;


static bool show_demo_window = true;
static bool show_another_window = false;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

static void printGlfwError(int error, const char* description)
{
    printf("Glfw Error %d: %s\n", error, description);
}

static void printDeviceError(WGPUErrorType error_type, const char* message, void*)
{
    const char* error_type_lbl = "";
    switch (error_type) {
    case WGPUErrorType_Validation:
        error_type_lbl = "Validation";
        break;
    case WGPUErrorType_OutOfMemory:
        error_type_lbl = "Out of memory";
        break;
    case WGPUErrorType_Unknown:
        error_type_lbl = "Unknown";
        break;
    case WGPUErrorType_DeviceLost:
        error_type_lbl = "Device lost";
        break;
    default:
        error_type_lbl = "Unknown";
    }

    printf("%s error: %s\n", error_type_lbl, message);
}

bool initWebGPU()
{
    wgpu_device = emscripten_webgpu_get_device();
    if (!wgpu_device)
        return false;

    wgpuDeviceSetUncapturedErrorCallback(wgpu_device, printDeviceError, nullptr);

    // Use C++ wrapper due to malbehaviour in Emscripten.
    // Some offset computation for wgpuInstanceCreateSurface in JavaScript
    // seem to be inline with struct alignments in the C++ structure
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc{};
    html_surface_desc.selector = "#canvas";

    wgpu::SurfaceDescriptor surface_desc{};
    surface_desc.nextInChain = &html_surface_desc;

    // Use 'null' instance
    wgpu::Instance instance{};
    wgpu_surface = instance.CreateSurface(&surface_desc).Release();

    return true;
}


void invalidateDeviceObjects()
{
    ImGui_ImplWGPU_InvalidateDeviceObjects();
}
void createDeviceObjects()
{
    ImGui_ImplWGPU_CreateDeviceObjects();
}

void resizeSwapChain(GLFWwindow* window, unsigned int width, unsigned int height)
{
    invalidateDeviceObjects();

    if (wgpu_swap_chain)
        wgpuSwapChainRelease(wgpu_swap_chain);

    wgpu_swap_chain_width = width;
    wgpu_swap_chain_height = height;

    WGPUSwapChainDescriptor swap_chain_desc = {};
    swap_chain_desc.usage = WGPUTextureUsage_OutputAttachment;
    swap_chain_desc.format = WGPUTextureFormat_RGBA8Unorm;
    swap_chain_desc.width = width;
    swap_chain_desc.height = height;
    swap_chain_desc.presentMode = WGPUPresentMode_Fifo;
    wgpu_swap_chain = wgpuDeviceCreateSwapChain(wgpu_device, wgpu_surface, &swap_chain_desc);

    createDeviceObjects();
}

void initializeImGui(GLFWwindow* window)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplWGPU_Init(wgpu_device, 3, WGPUTextureFormat_RGBA8Unorm);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
}

void updateFrame()
{
   ImGui_ImplWGPU_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();
   
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");						  // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");			   // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);	  // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);			// Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))							// Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
}

void renderFrame(WGPUTextureView back_buffer)
{
    WGPURenderPassColorAttachmentDescriptor color_attachments = {};
    color_attachments.loadOp = WGPULoadOp_Clear;
    color_attachments.storeOp = WGPUStoreOp_Store;
    color_attachments.clearColor = { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
    color_attachments.attachment = back_buffer;
    WGPURenderPassDescriptor render_pass_desc = {};
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &color_attachments;
    render_pass_desc.depthStencilAttachment = nullptr;

    WGPUCommandEncoderDescriptor enc_desc = {};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wgpu_device, &enc_desc);
    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);

    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);

    wgpuRenderPassEncoderEndPass(pass);

    WGPUCommandBufferDescriptor cmd_buffer_desc = {};
    WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
    WGPUQueue queue = wgpuDeviceGetDefaultQueue(wgpu_device);
    wgpuQueueSubmit(queue, 1, &cmd_buffer);
}

void mainLoop(void* window)
{
    glfwPollEvents();

    int width, height;
    glfwGetFramebufferSize((GLFWwindow*) window, &width, &height);

    if (width != wgpu_swap_chain_width && height != wgpu_swap_chain_height)
    {
        resizeSwapChain((GLFWwindow*) window, width, height);
    }

    // Allow to update the state of objects before waiting for the GPU
    updateFrame();
    
    auto back_buffer = wgpuSwapChainGetCurrentTextureView(wgpu_swap_chain);
    renderFrame(back_buffer);
}

int main(int, char**)
{
    glfwSetErrorCallback(printGlfwError);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+WebGPU example", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    if (!initWebGPU()) {
        if (window)
            glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    initializeImGui(window);
    glfwShowWindow(window);

    emscripten_set_main_loop_arg(mainLoop, window, 0, false);

    return 0;
}
