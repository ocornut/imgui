// dear imgui: standalone example application for Android + OpenGL ES 3
// This example demonstrates the clean imgui_impl_android backend.
// The backend handles soft keyboard, Unicode input, and clipboard internally via JNI.
// No JNI boilerplate needed in the application.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>

// EGL state — standard boilerplate for Android native apps
static EGLDisplay          g_EglDisplay = EGL_NO_DISPLAY;
static EGLSurface          g_EglSurface = EGL_NO_SURFACE;
static EGLContext          g_EglContext = EGL_NO_CONTEXT;
static struct android_app* g_App = nullptr;
static bool                g_Initialized = false;
static char                g_LogTag[] = "ImGuiExample";
static std::string         g_IniFilename;

// Forward declarations
static void Init(struct android_app* app);
static void Shutdown();
static void MainLoopStep();

static void handleAppCmd(struct android_app* app, int32_t appCmd)
{
    switch (appCmd)
    {
    case APP_CMD_INIT_WINDOW:
        Init(app);
        break;
    case APP_CMD_TERM_WINDOW:
        Shutdown();
        break;
    default:
        break;
    }
}

static int32_t handleInputEvent(struct android_app* /*app*/, AInputEvent* inputEvent)
{
    return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
}

void android_main(struct android_app* app)
{
    app->onAppCmd = handleAppCmd;
    app->onInputEvent = handleInputEvent;

    while (true)
    {
        int out_events;
        struct android_poll_source* out_data;

        // Poll all events. If the app is not visible, this loop blocks until g_Initialized == true.
        while (ALooper_pollOnce(g_Initialized ? 0 : -1, nullptr, &out_events, (void**)&out_data) >= 0)
        {
            if (out_data != nullptr)
                out_data->process(app, out_data);

            if (app->destroyRequested != 0)
            {
                if (g_Initialized)
                    Shutdown();
                return;
            }
        }

        MainLoopStep();
    }
}

static void Init(struct android_app* app)
{
    if (g_Initialized)
        return;

    g_App = app;
    ANativeWindow_acquire(g_App->window);

    // --- EGL initialization (standard Android boilerplate) ---
    g_EglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_EglDisplay == EGL_NO_DISPLAY)
        __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglGetDisplay() returned EGL_NO_DISPLAY");

    if (eglInitialize(g_EglDisplay, 0, 0) != EGL_TRUE)
        __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglInitialize() returned with an error");

    const EGLint egl_attributes[] = {
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 24, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE
    };
    EGLint num_configs = 0;
    if (eglChooseConfig(g_EglDisplay, egl_attributes, nullptr, 0, &num_configs) != EGL_TRUE || num_configs == 0)
        __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglChooseConfig() returned 0 matching config");

    EGLConfig egl_config;
    eglChooseConfig(g_EglDisplay, egl_attributes, &egl_config, 1, &num_configs);
    EGLint egl_format;
    eglGetConfigAttrib(g_EglDisplay, egl_config, EGL_NATIVE_VISUAL_ID, &egl_format);
    ANativeWindow_setBuffersGeometry(g_App->window, 0, 0, egl_format);

    const EGLint egl_context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    g_EglContext = eglCreateContext(g_EglDisplay, egl_config, EGL_NO_CONTEXT, egl_context_attributes);
    if (g_EglContext == EGL_NO_CONTEXT)
        __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s", "eglCreateContext() returned EGL_NO_CONTEXT");

    g_EglSurface = eglCreateWindowSurface(g_EglDisplay, egl_config, g_App->window, nullptr);
    eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext);

    // --- Dear ImGui setup ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Redirect .ini file to app-internal storage
    g_IniFilename = std::string(app->activity->internalDataPath) + "/imgui.ini";
    io.IniFilename = g_IniFilename.c_str();

    // Dark theme
    ImGui::StyleColorsDark();

    // Initialize backends — the Android backend handles keyboard/clipboard via JNI internally
    // Pass the NativeActivity object so the backend can do JNI calls without any app boilerplate
    ImGui_ImplAndroid_Init(g_App->window, app->activity->assetManager, app->activity->clazz);
    ImGui_ImplOpenGL3Init("#version 300 es");

    // Scale for typical mobile DPI — use actual display density from the backend
    ImGui_ImplAndroid_DisplayMetrics dm;
    ImGui_ImplAndroid_GetDisplayMetrics(&dm);
    float main_scale = dm.Density > 0.0f ? dm.Density : 2.0f;
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    // Enable sensors to showcase device capabilities
    ImGui_ImplAndroid_EnableSensor(ImGui_ImplAndroid_SensorType_Accelerometer);
    ImGui_ImplAndroid_EnableSensor(ImGui_ImplAndroid_SensorType_Gyroscope);
    ImGui_ImplAndroid_EnableSensor(ImGui_ImplAndroid_SensorType_Magnetometer);
    ImGui_ImplAndroid_EnableSensor(ImGui_ImplAndroid_SensorType_Light);
    ImGui_ImplAndroid_EnableSensor(ImGui_ImplAndroid_SensorType_Proximity);

    g_Initialized = true;
}

static void MainLoopStep()
{
    if (g_EglDisplay == EGL_NO_DISPLAY)
        return;

    ImGuiIO& io = ImGui::GetIO();

    // State
    static bool show_demo_window = true;
    static bool show_another_window = false;
    static bool show_sensor_window = true;
    static bool show_display_window = true;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Start a new frame — backend handles Unicode polling + soft keyboard automatically
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window we create ourselves
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, Android!");

        ImGui::Text("This is some useful text.");
        ImGui::Checkbox("Demo Window", &show_demo_window);
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&clear_color);

        if (ImGui::Button("Button"))
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    // 4. Show sensor data window — showcases the device's sensors
    if (show_sensor_window)
    {
        ImGui::Begin("Sensors", &show_sensor_window);

        const char* sensor_names[] = { "Accelerometer", "Gyroscope", "Magnetometer", "Light", "Proximity" };
        for (int i = 0; i < 5; i++)
        {
            ImGui_ImplAndroid_SensorData sd;
            ImGui_ImplAndroid_GetSensorData(i, &sd);
            if (!sd.Available)
            {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s: N/A", sensor_names[i]);
            }
            else
            {
                if (i <= 2) // 3-axis sensors
                {
                    ImGui::Text("%s:", sensor_names[i]);
                    ImGui::SameLine(120);
                    ImGui::Text("X:%.2f Y:%.2f Z:%.2f", sd.Values[0], sd.Values[1], sd.Values[2]);

                    // Visual bar for each axis
                    ImGui::SameLine(280);
                    char label[32];
                    snprintf(label, sizeof(label), "X##%d", i);
                    ImGui::ProgressBar(sd.Values[0] / 20.0f + 0.5f, ImVec2(40, 0), label);
                }
                else // Scalar sensors (light, proximity)
                {
                    ImGui::Text("%s:", sensor_names[i]);
                    ImGui::SameLine(120);
                    ImGui::Text("%.1f", sd.Values[0]);
                }
            }
        }

        ImGui::Separator();
        ImGui::Text("Accuracy: see Android docs for status codes");
        ImGui::End();
    }

    // 5. Show display metrics window
    if (show_display_window)
    {
        ImGui::Begin("Display", &show_display_window);

        ImGui_ImplAndroid_DisplayMetrics dm;
        ImGui_ImplAndroid_GetDisplayMetrics(&dm);

        const char* orient_names[] = { "Portrait", "Landscape", "Reverse Portrait", "Reverse Landscape" };
        int orient = dm.Orientation >= 0 && dm.Orientation <= 3 ? dm.Orientation : 0;

        ImGui::Text("Resolution: %d x %d", dm.WidthPixels, dm.HeightPixels);
        ImGui::Text("Density: %.2f (%d DPI)", dm.Density, dm.DensityDpi);
        ImGui::Text("Physical: %.1f x %.1f DPI", dm.Xdpi, dm.Ydpi);
        ImGui::Text("Refresh: %.1f Hz", dm.RefreshRate);
        ImGui::Text("Orientation: %s", orient_names[orient]);

        ImGui::Separator();
        ImGui::Text("Font scale: %.1fx", ImGui::GetStyle().FontScaleDpi);
        ImGui::End();
    }

    // Render
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(g_EglDisplay, g_EglSurface);
}

static void Shutdown()
{
    if (!g_Initialized)
        return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();

    if (g_EglDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_EglContext != EGL_NO_CONTEXT)
            eglDestroyContext(g_EglDisplay, g_EglContext);
        if (g_EglSurface != EGL_NO_SURFACE)
            eglDestroySurface(g_EglDisplay, g_EglSurface);
        eglTerminate(g_EglDisplay);
    }

    g_EglDisplay = EGL_NO_DISPLAY;
    g_EglContext = EGL_NO_CONTEXT;
    g_EglSurface = EGL_NO_SURFACE;

    if (g_App && g_App->window)
        ANativeWindow_release(g_App->window);

    g_Initialized = false;
}
