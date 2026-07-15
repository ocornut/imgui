// dear imgui: Platform Backend for Android native app
// This needs to be used along with a Renderer Backend (e.g. OpenGL3, Vulkan)

// Implemented features:
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy AKEYCODE_* values are obsolete since 1.87 and not supported since 1.91.5]
//  [X] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen.
//  [X] Platform: On-screen keyboard (soft input) — handled internally via JNI. No application code needed. (#3446)
//  [X] Platform: Unicode character input — handled internally via JNI. No application code needed. (#3446)
//  [X] Platform: Clipboard support (via JNI to Android ClipboardManager). (#7259)
// Missing features or Issues:
//  [ ] Platform: Gamepad support.
//  [ ] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors). Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'. FIXME: Check if this is even possible with Android.
// Important:
//  - Consider using SDL or GLFW backend on Android, which will be more full-featured than this.
//  - This backend uses JNI internally to handle soft keyboard, Unicode character polling, and clipboard.
//    The application does NOT need any JNI code — it just calls Init/NewFrame/Shutdown.

// BREAKING CHANGE: ImGui_ImplAndroid_Init() signature changed (#3446)
//
// Before (v1.92 and earlier):
//   ImGui_ImplAndroid_Init(g_App->window);
//   // Application had to call ShowSoftKeyboardInput() and PollUnicodeChars() every frame.
//   // Application had to implement JNI boilerplate for clipboard.
//
// After:
//   ImGui_ImplAndroid_Init(g_App->window, app->activity->assetManager, app->activity->clazz);
//   // The backend handles soft keyboard, Unicode input, and clipboard internally.
//   // Pass nullptr for asset_manager and/or native_activity to disable those features.
//
// To keep old behavior (no JNI features):
//   ImGui_ImplAndroid_Init(g_App->window, nullptr, nullptr);

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

struct ANativeWindow;
struct AInputEvent;
struct AAssetManager;

// Forward declaration so users don't need JNI headers in their own includes.
// The actual type is 'jobject' from <jni.h>. If you pass a real jobject, include <jni.h> before this header.
#if __ANDROID__
#  include <jni.h>
#else
typedef void* jobject;
#endif

// Display metrics returned by ImGui_ImplAndroid_GetDisplayMetrics().
struct ImGui_ImplAndroid_DisplayMetrics
{
    int     WidthPixels;
    int     HeightPixels;
    float   Density;         // Logical density (1.0 = mdpi, 2.0 = xhdpi, 3.0 = xxhdpi)
    int     DensityDpi;      // Physical dots per inch
    float   Xdpi;            // Exact physical pixels per inch X
    float   Ydpi;            // Exact physical pixels per inch Y
    int     Orientation;     // 0=portrait, 1=landscape, 2=reverse portrait, 3=reverse landscape
    float   RefreshRate;     // Screen refresh rate in Hz
};

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool     ImGui_ImplAndroid_Init(ANativeWindow* window, struct AAssetManager* asset_manager = nullptr, jobject native_activity = nullptr);
IMGUI_IMPL_API int32_t  ImGui_ImplAndroid_HandleInputEvent(const AInputEvent* input_event);
IMGUI_IMPL_API void     ImGui_ImplAndroid_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplAndroid_NewFrame();

// Optional: explicitly show/hide the soft keyboard (normally handled automatically in NewFrame based on io.WantTextInput)
IMGUI_IMPL_API void     ImGui_ImplAndroid_ShowSoftKeyboard();
IMGUI_IMPL_API void     ImGui_ImplAndroid_HideSoftKeyboard();

// Display metrics — queried once during Init and refreshed in NewFrame if the window resizes.
IMGUI_IMPL_API void     ImGui_ImplAndroid_GetDisplayMetrics(ImGui_ImplAndroid_DisplayMetrics* out_metrics);

#endif // #ifndef IMGUI_DISABLE
