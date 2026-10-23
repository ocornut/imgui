// Dear ImGui: standalone example application for using Emscripten + OpenGL 3
// - This uses imgui_impl_emscripten for platform inputs and imgui_impl_opengl3 for rendering.
// - Emscripten is required to build this example. See https://emscripten.org.
// - WebGL 1 is used by default. Define IMGUI_IMPL_OPENGL_ES3 and enable WebGL 2 to use WebGL 2 instead.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_emscripten.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#elif defined(IMGUI_IMPL_OPENGL_ES3)
#include <GLES3/gl3.h>
#endif
#include <emscripten.h>
#include <emscripten/html5.h>
#include "../libs/emscripten/emscripten_mainloop_stub.h"

// Data
static int gl_framebuffer_width = 0;
static int gl_framebuffer_height = 0;

// Forward declarations
static void GetFramebufferSizeFromDisplaySize(int width, int height, int* framebuffer_width, int* framebuffer_height);
static void ResizeCanvas(int width, int height);

static void GetFramebufferSizeFromDisplaySize(int width, int height, int* framebuffer_width, int* framebuffer_height)
{
    ImGuiIO& io = ImGui::GetIO();
    *framebuffer_width = (int)(width * io.DisplayFramebufferScale.x + 0.5f);
    *framebuffer_height = (int)(height * io.DisplayFramebufferScale.y + 0.5f);
}

static void ResizeCanvas(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    int framebuffer_width = 0;
    int framebuffer_height = 0;
    GetFramebufferSizeFromDisplaySize(width, height, &framebuffer_width, &framebuffer_height);
    if (framebuffer_width <= 0 || framebuffer_height <= 0)
        return;

    emscripten_set_canvas_element_size("#canvas", framebuffer_width, framebuffer_height);
    gl_framebuffer_width = framebuffer_width;
    gl_framebuffer_height = framebuffer_height;
}

// Main code
int main(int, char**)
{
    // Decide GL+GLSL versions
    EmscriptenWebGLContextAttributes context_attributes;
    emscripten_webgl_init_context_attributes(&context_attributes);
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    context_attributes.majorVersion = 1;
    context_attributes.minorVersion = 0;
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    context_attributes.majorVersion = 2;
    context_attributes.minorVersion = 0;
#endif

    // Create window with graphics context
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl_context = emscripten_webgl_create_context("#canvas", &context_attributes);
    if (gl_context == 0)
    {
        fprintf(stderr, "Failed to create WebGL context.\n");
        return 1;
    }
    if (emscripten_webgl_make_context_current(gl_context) != EMSCRIPTEN_RESULT_SUCCESS)
    {
        fprintf(stderr, "Failed to make WebGL context current.\n");
        emscripten_webgl_destroy_context(gl_context);
        return 1;
    }

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
    ImGui_ImplEmscripten_Init();
    if (io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f)
        ResizeCanvas((int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui_ImplOpenGL3_Init(glsl_version);

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
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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

        int framebuffer_width = 0;
        int framebuffer_height = 0;
        GetFramebufferSizeFromDisplaySize(width, height, &framebuffer_width, &framebuffer_height);
        if (framebuffer_width != gl_framebuffer_width || framebuffer_height != gl_framebuffer_height)
            ResizeCanvas(width, height);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplEmscripten_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
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
        glViewport(0, 0, framebuffer_width, framebuffer_height);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    EMSCRIPTEN_MAINLOOP_END;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplEmscripten_Shutdown();
    ImGui::DestroyContext();

    emscripten_webgl_destroy_context(gl_context);

    return 0;
}
