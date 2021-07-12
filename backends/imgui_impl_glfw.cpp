// dear imgui: Platform Backend for GLFW
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan, WebGPU..)
// (Info: GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (Requires: GLFW 3.1+)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Gamepad support. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange' (note: the resizing cursors requires GLFW 3.4+).
//  [X] Platform: Keyboard arrays indexed using GLFW_KEY_* codes, e.g. ImGui::IsKeyPressed(GLFW_KEY_SPACE).

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2021-06-29: Reorganized backend to pull data from a single structure to facilitate usage with multiple-contexts (all g_XXXX access changed to bd->XXXX).
//  2020-01-17: Inputs: Disable error callback while assigning mouse cursors because some X11 setup don't have them and it generates errors.
//  2019-12-05: Inputs: Added support for new mouse cursors added in GLFW 3.4+ (resizing cursors, not allowed cursor).
//  2019-10-18: Misc: Previously installed user callbacks are now restored on shutdown.
//  2019-07-21: Inputs: Added mapping for ImGuiKey_KeyPadEnter.
//  2019-05-11: Inputs: Don't filter value from character callback before calling AddInputCharacter().
//  2019-03-12: Misc: Preserve DisplayFramebufferScale when main window is minimized.
//  2018-11-30: Misc: Setting up io.BackendPlatformName so it can be displayed in the About Window.
//  2018-11-07: Inputs: When installing our GLFW callbacks, we save user's previously installed ones - if any - and chain call them.
//  2018-08-01: Inputs: Workaround for Emscripten which doesn't seem to handle focus related calls.
//  2018-06-29: Inputs: Added support for the ImGuiMouseCursor_Hand cursor.
//  2018-06-08: Misc: Extracted imgui_impl_glfw.cpp/.h away from the old combined GLFW+OpenGL/Vulkan examples.
//  2018-03-20: Misc: Setup io.BackendFlags ImGuiBackendFlags_HasMouseCursors flag + honor ImGuiConfigFlags_NoMouseCursorChange flag.
//  2018-02-20: Inputs: Added support for mouse cursors (ImGui::GetMouseCursor() value, passed to glfwSetCursor()).
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.
//  2018-01-25: Inputs: Added gamepad support if ImGuiConfigFlags_NavEnableGamepad is set.
//  2018-01-25: Inputs: Honoring the io.WantSetMousePos by repositioning the mouse (when using navigation and ImGuiConfigFlags_NavMoveMouse is set).
//  2018-01-20: Inputs: Added Horizontal Mouse Wheel support.
//  2018-01-18: Inputs: Added mapping for ImGuiKey_Insert.
//  2017-08-25: Inputs: MousePos set to -FLT_MAX,-FLT_MAX when mouse is unavailable/missing (instead of -1,-1).
//  2016-10-15: Misc: Added a void* user_data parameter to Clipboard function handlers.

#include "imgui.h"
#include "imgui_impl_glfw.h"

// GLFW
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>   // for glfwGetWin32Window
#endif
#define GLFW_HAS_WINDOW_TOPMOST       (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ GLFW_FLOATING
#define GLFW_HAS_WINDOW_HOVERED       (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ GLFW_HOVERED
#define GLFW_HAS_WINDOW_ALPHA         (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwSetWindowOpacity
#define GLFW_HAS_PER_MONITOR_DPI      (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwGetMonitorContentScale
#define GLFW_HAS_VULKAN               (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ glfwCreateWindowSurface
#ifdef GLFW_RESIZE_NESW_CURSOR  // let's be nice to people who pulled GLFW between 2019-04-16 (3.4 define) and 2019-11-29 (cursors defines) // FIXME: Remove when GLFW 3.4 is released?
#define GLFW_HAS_NEW_CURSORS          (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3400) // 3.4+ GLFW_RESIZE_ALL_CURSOR, GLFW_RESIZE_NESW_CURSOR, GLFW_RESIZE_NWSE_CURSOR, GLFW_NOT_ALLOWED_CURSOR
#else
#define GLFW_HAS_NEW_CURSORS          (0)
#endif

// GLFW data
enum GlfwClientApi
{
    GlfwClientApi_Unknown,
    GlfwClientApi_OpenGL,
    GlfwClientApi_Vulkan
};

struct ImGui_ImplGlfw_Data
{
    GLFWwindow*             Window;
    GlfwClientApi           ClientApi;
    double                  Time;
    bool                    MouseJustPressed[ImGuiMouseButton_COUNT];
    GLFWcursor*             MouseCursors[ImGuiMouseCursor_COUNT];
    bool                    InstalledCallbacks;

    // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
    GLFWmousebuttonfun      PrevUserCallbackMousebutton;
    GLFWscrollfun           PrevUserCallbackScroll;
    GLFWkeyfun              PrevUserCallbackKey;
    GLFWcharfun             PrevUserCallbackChar;
    GLFWmonitorfun          PrevUserCallbackMonitor;

    ImGui_ImplGlfw_Data()   { memset(this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// - Because glfwPollEvents() process all windows and some events may be called outside of it, you will need to register your own callbacks
//   (passing install_callbacks=false in ImGui_ImplGlfw_InitXXX functions), set the current dear imgui context and then call our callbacks.
// - Otherwise we may need to store a GLFWWindow* -> ImGuiContext* map and handle this in the backend, adding a little bit of extra complexity to it.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
static ImGui_ImplGlfw_Data* ImGui_ImplGlfw_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplGlfw_Data*)ImGui::GetIO().BackendPlatformUserData : NULL;
}

// Functions
static const char* ImGui_ImplGlfw_GetClipboardText(void* user_data)
{
    return glfwGetClipboardString((GLFWwindow*)user_data);
}

static void ImGui_ImplGlfw_SetClipboardText(void* user_data, const char* text)
{
    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if (bd->PrevUserCallbackMousebutton != NULL)
        bd->PrevUserCallbackMousebutton(window, button, action, mods);

    if (action == GLFW_PRESS && button >= 0 && button < IM_ARRAYSIZE(bd->MouseJustPressed))
        bd->MouseJustPressed[button] = true;
}

void ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if (bd->PrevUserCallbackScroll != NULL)
        bd->PrevUserCallbackScroll(window, xoffset, yoffset);

    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)xoffset;
    io.MouseWheel += (float)yoffset;
}

void ImGui_ImplGlfw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if (bd->PrevUserCallbackKey != NULL)
        bd->PrevUserCallbackKey(window, key, scancode, action, mods);

    ImGuiIO& io = ImGui::GetIO();
    if (key >= 0 && key < IM_ARRAYSIZE(io.KeysDown))
    {
        if (action == GLFW_PRESS)
            io.KeysDown[key] = true;
        if (action == GLFW_RELEASE)
            io.KeysDown[key] = false;
    }

    // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
#ifdef _WIN32
    io.KeySuper = false;
#else
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
#endif
}

void ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if (bd->PrevUserCallbackChar != NULL)
        bd->PrevUserCallbackChar(window, c);

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

void ImGui_ImplGlfw_MonitorCallback(GLFWmonitor*, int)
{
	// Unused in 'master' branch but 'docking' branch will use this, so we declare it ahead of it so if you have to install callbacks you can install this one too.
}

static bool ImGui_ImplGlfw_Init(GLFWwindow* window, bool install_callbacks, GlfwClientApi client_api)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == NULL && "Already initialized a platform backend!");

    // Setup backend capabilities flags
    ImGui_ImplGlfw_Data* bd = IM_NEW(ImGui_ImplGlfw_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_glfw";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)

    bd->Window = window;
    bd->Time = 0.0;

    // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_Apostrophe] = GLFW_KEY_APOSTROPHE; // '
    io.KeyMap[ImGuiKey_Comma] = GLFW_KEY_COMMA; // ,
    io.KeyMap[ImGuiKey_Minus] = GLFW_KEY_MINUS; // -
    io.KeyMap[ImGuiKey_Period] = GLFW_KEY_PERIOD; // .
    io.KeyMap[ImGuiKey_Slash] = GLFW_KEY_SLASH; // /
    io.KeyMap[ImGuiKey_Semicolon] = GLFW_KEY_SEMICOLON; // ;
    io.KeyMap[ImGuiKey_Equal] = GLFW_KEY_EQUAL; // =
    io.KeyMap[ImGuiKey_LeftBracket] = GLFW_KEY_LEFT_BRACKET; // [
    io.KeyMap[ImGuiKey_Backslash] = GLFW_KEY_BACKSLASH; // \ (this text inhibit multiline comment caused by backlash)
    io.KeyMap[ImGuiKey_RightBracket] = GLFW_KEY_RIGHT_BRACKET; // ]
    io.KeyMap[ImGuiKey_GraveAccent] = GLFW_KEY_GRAVE_ACCENT; // `
    io.KeyMap[ImGuiKey_CapsLock] = GLFW_KEY_CAPS_LOCK;
    io.KeyMap[ImGuiKey_ScrollLock] = GLFW_KEY_SCROLL_LOCK;
    io.KeyMap[ImGuiKey_NumLock] = GLFW_KEY_NUM_LOCK;
    io.KeyMap[ImGuiKey_PrintScreen] = GLFW_KEY_PRINT_SCREEN;
    io.KeyMap[ImGuiKey_Pause] = GLFW_KEY_PAUSE;
    io.KeyMap[ImGuiKey_KeyPad0] = GLFW_KEY_KP_0;
    io.KeyMap[ImGuiKey_KeyPad1] = GLFW_KEY_KP_1;
    io.KeyMap[ImGuiKey_KeyPad2] = GLFW_KEY_KP_2;
    io.KeyMap[ImGuiKey_KeyPad3] = GLFW_KEY_KP_3;
    io.KeyMap[ImGuiKey_KeyPad4] = GLFW_KEY_KP_4;
    io.KeyMap[ImGuiKey_KeyPad5] = GLFW_KEY_KP_5;
    io.KeyMap[ImGuiKey_KeyPad6] = GLFW_KEY_KP_6;
    io.KeyMap[ImGuiKey_KeyPad7] = GLFW_KEY_KP_7;
    io.KeyMap[ImGuiKey_KeyPad8] = GLFW_KEY_KP_8;
    io.KeyMap[ImGuiKey_KeyPad9] = GLFW_KEY_KP_9;
    io.KeyMap[ImGuiKey_KeyPadDecimal] = GLFW_KEY_KP_DECIMAL;
    io.KeyMap[ImGuiKey_KeyPadDivide] = GLFW_KEY_KP_DIVIDE;
    io.KeyMap[ImGuiKey_KeyPadMultiply] = GLFW_KEY_KP_MULTIPLY;
    io.KeyMap[ImGuiKey_KeyPadSubtract] = GLFW_KEY_KP_SUBTRACT;
    io.KeyMap[ImGuiKey_KeyPadAdd] = GLFW_KEY_KP_ADD;
    io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
    io.KeyMap[ImGuiKey_KeyPadEqual] = GLFW_KEY_KP_EQUAL;
    io.KeyMap[ImGuiKey_LeftShift] = GLFW_KEY_LEFT_SHIFT;
    io.KeyMap[ImGuiKey_LeftControl] = GLFW_KEY_LEFT_CONTROL;
    io.KeyMap[ImGuiKey_LeftAlt] = GLFW_KEY_LEFT_ALT;
    io.KeyMap[ImGuiKey_LeftSuper] = GLFW_KEY_LEFT_SUPER;
    io.KeyMap[ImGuiKey_RightShift] = GLFW_KEY_RIGHT_SHIFT;
    io.KeyMap[ImGuiKey_RightControl] = GLFW_KEY_RIGHT_CONTROL;
    io.KeyMap[ImGuiKey_RightAlt] = GLFW_KEY_RIGHT_ALT;
    io.KeyMap[ImGuiKey_RightSuper] = GLFW_KEY_RIGHT_SUPER;
    io.KeyMap[ImGuiKey_Menu] = GLFW_KEY_MENU;
    io.KeyMap[ImGuiKey_0] = GLFW_KEY_0;
    io.KeyMap[ImGuiKey_1] = GLFW_KEY_1;
    io.KeyMap[ImGuiKey_2] = GLFW_KEY_2;
    io.KeyMap[ImGuiKey_3] = GLFW_KEY_3;
    io.KeyMap[ImGuiKey_4] = GLFW_KEY_4;
    io.KeyMap[ImGuiKey_5] = GLFW_KEY_5;
    io.KeyMap[ImGuiKey_6] = GLFW_KEY_6;
    io.KeyMap[ImGuiKey_7] = GLFW_KEY_7;
    io.KeyMap[ImGuiKey_8] = GLFW_KEY_8;
    io.KeyMap[ImGuiKey_9] = GLFW_KEY_9;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_B] = GLFW_KEY_B;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_D] = GLFW_KEY_D;
    io.KeyMap[ImGuiKey_E] = GLFW_KEY_E;
    io.KeyMap[ImGuiKey_F] = GLFW_KEY_F;
    io.KeyMap[ImGuiKey_G] = GLFW_KEY_G;
    io.KeyMap[ImGuiKey_H] = GLFW_KEY_H;
    io.KeyMap[ImGuiKey_I] = GLFW_KEY_I;
    io.KeyMap[ImGuiKey_J] = GLFW_KEY_J;
    io.KeyMap[ImGuiKey_K] = GLFW_KEY_K;
    io.KeyMap[ImGuiKey_L] = GLFW_KEY_L;
    io.KeyMap[ImGuiKey_M] = GLFW_KEY_M;
    io.KeyMap[ImGuiKey_N] = GLFW_KEY_N;
    io.KeyMap[ImGuiKey_O] = GLFW_KEY_O;
    io.KeyMap[ImGuiKey_P] = GLFW_KEY_P;
    io.KeyMap[ImGuiKey_Q] = GLFW_KEY_Q;
    io.KeyMap[ImGuiKey_R] = GLFW_KEY_R;
    io.KeyMap[ImGuiKey_S] = GLFW_KEY_S;
    io.KeyMap[ImGuiKey_T] = GLFW_KEY_T;
    io.KeyMap[ImGuiKey_U] = GLFW_KEY_U;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_W] = GLFW_KEY_W;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
    io.KeyMap[ImGuiKey_F1] = GLFW_KEY_F1;
    io.KeyMap[ImGuiKey_F2] = GLFW_KEY_F2;
    io.KeyMap[ImGuiKey_F3] = GLFW_KEY_F3;
    io.KeyMap[ImGuiKey_F4] = GLFW_KEY_F4;
    io.KeyMap[ImGuiKey_F5] = GLFW_KEY_F5;
    io.KeyMap[ImGuiKey_F6] = GLFW_KEY_F6;
    io.KeyMap[ImGuiKey_F7] = GLFW_KEY_F7;
    io.KeyMap[ImGuiKey_F8] = GLFW_KEY_F8;
    io.KeyMap[ImGuiKey_F9] = GLFW_KEY_F9;
    io.KeyMap[ImGuiKey_F10] = GLFW_KEY_F10;
    io.KeyMap[ImGuiKey_F11] = GLFW_KEY_F11;
    io.KeyMap[ImGuiKey_F12] = GLFW_KEY_F12;

    io.SetClipboardTextFn = ImGui_ImplGlfw_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGlfw_GetClipboardText;
    io.ClipboardUserData = bd->Window;
#if defined(_WIN32)
    io.ImeWindowHandle = (void*)glfwGetWin32Window(bd->Window);
#endif

    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
    // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
    // Missing cursors will return NULL and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(NULL);
    bd->MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
    glfwSetErrorCallback(prev_error_callback);

    // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
    bd->PrevUserCallbackMousebutton = NULL;
    bd->PrevUserCallbackScroll = NULL;
    bd->PrevUserCallbackKey = NULL;
    bd->PrevUserCallbackChar = NULL;
    bd->PrevUserCallbackMonitor = NULL;
    if (install_callbacks)
    {
        bd->InstalledCallbacks = true;
        bd->PrevUserCallbackMousebutton = glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
        bd->PrevUserCallbackScroll = glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
        bd->PrevUserCallbackKey = glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
        bd->PrevUserCallbackChar = glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
        bd->PrevUserCallbackMonitor = glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);
    }

    bd->ClientApi = client_api;
    return true;
}

bool ImGui_ImplGlfw_InitForOpenGL(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_Init(window, install_callbacks, GlfwClientApi_OpenGL);
}

bool ImGui_ImplGlfw_InitForVulkan(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_Init(window, install_callbacks, GlfwClientApi_Vulkan);
}

bool ImGui_ImplGlfw_InitForOther(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_Init(window, install_callbacks, GlfwClientApi_Unknown);
}

void ImGui_ImplGlfw_Shutdown()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();

    if (bd->InstalledCallbacks)
    {
        glfwSetMouseButtonCallback(bd->Window, bd->PrevUserCallbackMousebutton);
        glfwSetScrollCallback(bd->Window, bd->PrevUserCallbackScroll);
        glfwSetKeyCallback(bd->Window, bd->PrevUserCallbackKey);
        glfwSetCharCallback(bd->Window, bd->PrevUserCallbackChar);
        glfwSetMonitorCallback(bd->PrevUserCallbackMonitor);
    }

    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        glfwDestroyCursor(bd->MouseCursors[cursor_n]);

    io.BackendPlatformName = NULL;
    io.BackendPlatformUserData = NULL;
    IM_DELETE(bd);
}

static void ImGui_ImplGlfw_UpdateMousePosAndButtons()
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();

    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = bd->MouseJustPressed[i] || glfwGetMouseButton(bd->Window, i) != 0;
        bd->MouseJustPressed[i] = false;
    }

    // Update mouse position
    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
#ifdef __EMSCRIPTEN__
    const bool focused = true; // Emscripten
#else
    const bool focused = glfwGetWindowAttrib(bd->Window, GLFW_FOCUSED) != 0;
#endif
    if (focused)
    {
        if (io.WantSetMousePos)
        {
            glfwSetCursorPos(bd->Window, (double)mouse_pos_backup.x, (double)mouse_pos_backup.y);
        }
        else
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(bd->Window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
        }
    }
}

static void ImGui_ImplGlfw_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(bd->Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        glfwSetInputMode(bd->Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else
    {
        // Show OS mouse cursor
        // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
        glfwSetCursor(bd->Window, bd->MouseCursors[imgui_cursor] ? bd->MouseCursors[imgui_cursor] : bd->MouseCursors[ImGuiMouseCursor_Arrow]);
        glfwSetInputMode(bd->Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

static void ImGui_ImplGlfw_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    memset(io.NavInputs, 0, sizeof(io.NavInputs));
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    // Update gamepad inputs
    #define MAP_BUTTON(NAV_NO, BUTTON_NO)       { if (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS) io.NavInputs[NAV_NO] = 1.0f; }
    #define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); if (v > 1.0f) v = 1.0f; if (io.NavInputs[NAV_NO] < v) io.NavInputs[NAV_NO] = v; }
    int axes_count = 0, buttons_count = 0;
    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
    const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
    MAP_BUTTON(ImGuiNavInput_Activate,   0);     // Cross / A
    MAP_BUTTON(ImGuiNavInput_Cancel,     1);     // Circle / B
    MAP_BUTTON(ImGuiNavInput_Menu,       2);     // Square / X
    MAP_BUTTON(ImGuiNavInput_Input,      3);     // Triangle / Y
    MAP_BUTTON(ImGuiNavInput_DpadLeft,   13);    // D-Pad Left
    MAP_BUTTON(ImGuiNavInput_DpadRight,  11);    // D-Pad Right
    MAP_BUTTON(ImGuiNavInput_DpadUp,     10);    // D-Pad Up
    MAP_BUTTON(ImGuiNavInput_DpadDown,   12);    // D-Pad Down
    MAP_BUTTON(ImGuiNavInput_FocusPrev,  4);     // L1 / LB
    MAP_BUTTON(ImGuiNavInput_FocusNext,  5);     // R1 / RB
    MAP_BUTTON(ImGuiNavInput_TweakSlow,  4);     // L1 / LB
    MAP_BUTTON(ImGuiNavInput_TweakFast,  5);     // R1 / RB
    MAP_ANALOG(ImGuiNavInput_LStickLeft, 0,  -0.3f,  -0.9f);
    MAP_ANALOG(ImGuiNavInput_LStickRight,0,  +0.3f,  +0.9f);
    MAP_ANALOG(ImGuiNavInput_LStickUp,   1,  +0.3f,  +0.9f);
    MAP_ANALOG(ImGuiNavInput_LStickDown, 1,  -0.3f,  -0.9f);
    #undef MAP_BUTTON
    #undef MAP_ANALOG
    if (axes_count > 0 && buttons_count > 0)
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    else
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
}

void ImGui_ImplGlfw_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplGlfw_InitForXXX()?");

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(bd->Window, &w, &h);
    glfwGetFramebufferSize(bd->Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);

    // Setup time step
    double current_time = glfwGetTime();
    io.DeltaTime = bd->Time > 0.0 ? (float)(current_time - bd->Time) : (float)(1.0f / 60.0f);
    bd->Time = current_time;

    ImGui_ImplGlfw_UpdateMousePosAndButtons();
    ImGui_ImplGlfw_UpdateMouseCursor();

    // Update game controllers (if enabled and available)
    ImGui_ImplGlfw_UpdateGamepads();
}
