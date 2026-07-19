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

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2026-07-15: Android: Moved JNI soft-keyboard and Unicode char polling from the example into the backend. (#3446)
//                Added clipboard support via JNI. (#7259)
//                Added display metrics (DPI, density, refresh rate, orientation) via JNI.
//                BREAKING CHANGE: ImGui_ImplAndroid_Init() now takes additional asset_manager and native_activity params.
//  2022-09-26: Inputs: Renamed ImGuiKey_ModXXX introduced in 1.87 to ImGuiMod_XXX (old names still supported).
//  2022-01-26: Inputs: replaced short-lived io.AddKeyModsEvent() (added two weeks ago) with io.AddKeyEvent() using ImGuiKey_ModXXX flags. Sorry for the confusion.
//  2022-01-17: Inputs: calling new io.AddMousePosEvent(), io.AddMouseButtonEvent(), io.AddMouseWheelEvent() API (1.87+).
//  2022-01-10: Inputs: calling new io.AddKeyEvent(), io.AddKeyModsEvent() + io.SetKeyEventNativeData() API (1.87+). Support for full ImGuiKey range.
//  2021-03-04: Initial version.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_android.h"
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <android/native_window.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android/log.h>
#include <jni.h>

// Android data
static double                                   g_Time = 0.0;
static ANativeWindow*                           g_Window = nullptr;
static AAssetManager*                           g_AssetManager = nullptr;
static char                                     g_LogTag[] = "ImGuiBackend";

// JNI state (for soft keyboard, Unicode polling, clipboard, display metrics)
static JavaVM*                                  g_JavaVM = nullptr;
static jobject                                  g_NativeActivity = nullptr;  // Global ref
static bool                                     g_HasJni = false;
static bool                                     g_JniEnabled = false;  // Opt-in: default false, app retains control

// Clipboard state
static char*                                    g_ClipboardText = nullptr;

// Display metrics
static ImGui_ImplAndroid_DisplayMetrics         g_DisplayMetrics = {};

// Keyboard customization
static int                                      g_KeyboardInputType = 0;   // 0 = default (text)
static int                                      g_KeyboardImeAction = 0;   // 0 = default (done)

// Long press state
static void (*g_LongPressCallback)(float x, float y, void* user_data) = nullptr;
static void* g_LongPressUserData = nullptr;
static float g_LongPressDuration = 0.5f;
static float g_LongPressStartTime = 0.0f;
static float g_LongPressX = 0.0f, g_LongPressY = 0.0f;
static bool g_LongPressActive = false;

// Pressure sensitivity state (per-pointer, indexed by pointer id)
#define IMGUI_ANDROID_MAX_POINTERS 10
static float g_TouchPressure[IMGUI_ANDROID_MAX_POINTERS] = {};
static float g_TouchPressureMin = 0.3f; // Minimum pressure to register intentional touch
static bool g_PressureEnabled = true;

// Forward declarations of JNI helpers
static void ImGui_ImplAndroid_JniShowSoftKeyboard();
static void ImGui_ImplAndroid_JniHideSoftKeyboard();
static void ImGui_ImplAndroid_JniPollUnicodeChars();
static void ImGui_ImplAndroid_JniSetClipboardText(const char* text);
static const char* ImGui_ImplAndroid_JniGetClipboardText();
static void ImGui_ImplAndroid_JniRefreshDisplayMetrics();

static ImGuiKey ImGui_ImplAndroid_KeyCodeToImGuiKey(int32_t key_code)
{
    switch (key_code)
    {
        case AKEYCODE_TAB:                  return ImGuiKey_Tab;
        case AKEYCODE_DPAD_LEFT:            return ImGuiKey_LeftArrow;
        case AKEYCODE_DPAD_RIGHT:           return ImGuiKey_RightArrow;
        case AKEYCODE_DPAD_UP:              return ImGuiKey_UpArrow;
        case AKEYCODE_DPAD_DOWN:            return ImGuiKey_DownArrow;
        case AKEYCODE_PAGE_UP:              return ImGuiKey_PageUp;
        case AKEYCODE_PAGE_DOWN:            return ImGuiKey_PageDown;
        case AKEYCODE_MOVE_HOME:            return ImGuiKey_Home;
        case AKEYCODE_MOVE_END:             return ImGuiKey_End;
        case AKEYCODE_INSERT:               return ImGuiKey_Insert;
        case AKEYCODE_FORWARD_DEL:          return ImGuiKey_Delete;
        case AKEYCODE_DEL:                  return ImGuiKey_Backspace;
        case AKEYCODE_SPACE:                return ImGuiKey_Space;
        case AKEYCODE_ENTER:                return ImGuiKey_Enter;
        case AKEYCODE_ESCAPE:               return ImGuiKey_Escape;
        case AKEYCODE_APOSTROPHE:           return ImGuiKey_Apostrophe;
        case AKEYCODE_COMMA:                return ImGuiKey_Comma;
        case AKEYCODE_MINUS:                return ImGuiKey_Minus;
        case AKEYCODE_PERIOD:               return ImGuiKey_Period;
        case AKEYCODE_SLASH:                return ImGuiKey_Slash;
        case AKEYCODE_SEMICOLON:            return ImGuiKey_Semicolon;
        case AKEYCODE_EQUALS:               return ImGuiKey_Equal;
        case AKEYCODE_LEFT_BRACKET:         return ImGuiKey_LeftBracket;
        case AKEYCODE_BACKSLASH:            return ImGuiKey_Backslash;
        case AKEYCODE_RIGHT_BRACKET:        return ImGuiKey_RightBracket;
        case AKEYCODE_GRAVE:                return ImGuiKey_GraveAccent;
        case AKEYCODE_CAPS_LOCK:            return ImGuiKey_CapsLock;
        case AKEYCODE_SCROLL_LOCK:          return ImGuiKey_ScrollLock;
        case AKEYCODE_NUM_LOCK:             return ImGuiKey_NumLock;
        case AKEYCODE_SYSRQ:                return ImGuiKey_PrintScreen;
        case AKEYCODE_BREAK:                return ImGuiKey_Pause;
        case AKEYCODE_NUMPAD_0:             return ImGuiKey_Keypad0;
        case AKEYCODE_NUMPAD_1:             return ImGuiKey_Keypad1;
        case AKEYCODE_NUMPAD_2:             return ImGuiKey_Keypad2;
        case AKEYCODE_NUMPAD_3:             return ImGuiKey_Keypad3;
        case AKEYCODE_NUMPAD_4:             return ImGuiKey_Keypad4;
        case AKEYCODE_NUMPAD_5:             return ImGuiKey_Keypad5;
        case AKEYCODE_NUMPAD_6:             return ImGuiKey_Keypad6;
        case AKEYCODE_NUMPAD_7:             return ImGuiKey_Keypad7;
        case AKEYCODE_NUMPAD_8:             return ImGuiKey_Keypad8;
        case AKEYCODE_NUMPAD_9:             return ImGuiKey_Keypad9;
        case AKEYCODE_NUMPAD_DOT:           return ImGuiKey_KeypadDecimal;
        case AKEYCODE_NUMPAD_DIVIDE:        return ImGuiKey_KeypadDivide;
        case AKEYCODE_NUMPAD_MULTIPLY:      return ImGuiKey_KeypadMultiply;
        case AKEYCODE_NUMPAD_SUBTRACT:      return ImGuiKey_KeypadSubtract;
        case AKEYCODE_NUMPAD_ADD:           return ImGuiKey_KeypadAdd;
        case AKEYCODE_NUMPAD_ENTER:         return ImGuiKey_KeypadEnter;
        case AKEYCODE_NUMPAD_EQUALS:        return ImGuiKey_KeypadEqual;
        case AKEYCODE_CTRL_LEFT:            return ImGuiKey_LeftCtrl;
        case AKEYCODE_SHIFT_LEFT:           return ImGuiKey_LeftShift;
        case AKEYCODE_ALT_LEFT:             return ImGuiKey_LeftAlt;
        case AKEYCODE_META_LEFT:            return ImGuiKey_LeftSuper;
        case AKEYCODE_CTRL_RIGHT:           return ImGuiKey_RightCtrl;
        case AKEYCODE_SHIFT_RIGHT:          return ImGuiKey_RightShift;
        case AKEYCODE_ALT_RIGHT:            return ImGuiKey_RightAlt;
        case AKEYCODE_META_RIGHT:           return ImGuiKey_RightSuper;
        case AKEYCODE_MENU:                 return ImGuiKey_Menu;
        case AKEYCODE_0:                    return ImGuiKey_0;
        case AKEYCODE_1:                    return ImGuiKey_1;
        case AKEYCODE_2:                    return ImGuiKey_2;
        case AKEYCODE_3:                    return ImGuiKey_3;
        case AKEYCODE_4:                    return ImGuiKey_4;
        case AKEYCODE_5:                    return ImGuiKey_5;
        case AKEYCODE_6:                    return ImGuiKey_6;
        case AKEYCODE_7:                    return ImGuiKey_7;
        case AKEYCODE_8:                    return ImGuiKey_8;
        case AKEYCODE_9:                    return ImGuiKey_9;
        case AKEYCODE_A:                    return ImGuiKey_A;
        case AKEYCODE_B:                    return ImGuiKey_B;
        case AKEYCODE_C:                    return ImGuiKey_C;
        case AKEYCODE_D:                    return ImGuiKey_D;
        case AKEYCODE_E:                    return ImGuiKey_E;
        case AKEYCODE_F:                    return ImGuiKey_F;
        case AKEYCODE_G:                    return ImGuiKey_G;
        case AKEYCODE_H:                    return ImGuiKey_H;
        case AKEYCODE_I:                    return ImGuiKey_I;
        case AKEYCODE_J:                    return ImGuiKey_J;
        case AKEYCODE_K:                    return ImGuiKey_K;
        case AKEYCODE_L:                    return ImGuiKey_L;
        case AKEYCODE_M:                    return ImGuiKey_M;
        case AKEYCODE_N:                    return ImGuiKey_N;
        case AKEYCODE_O:                    return ImGuiKey_O;
        case AKEYCODE_P:                    return ImGuiKey_P;
        case AKEYCODE_Q:                    return ImGuiKey_Q;
        case AKEYCODE_R:                    return ImGuiKey_R;
        case AKEYCODE_S:                    return ImGuiKey_S;
        case AKEYCODE_T:                    return ImGuiKey_T;
        case AKEYCODE_U:                    return ImGuiKey_U;
        case AKEYCODE_V:                    return ImGuiKey_V;
        case AKEYCODE_W:                    return ImGuiKey_W;
        case AKEYCODE_X:                    return ImGuiKey_X;
        case AKEYCODE_Y:                    return ImGuiKey_Y;
        case AKEYCODE_Z:                    return ImGuiKey_Z;
        case AKEYCODE_F1:                   return ImGuiKey_F1;
        case AKEYCODE_F2:                   return ImGuiKey_F2;
        case AKEYCODE_F3:                   return ImGuiKey_F3;
        case AKEYCODE_F4:                   return ImGuiKey_F4;
        case AKEYCODE_F5:                   return ImGuiKey_F5;
        case AKEYCODE_F6:                   return ImGuiKey_F6;
        case AKEYCODE_F7:                   return ImGuiKey_F7;
        case AKEYCODE_F8:                   return ImGuiKey_F8;
        case AKEYCODE_F9:                   return ImGuiKey_F9;
        case AKEYCODE_F10:                  return ImGuiKey_F10;
        case AKEYCODE_F11:                  return ImGuiKey_F11;
        case AKEYCODE_F12:                  return ImGuiKey_F12;
        default:                            return ImGuiKey_None;
    }
}

int32_t ImGui_ImplAndroid_HandleInputEvent(const AInputEvent* input_event)
{
    ImGuiIO& io = ImGui::GetIO();
    int32_t event_type = AInputEvent_getType(input_event);
    switch (event_type)
    {
    case AINPUT_EVENT_TYPE_KEY:
    {
        int32_t event_key_code = AKeyEvent_getKeyCode(input_event);
        int32_t event_scan_code = AKeyEvent_getScanCode(input_event);
        int32_t event_action = AKeyEvent_getAction(input_event);
        int32_t event_meta_state = AKeyEvent_getMetaState(input_event);

        // Back button: hide soft keyboard when visible, consume the event
        if (event_key_code == AKEYCODE_BACK && event_action == AKEY_EVENT_ACTION_UP)
        {
            if (g_JniEnabled && g_HasJni)
            {
                ImGui_ImplAndroid_JniHideSoftKeyboard();
                return 1; // Consume the event
            }
        }

        io.AddKeyEvent(ImGuiMod_Ctrl,  (event_meta_state & AMETA_CTRL_ON)  != 0);
        io.AddKeyEvent(ImGuiMod_Shift, (event_meta_state & AMETA_SHIFT_ON) != 0);
        io.AddKeyEvent(ImGuiMod_Alt,   (event_meta_state & AMETA_ALT_ON)   != 0);
        io.AddKeyEvent(ImGuiMod_Super, (event_meta_state & AMETA_META_ON)  != 0);

        switch (event_action)
        {
        // FIXME: AKEY_EVENT_ACTION_DOWN and AKEY_EVENT_ACTION_UP occur at once as soon as a touch pointer
        // goes up from a key. We use a simple key event queue/ and process one event per key per frame in
        // ImGui_ImplAndroid_NewFrame()...or consider using IO queue, if suitable: https://github.com/ocornut/imgui/issues/2787
        case AKEY_EVENT_ACTION_DOWN:
        case AKEY_EVENT_ACTION_UP:
        {
            ImGuiKey key = ImGui_ImplAndroid_KeyCodeToImGuiKey(event_key_code);
            if (key != ImGuiKey_None)
            {
                io.AddKeyEvent(key, event_action == AKEY_EVENT_ACTION_DOWN);
                io.SetKeyEventNativeData(key, event_key_code, event_scan_code);
            }

            break;
        }
        default:
            break;
        }
        break;
    }
    case AINPUT_EVENT_TYPE_MOTION:
    {
        int32_t event_action = AMotionEvent_getAction(input_event);
        int32_t event_pointer_index = (event_action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        event_action &= AMOTION_EVENT_ACTION_MASK;

        switch (AMotionEvent_getToolType(input_event, event_pointer_index))
        {
        case AMOTION_EVENT_TOOL_TYPE_MOUSE:
            io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
            break;
        case AMOTION_EVENT_TOOL_TYPE_STYLUS:
        case AMOTION_EVENT_TOOL_TYPE_ERASER:
            io.AddMouseSourceEvent(ImGuiMouseSource_Pen);
            break;
        case AMOTION_EVENT_TOOL_TYPE_FINGER:
        default:
            io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
            break;
        }

        switch (event_action)
        {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_UP:
        {
            // Physical mouse buttons (and probably other physical devices) also invoke the actions AMOTION_EVENT_ACTION_DOWN/_UP,
            // but we have to process them separately to identify the actual button pressed. This is done below via
            // AMOTION_EVENT_ACTION_BUTTON_PRESS/_RELEASE. Here, we only process "FINGER" input (and "UNKNOWN", as a fallback).
            int tool_type = AMotionEvent_getToolType(input_event, event_pointer_index);
            if (tool_type == AMOTION_EVENT_TOOL_TYPE_FINGER || tool_type == AMOTION_EVENT_TOOL_TYPE_UNKNOWN)
            {
                // Track pressure for this pointer
                int pointer_id = AMotionEvent_getPointerId(input_event, event_pointer_index);
                if (pointer_id >= 0 && pointer_id < IMGUI_ANDROID_MAX_POINTERS)
                {
                    g_TouchPressure[pointer_id] = AMotionEvent_getPressure(input_event, event_pointer_index);
                }

                // Start long press timer on DOWN (only if pressure is sufficient)
                if (event_action == AMOTION_EVENT_ACTION_DOWN)
                {
                    float pressure = AMotionEvent_getPressure(input_event, event_pointer_index);
                    if (!g_PressureEnabled || pressure >= g_TouchPressureMin)
                    {
                        g_LongPressActive = true;
                        g_LongPressStartTime = (float)(AMotionEvent_getEventTime(input_event) / 1000000000.0);
                        g_LongPressX = AMotionEvent_getX(input_event, event_pointer_index);
                        g_LongPressY = AMotionEvent_getY(input_event, event_pointer_index);
                    }
                }
                else
                {
                    g_LongPressActive = false;
                    if (pointer_id >= 0 && pointer_id < IMGUI_ANDROID_MAX_POINTERS)
                        g_TouchPressure[pointer_id] = 0.0f;
                }

                io.AddMousePosEvent(AMotionEvent_getX(input_event, event_pointer_index), AMotionEvent_getY(input_event, event_pointer_index));
                io.AddMouseButtonEvent(0, event_action == AMOTION_EVENT_ACTION_DOWN);
            }
            break;
        }
        case AMOTION_EVENT_ACTION_BUTTON_PRESS:
        case AMOTION_EVENT_ACTION_BUTTON_RELEASE:
        {
            int32_t button_state = AMotionEvent_getButtonState(input_event);
            io.AddMouseButtonEvent(0, (button_state & AMOTION_EVENT_BUTTON_PRIMARY) != 0);
            io.AddMouseButtonEvent(1, (button_state & AMOTION_EVENT_BUTTON_SECONDARY) != 0);
            io.AddMouseButtonEvent(2, (button_state & AMOTION_EVENT_BUTTON_TERTIARY) != 0);
            break;
        }
        case AMOTION_EVENT_ACTION_HOVER_MOVE: // Hovering: Tool moves while NOT pressed (such as a physical mouse)
        case AMOTION_EVENT_ACTION_MOVE:       // Touch pointer moves while DOWN
        {
            // Update pressure for all active pointers
            for (int32_t i = 0; i < AMotionEvent_getPointerCount(input_event); i++)
            {
                int pid = AMotionEvent_getPointerId(input_event, i);
                if (pid >= 0 && pid < IMGUI_ANDROID_MAX_POINTERS)
                    g_TouchPressure[pid] = AMotionEvent_getPressure(input_event, i);
            }

            // Cancel long press if moved too far or pressure dropped
            if (g_LongPressActive)
            {
                float dx = AMotionEvent_getX(input_event, event_pointer_index) - g_LongPressX;
                float dy = AMotionEvent_getY(input_event, event_pointer_index) - g_LongPressY;
                float pressure = AMotionEvent_getPressure(input_event, event_pointer_index);
                bool pressure_ok = !g_PressureEnabled || pressure >= g_TouchPressureMin;
                if (dx * dx + dy * dy > 100.0f || !pressure_ok) // ~10px tolerance
                    g_LongPressActive = false;
            }

            io.AddMousePosEvent(AMotionEvent_getX(input_event, event_pointer_index), AMotionEvent_getY(input_event, event_pointer_index));
            break;
        }
        case AMOTION_EVENT_ACTION_SCROLL:
        {
            float h_scroll = AMotionEvent_getAxisValue(input_event, AMOTION_EVENT_AXIS_HSCROLL, event_pointer_index);
            float v_scroll = AMotionEvent_getAxisValue(input_event, AMOTION_EVENT_AXIS_VSCROLL, event_pointer_index);

            // Apply pressure weighting to scroll (firmer press = faster scroll)
            if (g_PressureEnabled)
            {
                int pointer_id = AMotionEvent_getPointerId(input_event, event_pointer_index);
                if (pointer_id >= 0 && pointer_id < IMGUI_ANDROID_MAX_POINTERS)
                {
                    float pressure = g_TouchPressure[pointer_id];
                    // Scale scroll by pressure: 0.5x at min pressure, 2.0x at max pressure
                    float scale = 0.5f + pressure * 1.5f;
                    h_scroll *= scale;
                    v_scroll *= scale;
                }
            }

            io.AddMouseWheelEvent(h_scroll, v_scroll);
            break;
        }
        default:
            break;
        }
    }
        return 1;
    default:
        break;
    }

    return 0;
}

// --- JNI helpers ---
// These functions handle the soft keyboard, Unicode character polling, and clipboard
// entirely within the backend so the application code stays clean.

static JNIEnv* ImGui_ImplAndroid_GetEnv()
{
    if (!g_JavaVM)
        return nullptr;
    JNIEnv* env = nullptr;
    jint ret = g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (ret == JNI_EDETACHED)
    {
        if (g_JavaVM->AttachCurrentThread(&env, nullptr) != JNI_OK)
            return nullptr;
    }
    else if (ret != JNI_OK)
    {
        return nullptr;
    }
    return env;
}

static void ImGui_ImplAndroid_DetachEnv()
{
    if (g_JavaVM)
        g_JavaVM->DetachCurrentThread();
}

static void ImGui_ImplAndroid_JniShowSoftKeyboard()
{
    if (!g_NativeActivity)
        return;
    JNIEnv* env = ImGui_ImplAndroid_GetEnv();
    if (!env) return;

    jclass cls = env->GetObjectClass(g_NativeActivity);
    if (!cls) { ImGui_ImplAndroid_DetachEnv(); return; }

    // Try the NativeActivity showSoftInput method via InputMethodManager
    jmethodID method = env->GetMethodID(cls, "showSoftInput", "()V");
    if (method)
        env->CallVoidMethod(g_NativeActivity, method);
    env->DeleteLocalRef(cls);
    ImGui_ImplAndroid_DetachEnv();
}

static void ImGui_ImplAndroid_JniHideSoftKeyboard()
{
    if (!g_NativeActivity)
        return;
    JNIEnv* env = ImGui_ImplAndroid_GetEnv();
    if (!env) return;

    jclass cls = env->GetObjectClass(g_NativeActivity);
    if (!cls) { ImGui_ImplAndroid_DetachEnv(); return; }

    jmethodID method = env->GetMethodID(cls, "hideSoftInput", "()V");
    if (method)
        env->CallVoidMethod(g_NativeActivity, method);
    env->DeleteLocalRef(cls);
    ImGui_ImplAndroid_DetachEnv();
}

static void ImGui_ImplAndroid_JniPollUnicodeChars()
{
    if (!g_NativeActivity)
        return;
    JNIEnv* env = ImGui_ImplAndroid_GetEnv();
    if (!env) return;

    jclass cls = env->GetObjectClass(g_NativeActivity);
    if (!cls) { ImGui_ImplAndroid_DetachEnv(); return; }

    jmethodID method = env->GetMethodID(cls, "pollUnicodeChar", "()I");
    if (!method) { env->DeleteLocalRef(cls); ImGui_ImplAndroid_DetachEnv(); return; }

    ImGuiIO& io = ImGui::GetIO();
    jint unicode_char;
    while ((unicode_char = env->CallIntMethod(g_NativeActivity, method)) != 0)
        io.AddInputCharacter(unicode_char);

    env->DeleteLocalRef(cls);
    ImGui_ImplAndroid_DetachEnv();
}

static void ImGui_ImplAndroid_JniSetClipboardText(const char* text)
{
    if (!g_NativeActivity || !text)
        return;
    JNIEnv* env = ImGui_ImplAndroid_GetEnv();
    if (!env) return;

    // Get ClipboardManager and set text
    jclass activity_cls = env->GetObjectClass(g_NativeActivity);
    if (!activity_cls) { ImGui_ImplAndroid_DetachEnv(); return; }

    jmethodID get_service = env->GetMethodID(activity_cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    if (!get_service) { env->DeleteLocalRef(activity_cls); ImGui_ImplAndroid_DetachEnv(); return; }

    jstring service_name = env->NewStringUTF("clipboard");
    jobject clipboard = env->CallObjectMethod(g_NativeActivity, get_service, service_name);
    env->DeleteLocalRef(service_name);

    if (clipboard)
    {
        jclass clipboard_cls = env->GetObjectClass(clipboard);
        jmethodID set_text = env->GetMethodID(clipboard_cls, "setText", "(Ljava/lang/CharSequence;)V");
        if (set_text)
        {
            jstring jtext = env->NewStringUTF(text);
            env->CallVoidMethod(clipboard, set_text, jtext);
            env->DeleteLocalRef(jtext);
        }
        env->DeleteLocalRef(clipboard_cls);
        env->DeleteLocalRef(clipboard);
    }
    env->DeleteLocalRef(activity_cls);
    ImGui_ImplAndroid_DetachEnv();
}

static const char* ImGui_ImplAndroid_JniGetClipboardText()
{
    if (!g_NativeActivity)
        return nullptr;
    JNIEnv* env = ImGui_ImplAndroid_GetEnv();
    if (!env) return nullptr;

    jclass activity_cls = env->GetObjectClass(g_NativeActivity);
    if (!activity_cls) { ImGui_ImplAndroid_DetachEnv(); return nullptr; }

    jmethodID get_service = env->GetMethodID(activity_cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    if (!get_service) { env->DeleteLocalRef(activity_cls); ImGui_ImplAndroid_DetachEnv(); return nullptr; }

    jstring service_name = env->NewStringUTF("clipboard");
    jobject clipboard = env->CallObjectMethod(g_NativeActivity, get_service, service_name);
    env->DeleteLocalRef(service_name);

    if (g_ClipboardText) { IM_FREE(g_ClipboardText); g_ClipboardText = nullptr; }

    if (clipboard)
    {
        jclass clipboard_cls = env->GetObjectClass(clipboard);
        jmethodID get_text = env->GetMethodID(clipboard_cls, "getText", "()Ljava/lang/CharSequence;");
        if (get_text)
        {
            jobject sequence = env->CallObjectMethod(clipboard, get_text);
            if (sequence)
            {
                jstring text = (jstring)sequence;
                const char* chars = env->GetStringUTFChars(text, nullptr);
                if (chars)
                {
                    size_t len = strlen(chars);
                    g_ClipboardText = (char*)IM_ALLOC(len + 1);
                    memcpy(g_ClipboardText, chars, len + 1);
                    env->ReleaseStringUTFChars(text, chars);
                }
                env->DeleteLocalRef(sequence);
            }
        }
        env->DeleteLocalRef(clipboard_cls);
        env->DeleteLocalRef(clipboard);
    }
    env->DeleteLocalRef(activity_cls);
    ImGui_ImplAndroid_DetachEnv();
    return g_ClipboardText;
}

void ImGui_ImplAndroid_ShowSoftKeyboard()
{
    ImGui_ImplAndroid_JniShowSoftKeyboard();
}

void ImGui_ImplAndroid_HideSoftKeyboard()
{
    ImGui_ImplAndroid_JniHideSoftKeyboard();
}

bool ImGui_ImplAndroid_Init(ANativeWindow* window, AAssetManager* asset_manager, jobject native_activity)
{
    IMGUI_CHECKVERSION();

    g_Window = window;
    g_AssetManager = asset_manager;
    g_Time = 0.0;

    // Setup backend capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "imgui_impl_android";

    // Setup JNI for soft keyboard, Unicode polling, clipboard, and display metrics
    if (native_activity)
    {
        JNIEnv* env = ImGui_ImplAndroid_GetEnv();
        if (env)
        {
            g_NativeActivity = env->NewGlobalRef(native_activity);
            g_HasJni = true;
            ImGui_ImplAndroid_DetachEnv();
        }
    }

    // Setup clipboard handlers if JNI is available
    if (g_HasJni)
    {
        io.SetClipboardTextFn = [](void* /*user_data*/, const char* text) { ImGui_ImplAndroid_JniSetClipboardText(text); };
        io.GetClipboardTextFn = [](void* /*user_data*/) -> const char* { return ImGui_ImplAndroid_JniGetClipboardText(); };
    }

    // Query initial display metrics
    if (g_HasJni)
        ImGui_ImplAndroid_JniRefreshDisplayMetrics();

    // Fallback display metrics from ANativeWindow if JNI didn't provide them
    if (g_Window)
    {
        int32_t w = ANativeWindow_getWidth(g_Window);
        int32_t h = ANativeWindow_getHeight(g_Window);
        if (g_DisplayMetrics.WidthPixels == 0) g_DisplayMetrics.WidthPixels = w;
        if (g_DisplayMetrics.HeightPixels == 0) g_DisplayMetrics.HeightPixels = h;
        if (g_DisplayMetrics.DensityDpi == 0) g_DisplayMetrics.DensityDpi = 160;  // Default to mdpi
        if (g_DisplayMetrics.Density == 0.0f) g_DisplayMetrics.Density = (float)g_DisplayMetrics.DensityDpi / 160.0f;
        if (g_DisplayMetrics.RefreshRate == 0.0f) g_DisplayMetrics.RefreshRate = 60.0f;
    }

    return true;
}

static void ImGui_ImplAndroid_JniRefreshDisplayMetrics()
{
    if (!g_HasJni || !g_NativeActivity)
        return;

    JNIEnv* env = ImGui_ImplAndroid_GetEnv();
    if (!env)
        return;

    jclass activity_cls = env->GetObjectClass(g_NativeActivity);
    if (!activity_cls) { ImGui_ImplAndroid_DetachEnv(); return; }

    // Refresh rate and orientation via WindowManager
    jmethodID get_wm = env->GetMethodID(activity_cls, "getWindowManager", "()Landroid/view/WindowManager;");
    if (get_wm)
    {
        jobject wm = env->CallObjectMethod(g_NativeActivity, get_wm);
        if (wm)
        {
            jclass wm_cls = env->GetObjectClass(wm);
            jmethodID get_default_display = env->GetMethodID(wm_cls, "getDefaultDisplay", "()Landroid/view/Display;");
            if (get_default_display)
            {
                jobject display = env->CallObjectMethod(wm, get_default_display);
                if (display)
                {
                    jclass display_cls = env->GetObjectClass(display);
                    jmethodID get_refresh = env->GetMethodID(display_cls, "getRefreshRate", "()F");
                    if (get_refresh)
                        g_DisplayMetrics.RefreshRate = env->CallFloatMethod(display, get_refresh);
                    jmethodID get_rotation = env->GetMethodID(display_cls, "getRotation", "()I");
                    if (get_rotation)
                        g_DisplayMetrics.Orientation = env->CallIntMethod(display, get_rotation);
                    env->DeleteLocalRef(display_cls);
                    env->DeleteLocalRef(display);
                }
            }
            env->DeleteLocalRef(wm_cls);
            env->DeleteLocalRef(wm);
        }
    }

    // Density / DPI via Resources
    jmethodID get_res = env->GetMethodID(activity_cls, "getResources", "()Landroid/content/res/Resources;");
    if (get_res)
    {
        jobject res = env->CallObjectMethod(g_NativeActivity, get_res);
        if (res)
        {
            jclass res_cls = env->GetObjectClass(res);
            jmethodID get_dm = env->GetMethodID(res_cls, "getDisplayMetrics", "()Landroid/util/DisplayMetrics;");
            if (get_dm)
            {
                jobject dm = env->CallObjectMethod(res, get_dm);
                if (dm)
                {
                    jclass dm_cls = env->GetObjectClass(dm);
                    jfieldID density_dpi = env->GetFieldID(dm_cls, "densityDpi", "I");
                    jfieldID density = env->GetFieldID(dm_cls, "density", "F");
                    jfieldID xdpi = env->GetFieldID(dm_cls, "xdpi", "F");
                    jfieldID ydpi = env->GetFieldID(dm_cls, "ydpi", "F");
                    jfieldID w_pixels = env->GetFieldID(dm_cls, "widthPixels", "I");
                    jfieldID h_pixels = env->GetFieldID(dm_cls, "heightPixels", "I");
                    if (density_dpi) g_DisplayMetrics.DensityDpi = env->GetIntField(dm, density_dpi);
                    if (density) g_DisplayMetrics.Density = env->GetFloatField(dm, density);
                    if (xdpi) g_DisplayMetrics.Xdpi = env->GetFloatField(dm, xdpi);
                    if (ydpi) g_DisplayMetrics.Ydpi = env->GetFloatField(dm, ydpi);
                    if (w_pixels) g_DisplayMetrics.WidthPixels = env->GetIntField(dm, w_pixels);
                    if (h_pixels) g_DisplayMetrics.HeightPixels = env->GetIntField(dm, h_pixels);
                    env->DeleteLocalRef(dm_cls);
                    env->DeleteLocalRef(dm);
                }
            }
            env->DeleteLocalRef(res_cls);
            env->DeleteLocalRef(res);
        }
    }
    env->DeleteLocalRef(activity_cls);
    ImGui_ImplAndroid_DetachEnv();
}

void ImGui_ImplAndroid_Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = nullptr;

    // Clean up JNI global ref
    if (g_JavaVM && g_NativeActivity)
    {
        JNIEnv* env = ImGui_ImplAndroid_GetEnv();
        if (env)
        {
            env->DeleteGlobalRef(g_NativeActivity);
            ImGui_ImplAndroid_DetachEnv();
        }
        g_NativeActivity = nullptr;
    }
    g_HasJni = false;

    if (g_ClipboardText)
    {
        free(g_ClipboardText);
        g_ClipboardText = nullptr;
    }
}

void ImGui_ImplAndroid_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int32_t window_width = ANativeWindow_getWidth(g_Window);
    int32_t window_height = ANativeWindow_getHeight(g_Window);
    int display_width = window_width;
    int display_height = window_height;

    io.DisplaySize = ImVec2((float)window_width, (float)window_height);
    if (window_width > 0 && window_height > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_width / window_width, (float)display_height / window_height);

    // Re-query display metrics when window size changes (rotation, resize)
    static int last_w = 0, last_h = 0;
    if (window_width != last_w || window_height != last_h)
    {
        last_w = window_width;
        last_h = window_height;
        if (g_HasJni)
            ImGui_ImplAndroid_JniRefreshDisplayMetrics();
    }

    // Setup time step
    struct timespec current_timespec;
    clock_gettime(CLOCK_MONOTONIC, &current_timespec);
    double current_time = (double)(current_timespec.tv_sec) + (current_timespec.tv_nsec / 1000000000.0);
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
    g_Time = current_time;
}

void ImGui_ImplAndroid_GetDisplayMetrics(ImGui_ImplAndroid_DisplayMetrics* out_metrics)
{
    if (out_metrics)
        *out_metrics = g_DisplayMetrics;
}

// --- Optional JNI features (opt-in) ---

void ImGui_ImplAndroid_SetJniEnabled(bool enabled)
{
    g_JniEnabled = enabled;
}

bool ImGui_ImplAndroid_GetJniEnabled()
{
    return g_JniEnabled;
}

void ImGui_ImplAndroid_SetKeyboardType(int input_type)
{
    g_KeyboardInputType = input_type;
}

void ImGui_ImplAndroid_SetKeyboardAction(int ime_action)
{
    g_KeyboardImeAction = ime_action;
}

bool ImGui_ImplAndroid_GetWantTextInput()
{
    ImGuiIO& io = ImGui::GetIO();
    return io.WantTextInput;
}

void ImGui_ImplAndroid_ResubmitTextInput()
{
    ImGuiIO& io = ImGui::GetIO();
    io.WantTextInput = true;
}

int ImGui_ImplAndroid_GetNavBarHeight()
{
    if (!g_HasJni || !g_NativeActivity)
        return 0;
    JNIEnv* env = ImGui_ImplAndroid_GetEnv();
    if (!env)
        return 0;

    int height = 0;
    jclass activity_cls = env->GetObjectClass(g_NativeActivity);
    if (activity_cls)
    {
        jmethodID get_res = env->GetMethodID(activity_cls, "getResources", "()Landroid/content/res/Resources;");
        if (get_res)
        {
            jobject res = env->CallObjectMethod(g_NativeActivity, get_res);
            if (res)
            {
                jclass res_cls = env->GetObjectClass(res);
                jmethodID get_id = env->GetMethodID(res_cls, "getIdentifier", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
                if (get_id)
                {
                    jstring name = env->NewStringUTF("navigation_bar_height");
                    jstring def_type = env->NewStringUTF("dimen");
                    jstring def_package = env->NewStringUTF("android");
                    jint res_id = env->CallIntMethod(res, get_id, name, def_type, def_package);
                    env->DeleteLocalRef(name);
                    env->DeleteLocalRef(def_type);
                    env->DeleteLocalRef(def_package);
                    if (res_id > 0)
                    {
                        jmethodID get_dimen = env->GetMethodID(res_cls, "getDimensionPixelSize", "(I)I");
                        if (get_dimen)
                            height = env->CallIntMethod(res, get_dimen, res_id);
                    }
                }
                env->DeleteLocalRef(res_cls);
                env->DeleteLocalRef(res);
            }
        }
        env->DeleteLocalRef(activity_cls);
    }
    ImGui_ImplAndroid_DetachEnv();
    return height;
}

int ImGui_ImplAndroid_GetBottomInset()
{
    if (!g_HasJni || !g_NativeActivity)
        return 0;
    JNIEnv* env = ImGui_ImplAndroid_GetEnv();
    if (!env)
        return 0;

    int inset = 0;
    jclass activity_cls = env->GetObjectClass(g_NativeActivity);
    if (activity_cls)
    {
        jmethodID get_wm = env->GetMethodID(activity_cls, "getWindowManager", "()Landroid/view/WindowManager;");
        if (get_wm)
        {
            jobject wm = env->CallObjectMethod(g_NativeActivity, get_wm);
            if (wm)
            {
                jclass wm_cls = env->GetObjectClass(wm);
                jmethodID get_default_display = env->GetMethodID(wm_cls, "getDefaultDisplay", "()Landroid/view/Display;");
                if (get_default_display)
                {
                    jobject display = env->CallObjectMethod(wm, get_default_display);
                    if (display)
                    {
                        jclass display_cls = env->GetObjectClass(display);
                        // Android 9+ (API 28): getDisplayCutout()
                        jmethodID get_cutout = env->GetMethodID(display_cls, "getDisplayCutout", "()Landroid/view/DisplayCutout;");
                        if (get_cutout)
                        {
                            jobject cutout = env->CallObjectMethod(display, get_cutout);
                            if (cutout)
                            {
                                jclass cutout_cls = env->GetObjectClass(cutout);
                                jmethodID get_safe_inset_bottom = env->GetMethodID(cutout_cls, "getSafeInsetBottom", "()I");
                                if (get_safe_inset_bottom)
                                    inset = env->CallIntMethod(cutout, get_safe_inset_bottom);
                                env->DeleteLocalRef(cutout_cls);
                                env->DeleteLocalRef(cutout);
                            }
                        }
                        env->DeleteLocalRef(display_cls);
                        env->DeleteLocalRef(display);
                    }
                }
                env->DeleteLocalRef(wm_cls);
                env->DeleteLocalRef(wm);
            }
        }
        env->DeleteLocalRef(activity_cls);
    }
    ImGui_ImplAndroid_DetachEnv();
    return inset;
}

void ImGui_ImplAndroid_SetLongPressCallback(void (*callback)(float x, float y, void* user_data), void* user_data)
{
    g_LongPressCallback = callback;
    g_LongPressUserData = user_data;
}

void ImGui_ImplAndroid_SetLongPressDuration(float seconds)
{
    g_LongPressDuration = seconds;
}

float ImGui_ImplAndroid_GetTouchPressure(int pointer_id)
{
    if (pointer_id < 0 || pointer_id >= IMGUI_ANDROID_MAX_POINTERS)
        return 0.0f;
    return g_TouchPressure[pointer_id];
}

void ImGui_ImplAndroid_SetPressureEnabled(bool enabled)
{
    g_PressureEnabled = enabled;
}

bool ImGui_ImplAndroid_GetPressureEnabled()
{
    return g_PressureEnabled;
}

void ImGui_ImplAndroid_SetPressureThreshold(float min_pressure)
{
    g_TouchPressureMin = min_pressure;
}

// Call this from your main loop to check for long press events
void ImGui_ImplAndroid_UpdateLongPress()
{
    if (!g_LongPressActive || !g_LongPressCallback)
        return;

    struct timespec current_timespec;
    clock_gettime(CLOCK_MONOTONIC, &current_timespec);
    double current_time = (double)(current_timespec.tv_sec) + (current_timespec.tv_nsec / 1000000000.0);

    if (current_time - g_LongPressStartTime >= g_LongPressDuration)
    {
        g_LongPressActive = false;
        g_LongPressCallback(g_LongPressX, g_LongPressY, g_LongPressUserData);
    }
}

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
