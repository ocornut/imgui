// Dear ImGui: standalone example application for QNX Screen + OpenGL ES 3

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_qnx.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>          // printf, fprintf
#include <errno.h>
#include <string.h>
#include <sys/keycodes.h>
#include <screen/screen.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#ifndef EGL_OPENGL_ES3_BIT
#ifdef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT EGL_OPENGL_ES3_BIT_KHR
#else
#define EGL_OPENGL_ES3_BIT 0x0040
#endif
#endif

struct QNXWindow
{
    screen_context_t Context = nullptr;
    screen_window_t Window = nullptr;
    screen_event_t Event = nullptr;
    EGLDisplay Display = EGL_NO_DISPLAY;
    EGLContext GLContext = EGL_NO_CONTEXT;
    EGLSurface GLSurface = EGL_NO_SURFACE;
    int Size[2] = {};
};

// Forward declarations of helper functions
bool CreateQNXWindow(QNXWindow* window);
void DestroyQNXWindow(QNXWindow* window);
bool ProcessQNXEvents(QNXWindow* window);

// Main code
int main(int, char**)
{
    // Setup QNX Screen and EGL
    QNXWindow window;
    if (!CreateQNXWindow(&window))
    {
        DestroyQNXWindow(&window);
        return 1;
    }

    // GL ES 3.0 + GLSL 300 es
    const char* glsl_version = "#version 300 es";
    float main_scale = 1.0f;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplQNX_Init(window.Context, window.Window);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (!ProcessQNXEvents(&window))
            done = true;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplQNX_NewFrame();
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
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        eglSwapBuffers(window.Display, window.GLSurface);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplQNX_Shutdown();
    ImGui::DestroyContext();

    DestroyQNXWindow(&window);

    return 0;
}

// Helper functions

bool CreateQNXWindow(QNXWindow* window)
{
    if (screen_create_context(&window->Context, 0) != 0 ||
        screen_create_window(&window->Window, window->Context) != 0 ||
        screen_create_event(&window->Event) != 0)
    {
        fprintf(stderr, "QNX Screen initialization failed: %s\n", strerror(errno));
        return false;
    }

    // A newly created QNX Screen window reports the fullscreen dimensions.
    if (screen_get_window_property_iv(window->Window, SCREEN_PROPERTY_SIZE, window->Size) != 0 || window->Size[0] <= 0 || window->Size[1] <= 0)
    {
        fprintf(stderr, "Failed to obtain QNX Screen window size: %s\n", strerror(errno));
        return false;
    }

    const int usage = SCREEN_USAGE_OPENGL_ES2 | SCREEN_USAGE_OPENGL_ES3 | SCREEN_USAGE_NATIVE;
    const int transparency = SCREEN_TRANSPARENCY_NONE;
    const int swap_interval = 1;
    if (screen_set_window_property_iv(window->Window, SCREEN_PROPERTY_USAGE, &usage) != 0 ||
        screen_set_window_property_iv(window->Window, SCREEN_PROPERTY_SIZE, window->Size) != 0 ||
        screen_set_window_property_iv(window->Window, SCREEN_PROPERTY_TRANSPARENCY, &transparency) != 0 ||
        screen_set_window_property_iv(window->Window, SCREEN_PROPERTY_SWAP_INTERVAL, &swap_interval) != 0)
    {
        fprintf(stderr, "Failed to configure QNX Screen window: %s\n", strerror(errno));
        return false;
    }

    window->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (window->Display == EGL_NO_DISPLAY || eglInitialize(window->Display, nullptr, nullptr) != EGL_TRUE || eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE)
    {
        fprintf(stderr, "EGL initialization failed: 0x%04x\n", eglGetError());
        return false;
    }

    const EGLint config_attributes[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    EGLConfig config = nullptr;
    EGLint config_count = 0;
    if (eglChooseConfig(window->Display, config_attributes, &config, 1, &config_count) != EGL_TRUE || config_count == 0)
    {
        fprintf(stderr, "eglChooseConfig() failed: 0x%04x\n", eglGetError());
        return false;
    }

    EGLint native_visual = 0;
    if (eglGetConfigAttrib(window->Display, config, EGL_NATIVE_VISUAL_ID, &native_visual) != EGL_TRUE ||
        screen_set_window_property_iv(window->Window, SCREEN_PROPERTY_FORMAT, &native_visual) != 0 ||
        screen_create_window_buffers(window->Window, 2) != 0)
    {
        fprintf(stderr, "Failed to create QNX Screen buffers: %s\n", strerror(errno));
        return false;
    }

    const EGLint context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    window->GLContext = eglCreateContext(window->Display, config, EGL_NO_CONTEXT, context_attributes);
    window->GLSurface = eglCreateWindowSurface(window->Display, config, window->Window, nullptr);
    if (window->GLContext == EGL_NO_CONTEXT || window->GLSurface == EGL_NO_SURFACE ||
        eglMakeCurrent(window->Display, window->GLSurface, window->GLSurface, window->GLContext) != EGL_TRUE ||
        eglSwapInterval(window->Display, 1) != EGL_TRUE)
    {
        fprintf(stderr, "EGL context creation failed: 0x%04x\n", eglGetError());
        return false;
    }

    const int visible = 1;
    if (screen_set_window_property_iv(window->Window, SCREEN_PROPERTY_VISIBLE, &visible) != 0)
    {
        fprintf(stderr, "Failed to show QNX Screen window: %s\n", strerror(errno));
        return false;
    }
    return true;
}

void DestroyQNXWindow(QNXWindow* window)
{
    if (window->Display != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(window->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (window->GLSurface != EGL_NO_SURFACE)
            eglDestroySurface(window->Display, window->GLSurface);
        if (window->GLContext != EGL_NO_CONTEXT)
            eglDestroyContext(window->Display, window->GLContext);
        eglTerminate(window->Display);
    }
    if (window->Event != nullptr)
        screen_destroy_event(window->Event);
    if (window->Window != nullptr)
        screen_destroy_window(window->Window);
    if (window->Context != nullptr)
        screen_destroy_context(window->Context);
}

bool ProcessQNXEvents(QNXWindow* window)
{
    for (;;)
    {
        if (screen_get_event(window->Context, window->Event, 0) != 0)
            return false;

        int type = SCREEN_EVENT_NONE;
        if (screen_get_event_property_iv(window->Event, SCREEN_PROPERTY_TYPE, &type) != 0)
            return false;
        if (type == SCREEN_EVENT_NONE)
            return true;

        ImGui_ImplQNX_ProcessEvent(window->Event);
        if (type == SCREEN_EVENT_CLOSE)
            return false;
        if (type == SCREEN_EVENT_KEYBOARD)
        {
            int flags = 0;
            int key_cap = 0;
            if (screen_get_event_property_iv(window->Event, SCREEN_PROPERTY_FLAGS, &flags) == 0 &&
                screen_get_event_property_iv(window->Event, SCREEN_PROPERTY_KEY_CAP, &key_cap) == 0 &&
                (flags & SCREEN_FLAG_KEY_DOWN) != 0 && key_cap == KEYCODE_ESCAPE)
                return false;
        }
    }
}

