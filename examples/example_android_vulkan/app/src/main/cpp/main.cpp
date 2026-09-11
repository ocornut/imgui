// ═════════════════════════════════════════════════════════════════════════════
//  main.cpp — Android NativeActivity entry point
//
//  Lifecycle: android_main → Init (on INIT_WINDOW) → MainLoopStep → Shutdown
//  Vulkan setup is in vulkan_helper.h, UI drawing is in menu.h.
// ═════════════════════════════════════════════════════════════════════════════

#include "vulkan_helper.h"
#include "menu.h"
#include "imgui_impl_android.h"
#include <jni.h>
#include <string>

static struct android_app* g_App         = nullptr;
static bool                g_Initialized = false;
static bool                g_SoftKeyboardVisible = false;
static std::string         g_ClipboardText;

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────────────────────
static void Init(struct android_app* app);
static void Shutdown();
static void MainLoopStep();

// ─────────────────────────────────────────────────────────────────────────────
// Keyboard JNI Helpers
// ─────────────────────────────────────────────────────────────────────────────
static int CallJniVoidMethod(const char* methodName)
{
    JavaVM* java_vm = g_App->activity->vm;
    JNIEnv* java_env = nullptr;

    if (java_vm->GetEnv((void**)&java_env, JNI_VERSION_1_6) == JNI_ERR) return -1;
    if (java_vm->AttachCurrentThread(&java_env, nullptr) != JNI_OK) return -2;

    jclass clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (!clazz) return -3;

    jmethodID method_id = java_env->GetMethodID(clazz, methodName, "()V");
    if (!method_id) return -4;

    java_env->CallVoidMethod(g_App->activity->clazz, method_id);
    if (java_vm->DetachCurrentThread() != JNI_OK) return -5;

    return 0;
}

static void ShowSoftKeyboardInput() { CallJniVoidMethod("showSoftInput"); }
static void HideSoftKeyboardInput() { CallJniVoidMethod("hideSoftInput"); }

static int PollUnicodeChars()
{
    JavaVM* java_vm = g_App->activity->vm;
    JNIEnv* java_env = nullptr;

    if (java_vm->GetEnv((void**)&java_env, JNI_VERSION_1_6) == JNI_ERR) return -1;
    if (java_vm->AttachCurrentThread(&java_env, nullptr) != JNI_OK) return -2;

    jclass clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (!clazz) return -3;

    jmethodID method_id = java_env->GetMethodID(clazz, "pollUnicodeChar", "()I");
    if (!method_id) return -4;

    ImGuiIO& io = ImGui::GetIO();
    jint unicode_character;
    while ((unicode_character = java_env->CallIntMethod(g_App->activity->clazz, method_id)) != 0)
        io.AddInputCharacter(unicode_character);

    if (java_vm->DetachCurrentThread() != JNI_OK) return -5;
    return 0;
}

static const char* GetClipboardTextFn(void* user_data)
{
    JavaVM* java_vm = g_App->activity->vm;
    JNIEnv* java_env = nullptr;

    if (java_vm->GetEnv((void**)&java_env, JNI_VERSION_1_6) == JNI_ERR) return nullptr;
    if (java_vm->AttachCurrentThread(&java_env, nullptr) != JNI_OK) return nullptr;

    jclass clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (!clazz) return nullptr;

    jmethodID method_id = java_env->GetMethodID(clazz, "getClipboardText", "()Ljava/lang/String;");
    if (!method_id) return nullptr;

    jstring jstr = (jstring)java_env->CallObjectMethod(g_App->activity->clazz, method_id);
    if (jstr)
    {
        const char* str = java_env->GetStringUTFChars(jstr, nullptr);
        g_ClipboardText = str;
        java_env->ReleaseStringUTFChars(jstr, str);
        java_env->DeleteLocalRef(jstr);
    }
    else
    {
        g_ClipboardText = "";
    }

    if (java_vm->DetachCurrentThread() != JNI_OK) return nullptr;
    return g_ClipboardText.c_str();
}

static void SetClipboardTextFn(void* user_data, const char* text)
{
    JavaVM* java_vm = g_App->activity->vm;
    JNIEnv* java_env = nullptr;

    if (java_vm->GetEnv((void**)&java_env, JNI_VERSION_1_6) == JNI_ERR) return;
    if (java_vm->AttachCurrentThread(&java_env, nullptr) != JNI_OK) return;

    jclass clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (!clazz) return;

    jmethodID method_id = java_env->GetMethodID(clazz, "setClipboardText", "(Ljava/lang/String;)V");
    if (!method_id) return;

    jstring jstr = java_env->NewStringUTF(text);
    java_env->CallVoidMethod(g_App->activity->clazz, method_id, jstr);
    java_env->DeleteLocalRef(jstr);

    if (java_vm->DetachCurrentThread() != JNI_OK) return;
}

// ─────────────────────────────────────────────────────────────────────────────
// Android callbacks
// ─────────────────────────────────────────────────────────────────────────────
static void handleAppCmd(struct android_app* app, int32_t cmd)
{
    switch (cmd)
    {
    case APP_CMD_INIT_WINDOW:    Init(app);                      break;
    case APP_CMD_TERM_WINDOW:    Shutdown();                     break;
    case APP_CMD_CONFIG_CHANGED:
    case APP_CMD_WINDOW_RESIZED: vkh::g_SwapChainRebuild = true; break;
    default: break;
    }
}

static int32_t handleInputEvent(struct android_app*, AInputEvent* event)
{
    return ImGui_ImplAndroid_HandleInputEvent(event);
}

// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────
void android_main(struct android_app* app)
{
    app->onAppCmd    = handleAppCmd;
    app->onInputEvent = handleInputEvent;

    while (true)
    {
        int events;
        struct android_poll_source* source;
        while (ALooper_pollOnce(g_Initialized ? 0 : -1, nullptr, &events, (void**)&source) >= 0)
        {
            if (source) source->process(app, source);
            if (app->destroyRequested) { if (g_Initialized) Shutdown(); return; }
        }
        MainLoopStep();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Init
// ─────────────────────────────────────────────────────────────────────────────
void Init(struct android_app* app)
{
    if (g_Initialized) return;
    g_App = app;
    ANativeWindow_acquire(app->window);

    // Vulkan
    vkh::SetupVulkan();

    VkAndroidSurfaceCreateInfoKHR sci = { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR };
    sci.window = app->window;
    VkSurfaceKHR surface;
    vkCreateAndroidSurfaceKHR(vkh::g_Instance, &sci, nullptr, &surface);

    int w = ANativeWindow_getWidth(app->window);
    int h = ANativeWindow_getHeight(app->window);

    auto* wd = &vkh::g_MainWindowData;
    vkh::SetupWindow(wd, surface, w, h);

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.SetClipboardTextFn = SetClipboardTextFn;
    io.GetClipboardTextFn = GetClipboardTextFn;

    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.ScaleAllSizes(2.0f);
    st.FontScaleDpi    = 2.0f;
    st.WindowRounding  = 8.0f;
    st.FrameRounding   = 4.0f;
    st.GrabRounding    = 4.0f;

    ImGui_ImplAndroid_Init(app->window);

    ImGui_ImplVulkan_InitInfo ii = {};
    ii.Instance       = vkh::g_Instance;
    ii.PhysicalDevice = vkh::g_PhysicalDevice;
    ii.Device         = vkh::g_Device;
    ii.QueueFamily    = vkh::g_QueueFamily;
    ii.Queue          = vkh::g_Queue;
    ii.PipelineCache  = vkh::g_PipelineCache;
    ii.DescriptorPool = vkh::g_DescriptorPool;
    ii.MinImageCount  = vkh::g_MinImageCount;
    ii.ImageCount     = wd->ImageCount;
    ii.CheckVkResultFn = vkh::CheckVkResult;
    ii.PipelineInfoMain.RenderPass  = wd->RenderPass;
    ii.PipelineInfoMain.Subpass     = 0;
    ii.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&ii);

    g_Initialized = true;
    LOGI("[init] Ready! %dx%d", w, h);
}

// ─────────────────────────────────────────────────────────────────────────────
// MainLoopStep
// ─────────────────────────────────────────────────────────────────────────────
void MainLoopStep()
{
    if (!g_Initialized) return;
    auto* wd = &vkh::g_MainWindowData;

    vkh::RebuildIfNeeded(wd, g_App->window);

    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    // Poll unicode characters from Java
    PollUnicodeChars();

    // Show/Hide soft keyboard based on ImGui focus
    if (io.WantTextInput && !g_SoftKeyboardVisible)
    {
        ShowSoftKeyboardInput();
        g_SoftKeyboardVisible = true;
    }
    else if (!io.WantTextInput && g_SoftKeyboardVisible)
    {
        HideSoftKeyboardInput();
        g_SoftKeyboardVisible = false;
    }

    menu::Draw();

    ImGui::Render();
    ImDrawData* dd = ImGui::GetDrawData();
    if (dd->DisplaySize.x > 0 && dd->DisplaySize.y > 0)
    {
        auto& cc = menu::clearColor;
        wd->ClearValue.color.float32[0] = cc.x * cc.w;
        wd->ClearValue.color.float32[1] = cc.y * cc.w;
        wd->ClearValue.color.float32[2] = cc.z * cc.w;
        wd->ClearValue.color.float32[3] = cc.w;
        vkh::FrameRender(wd, dd);
        vkh::FramePresent(wd);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Shutdown
// ─────────────────────────────────────────────────────────────────────────────
void Shutdown()
{
    if (!g_Initialized) return;
    vkDeviceWaitIdle(vkh::g_Device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();

    vkh::CleanupWindow(&vkh::g_MainWindowData);
    vkh::CleanupVulkan();

    ANativeWindow_release(g_App->window);
    g_Initialized = false;
    LOGI("[shutdown] Done");
}
