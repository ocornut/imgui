// dear imgui: Platform Backend for Android native app
// This needs to be used along with a Renderer Backend (e.g. OpenGL3, Vulkan)

// Implemented features:
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy AKEYCODE_* values are obsolete since 1.87 and not supported since 1.91.5]
//  [X] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen.
//  [X] Platform: On-screen keyboard (soft input) — handled internally via JNI. No application code needed.
//  [X] Platform: Unicode character input — handled internally via JNI. No application code needed.
//  [X] Platform: Clipboard support (via JNI to Android ClipboardManager).
//  [X] Platform: Display metrics (DPI, density, refresh rate, orientation).
//  [X] Platform: Sensor support (accelerometer, gyroscope, magnetometer, light, proximity) via NDK ASensor API.
// Missing features or Issues:
//  [ ] Platform: Gamepad support.
//  [ ] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors). Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'. FIXME: Check if this is even possible with Android.
// Important:
//  - Consider using SDL or GLFW backend on Android, which will be more full-featured than this.
//  - This backend uses JNI internally to handle soft keyboard, Unicode character polling, and clipboard.
//    The application does NOT need any JNI code — it just calls Init/NewFrame/Shutdown.
//  - Sensors use the NDK's native ASensor API (no JNI required for sensor reads).

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

// Sensor types supported by the backend.
enum ImGui_ImplAndroid_SensorType
{
    ImGui_ImplAndroid_SensorType_Accelerometer = 0,
    ImGui_ImplAndroid_SensorType_Gyroscope,
    ImGui_ImplAndroid_SensorType_Magnetometer,
    ImGui_ImplAndroid_SensorType_Light,
    ImGui_ImplAndroid_SensorType_Proximity,
    ImGui_ImplAndroid_SensorType_Pressure,
    ImGui_ImplAndroid_SensorType_Humidity,
    ImGui_ImplAndroid_SensorType_AmbientTemperature,
    ImGui_ImplAndroid_SensorType_Count
};

// Sensor reading returned by ImGui_ImplAndroid_GetSensorData().
struct ImGui_ImplAndroid_SensorData
{
    bool    Available;       // Whether this sensor is present on the device
    float   Values[3];      // X, Y, Z (or scalar for 1D sensors like light/proximity)
    float   Accuracy;       // 0=none, 1=low, 2=medium, 3=high
    double  Timestamp;      // Last event timestamp (seconds, monotonic)
};

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

// Initialize the Android platform backend.
// 'window' is the ANativeWindow obtained from android_app->window.
// 'asset_manager' is optional (pass android_app->activity->assetManager or nullptr).
// 'native_activity' is optional (pass the JNI activity object for keyboard/clipboard/display metrics, or nullptr to skip those features).
IMGUI_IMPL_API bool     ImGui_ImplAndroid_Init(ANativeWindow* window, struct AAssetManager* asset_manager = nullptr, jobject native_activity = nullptr);
IMGUI_IMPL_API int32_t  ImGui_ImplAndroid_HandleInputEvent(const AInputEvent* input_event);
IMGUI_IMPL_API void     ImGui_ImplAndroid_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplAndroid_NewFrame();

// Optional: explicitly show/hide the soft keyboard (normally handled automatically in NewFrame based on io.WantTextInput)
IMGUI_IMPL_API void     ImGui_ImplAndroid_ShowSoftKeyboard();
IMGUI_IMPL_API void     ImGui_ImplAndroid_HideSoftKeyboard();

// Sensor API — enable sensors in Init() via flags, then read in your render loop.
// Enable one or more sensors (call after Init). Returns true if the sensor was successfully enabled.
IMGUI_IMPL_API bool     ImGui_ImplAndroid_EnableSensor(int sensor_type);
IMGUI_IMPL_API void     ImGui_ImplAndroid_DisableSensor(int sensor_type);
IMGUI_IMPL_API void     ImGui_ImplAndroid_GetSensorData(int sensor_type, ImGui_ImplAndroid_SensorData* out_data);
IMGUI_IMPL_API bool     ImGui_ImplAndroid_IsSensorAvailable(int sensor_type);

// Display metrics — queried once during Init and refreshed in NewFrame if the window resizes.
IMGUI_IMPL_API void     ImGui_ImplAndroid_GetDisplayMetrics(ImGui_ImplAndroid_DisplayMetrics* out_metrics);

#endif // #ifndef IMGUI_DISABLE
