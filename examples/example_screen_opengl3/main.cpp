// Dear ImGui: standalone example application for QNX Screen + OpenGL ES 3

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_screen.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <errno.h>
#include <screen/screen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/keycodes.h>

#ifndef EGL_OPENGL_ES3_BIT
#ifdef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT EGL_OPENGL_ES3_BIT_KHR
#else
#define EGL_OPENGL_ES3_BIT 0x0040
#endif
#endif

struct AppWindow
{
    screen_context_t ScreenContext;
    screen_window_t  ScreenWindow;
    screen_event_t   ScreenEvent;
    EGLDisplay       EglDisplay;
    EGLConfig        EglConfig;
    EGLContext       EglContext;
    EGLSurface       EglSurface;
    int              WindowSize[2];

    AppWindow()
    {
        memset(this, 0, sizeof(*this));
        EglDisplay = EGL_NO_DISPLAY;
        EglContext = EGL_NO_CONTEXT;
        EglSurface = EGL_NO_SURFACE;
    }
};

static void PrintEglError(const char* operation)
{
    fprintf(stderr, "%s failed: EGL error 0x%04x\n", operation, eglGetError());
}

static bool CreateScreenWindow(AppWindow* app)
{
    if (screen_create_context(&app->ScreenContext, 0) != 0)
    {
        fprintf(stderr, "screen_create_context() failed: %s\n", strerror(errno));
        return false;
    }
    if (screen_create_window(&app->ScreenWindow, app->ScreenContext) != 0)
    {
        fprintf(stderr, "screen_create_window() failed: %s\n", strerror(errno));
        return false;
    }
    if (screen_create_event(&app->ScreenEvent) != 0)
    {
        fprintf(stderr, "screen_create_event() failed: %s\n", strerror(errno));
        return false;
    }
    if (screen_get_window_property_iv(app->ScreenWindow, SCREEN_PROPERTY_SIZE, app->WindowSize) != 0)
    {
        fprintf(stderr, "screen_get_window_property_iv(SCREEN_PROPERTY_SIZE) failed: %s\n", strerror(errno));
        return false;
    }
    if (app->WindowSize[0] <= 0 || app->WindowSize[1] <= 0)
    {
        fprintf(stderr, "screen_get_window_property_iv(SCREEN_PROPERTY_SIZE) returned invalid size %dx%d\n",
                app->WindowSize[0], app->WindowSize[1]);
        return false;
    }

    const char* id = "dear-imgui-screen-opengl3";
    (void)screen_set_window_property_cv(app->ScreenWindow, SCREEN_PROPERTY_ID_STRING, (int)strlen(id) + 1, id);

    const int usage = SCREEN_USAGE_OPENGL_ES2 | SCREEN_USAGE_OPENGL_ES3 | SCREEN_USAGE_NATIVE;
    const int transparency = SCREEN_TRANSPARENCY_NONE;
    const int swap_interval = 1;
    if (screen_set_window_property_iv(app->ScreenWindow, SCREEN_PROPERTY_USAGE, &usage) != 0 ||
        screen_set_window_property_iv(app->ScreenWindow, SCREEN_PROPERTY_SIZE, app->WindowSize) != 0 ||
        screen_set_window_property_iv(app->ScreenWindow, SCREEN_PROPERTY_TRANSPARENCY, &transparency) != 0 ||
        screen_set_window_property_iv(app->ScreenWindow, SCREEN_PROPERTY_SWAP_INTERVAL, &swap_interval) != 0)
    {
        fprintf(stderr, "Failed to configure QNX Screen window: %s\n", strerror(errno));
        return false;
    }
    return true;
}

static bool CreateEglContext(AppWindow* app)
{
    app->EglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (app->EglDisplay == EGL_NO_DISPLAY)
    {
        PrintEglError("eglGetDisplay()");
        return false;
    }
    if (eglInitialize(app->EglDisplay, nullptr, nullptr) != EGL_TRUE)
    {
        PrintEglError("eglInitialize()");
        return false;
    }
    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE)
    {
        PrintEglError("eglBindAPI()");
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
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_NONE
    };
    EGLint config_count = 0;
    if (eglChooseConfig(app->EglDisplay, config_attributes, &app->EglConfig, 1, &config_count) != EGL_TRUE || config_count < 1)
    {
        PrintEglError("eglChooseConfig()");
        return false;
    }

    EGLint native_visual = 0;
    if (eglGetConfigAttrib(app->EglDisplay, app->EglConfig, EGL_NATIVE_VISUAL_ID, &native_visual) != EGL_TRUE)
    {
        PrintEglError("eglGetConfigAttrib(EGL_NATIVE_VISUAL_ID)");
        return false;
    }
    if (screen_set_window_property_iv(app->ScreenWindow, SCREEN_PROPERTY_FORMAT, &native_visual) != 0)
    {
        fprintf(stderr, "screen_set_window_property_iv(SCREEN_PROPERTY_FORMAT) failed: %s\n", strerror(errno));
        return false;
    }

    const EGLint context_attributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    app->EglContext = eglCreateContext(app->EglDisplay, app->EglConfig, EGL_NO_CONTEXT, context_attributes);
    if (app->EglContext == EGL_NO_CONTEXT)
    {
        PrintEglError("eglCreateContext()");
        return false;
    }

    if (screen_create_window_buffers(app->ScreenWindow, 2) != 0)
    {
        fprintf(stderr, "screen_create_window_buffers() failed: %s\n", strerror(errno));
        return false;
    }

    app->EglSurface = eglCreateWindowSurface(app->EglDisplay, app->EglConfig, app->ScreenWindow, nullptr);
    if (app->EglSurface == EGL_NO_SURFACE)
    {
        PrintEglError("eglCreateWindowSurface()");
        return false;
    }
    if (eglMakeCurrent(app->EglDisplay, app->EglSurface, app->EglSurface, app->EglContext) != EGL_TRUE)
    {
        PrintEglError("eglMakeCurrent()");
        return false;
    }
    if (eglSwapInterval(app->EglDisplay, 1) != EGL_TRUE)
    {
        PrintEglError("eglSwapInterval()");
        return false;
    }

    const int visible = 1;
    if (screen_set_window_property_iv(app->ScreenWindow, SCREEN_PROPERTY_VISIBLE, &visible) != 0)
    {
        fprintf(stderr, "screen_set_window_property_iv(SCREEN_PROPERTY_VISIBLE) failed: %s\n", strerror(errno));
        return false;
    }
    return true;
}

static bool IsExitEvent(screen_event_t event)
{
    int type = SCREEN_EVENT_NONE;
    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &type) != 0)
    {
        return false;
    }
    if (type == SCREEN_EVENT_KEYBOARD)
    {
        int flags = 0;
        int symbol = 0;
        if (screen_get_event_property_iv(event, SCREEN_PROPERTY_FLAGS, &flags) == 0 &&
            screen_get_event_property_iv(event, SCREEN_PROPERTY_SYM, &symbol) == 0 &&
            (flags & KEY_DOWN) != 0 && symbol == KEYCODE_ESCAPE)
        {
            return true;
        }
    }
    if (type == SCREEN_EVENT_CLOSE)
    {
        return true;
    }
    return false;
}

static bool ProcessScreenEvents(AppWindow* app)
{
    for (;;)
    {
        if (screen_get_event(app->ScreenContext, app->ScreenEvent, 0) != 0)
        {
            fprintf(stderr, "screen_get_event() failed: %s\n", strerror(errno));
            return false;
        }

        int type = SCREEN_EVENT_NONE;
        if (screen_get_event_property_iv(app->ScreenEvent, SCREEN_PROPERTY_TYPE, &type) != 0)
        {
            fprintf(stderr, "screen_get_event_property_iv(SCREEN_PROPERTY_TYPE) failed: %s\n", strerror(errno));
            return false;
        }
        if (type == SCREEN_EVENT_NONE)
        {
            break;
        }
        if (IsExitEvent(app->ScreenEvent))
        {
            return false;
        }
        ImGui_ImplScreen_ProcessEvent(app->ScreenEvent);
    }
    return true;
}

static void DestroyAppWindow(AppWindow* app)
{
    if (app->EglDisplay != EGL_NO_DISPLAY)
    {
        (void)eglMakeCurrent(app->EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (app->EglSurface != EGL_NO_SURFACE)
        {
            eglDestroySurface(app->EglDisplay, app->EglSurface);
        }
        if (app->EglContext != EGL_NO_CONTEXT)
        {
            eglDestroyContext(app->EglDisplay, app->EglContext);
        }
        eglTerminate(app->EglDisplay);
    }
    if (app->ScreenEvent != nullptr)
    {
        screen_destroy_event(app->ScreenEvent);
    }
    if (app->ScreenWindow != nullptr)
    {
        screen_destroy_window(app->ScreenWindow);
    }
    if (app->ScreenContext != nullptr)
    {
        screen_destroy_context(app->ScreenContext);
    }
}

int main(int, char**)
{
    AppWindow app;
    if (!CreateScreenWindow(&app) || !CreateEglContext(&app))
    {
        DestroyAppWindow(&app);
        return EXIT_FAILURE;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    const bool screen_backend_initialized = ImGui_ImplScreen_Init(app.ScreenContext, app.ScreenWindow);
    const bool renderer_backend_initialized = screen_backend_initialized && ImGui_ImplOpenGL3_Init("#version 300 es");
    if (!screen_backend_initialized || !renderer_backend_initialized)
    {
        fprintf(stderr, "Failed to initialize Dear ImGui backends.\n");
        if (renderer_backend_initialized)
        {
            ImGui_ImplOpenGL3_Shutdown();
        }
        if (screen_backend_initialized)
        {
            ImGui_ImplScreen_Shutdown();
        }
        ImGui::DestroyContext();
        DestroyAppWindow(&app);
        return EXIT_FAILURE;
    }

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);
    bool running = true;
    while (running)
    {
        running = ProcessScreenEvents(&app);
        if (!running)
        {
            break;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplScreen_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        static float value = 0.0f;
        static int counter = 0;
        static char text[128] = "Keyboard input is handled by QNX Screen";
        ImGui::Begin("QNX Screen + OpenGL ES 3");
        ImGui::Text("Renderer: %s", (const char*)glGetString(GL_RENDERER));
        ImGui::Checkbox("Demo Window", &show_demo_window);
        ImGui::Checkbox("Another Window", &show_another_window);
        ImGui::SliderFloat("float", &value, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        ImGui::InputText("text", text, IM_ARRAYSIZE(text));
        if (ImGui::Button("Button"))
        {
            counter++;
        }
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::TextUnformatted("Press Esc to exit.");
        ImGui::End();

        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::TextUnformatted("Hello from QNX Screen!");
            if (ImGui::Button("Close Me"))
            {
                show_another_window = false;
            }
            ImGui::End();
        }

        ImGui::Render();
        EGLint framebuffer_width = 0;
        EGLint framebuffer_height = 0;
        eglQuerySurface(app.EglDisplay, app.EglSurface, EGL_WIDTH, &framebuffer_width);
        eglQuerySurface(app.EglDisplay, app.EglSurface, EGL_HEIGHT, &framebuffer_height);
        if (framebuffer_width > 0 && framebuffer_height > 0)
        {
            glViewport(0, 0, framebuffer_width, framebuffer_height);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if (eglSwapBuffers(app.EglDisplay, app.EglSurface) != EGL_TRUE)
            {
                PrintEglError("eglSwapBuffers()");
                running = false;
            }
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplScreen_Shutdown();
    ImGui::DestroyContext();
    DestroyAppWindow(&app);
    return EXIT_SUCCESS;
}
