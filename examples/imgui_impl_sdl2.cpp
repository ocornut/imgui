// ImGui Platform Binding for: SDL2
// This needs to be used along with a Renderer (e.g. DirectX11, OpenGL3, Vulkan..)
// (Info: SDL2 is a cross-platform general purpose library for handling windows, inputs, graphics context creation, etc.)

// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2018-XX-XX: Platform: Added support for multiple windows via the ImGuiPlatformInterface
//  2018-XX-XX: Misc: ImGui_ImplSDL2_Init() now takes a SDL_GLContext parameter. 
//  2018-02-16: Inputs: Added support for mouse cursors, honoring ImGui::GetMouseCursor() value.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.
//  2018-02-05: Misc: Using SDL_GetPerformanceCounter() instead of SDL_GetTicks() to be able to handle very high framerate (1000+ FPS).
//  2018-02-05: Inputs: Keyboard mapping is using scancodes everywhere instead of a confusing mixture of keycodes and scancodes. 
//  2018-01-20: Inputs: Added Horizontal Mouse Wheel support.
//  2018-01-19: Inputs: When available (SDL 2.0.4+) using SDL_CaptureMouse() to retrieve coordinates outside of client area when dragging. Otherwise (SDL 2.0.3 and before) testing for SDL_WINDOW_INPUT_FOCUS instead of SDL_WINDOW_MOUSE_FOCUS.
//  2018-01-18: Inputs: Added mapping for ImGuiKey_Insert.
//  2017-08-25: Inputs: MousePos set to -FLT_MAX,-FLT_MAX when mouse is unavailable/missing (instead of -1,-1).
//  2016-10-15: Misc: Added a void* user_data parameter to Clipboard function handlers.

#include "imgui.h"
#include "imgui_impl_sdl2.h"

#include "imgui_internal.h" // FIXME-PLATFORM

// SDL
#include <SDL.h>
#include <SDL_syswm.h>

// Data
static Uint64       g_Time = 0;
static bool         g_MousePressed[3] = { false, false, false };
static SDL_Cursor*  g_MouseCursors[ImGuiMouseCursor_Count_] = { 0 };

// Forward Declarations
static void ImGui_ImplSDL2_InitPlatformInterface(SDL_Window* window, void* sdl_gl_context);
static void ImGui_ImplSDL2_ShutdownPlatformInterface();

static const char* ImGui_ImplSDL2_GetClipboardText(void*)
{
    return SDL_GetClipboardText();
}

static void ImGui_ImplSDL2_SetClipboardText(void*, const char* text)
{
    SDL_SetClipboardText(text);
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
bool ImGui_ImplSDL2_ProcessEvent(SDL_Event* event)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (event->type)
    {
    case SDL_MOUSEWHEEL:
        {
            if (event->wheel.x > 0) io.MouseWheelH += 1;
            if (event->wheel.x < 0) io.MouseWheelH -= 1;
            if (event->wheel.y > 0) io.MouseWheel += 1;
            if (event->wheel.y < 0) io.MouseWheel -= 1;
            return true;
        }
    case SDL_MOUSEBUTTONDOWN:
        {
            if (event->button.button == SDL_BUTTON_LEFT) g_MousePressed[0] = true;
            if (event->button.button == SDL_BUTTON_RIGHT) g_MousePressed[1] = true;
            if (event->button.button == SDL_BUTTON_MIDDLE) g_MousePressed[2] = true;
            return true;
        }
    case SDL_TEXTINPUT:
        {
            io.AddInputCharactersUTF8(event->text.text);
            return true;
        }
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        {
            int key = event->key.keysym.scancode;
            IM_ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
            io.KeysDown[key] = (event->type == SDL_KEYDOWN);
            io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
            io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
            io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
            io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
            return true;
        }
    }
    return false;
}

bool    ImGui_ImplSDL2_Init(SDL_Window* window, void* sdl_gl_context)
{
    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
    io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
    io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

    io.SetClipboardTextFn = ImGui_ImplSDL2_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplSDL2_GetClipboardText;
    io.ClipboardUserData = NULL;

    g_MouseCursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    g_MouseCursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    g_MouseCursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    g_MouseCursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    g_MouseCursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);

#ifdef _WIN32
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    io.ImeWindowHandle = wmInfo.info.win.window;
#else
    (void)window;
#endif

    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void*)window;
    if (io.ConfigFlags & ImGuiConfigFlags_MultiViewports)
        ImGui_ImplSDL2_InitPlatformInterface(window, sdl_gl_context);

    return true;
}

void ImGui_ImplSDL2_Shutdown()
{
    ImGui_ImplSDL2_ShutdownPlatformInterface();

    // Destroy SDL mouse cursors
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_Count_; cursor_n++)
        SDL_FreeCursor(g_MouseCursors[cursor_n]);
    memset(g_MouseCursors, 0, sizeof(g_MouseCursors));
}

static void ImGui_ImplSDL2_UpdateMouse()
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    io.MousePosViewport = 0;
    io.MouseHoveredViewport = 0;
    io.ConfigFlags &= ~ImGuiConfigFlags_PlatformHasMouseHoveredViewport; // FIXME-VIEWPORT: We can't get this info properly with SDL, capture is messing up with SDL_WINDOW_MOUSE_FOCUS report and we'd need to handle _NoInputs

    int mx, my;
    Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);
    io.MouseDown[0] = g_MousePressed[0] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;  // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
    io.MouseDown[1] = g_MousePressed[1] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    io.MouseDown[2] = g_MousePressed[2] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;

    SDL_Window* focused_window = SDL_GetKeyboardFocus();
    if (focused_window)
    {
        // SDL_GetMouseState() gives me mouse position seemingly based on the last window entered/focused(?)
        // The creation of new window and SDL_CaptureMouse both seems to severely mess up with that, so we retrieve that position globally.
        int wx, wy;
        SDL_GetWindowPosition(focused_window, &wx, &wy);
        SDL_GetGlobalMouseState(&mx, &my);
        mx -= wx;
        my -= wy;
    }
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void*)focused_window))
    {
        io.MousePos = ImVec2(viewport->Pos.x + (float)mx, viewport->Pos.y + (float)my);
        io.MousePosViewport = viewport->ID;
    }

    // We already retrieve global mouse position, SDL_CaptureMouse() also let the OS know our drag outside boundaries shouldn't trigger, e.g.: OS window resize cursor
    // The function is only supported from SDL 2.0.4 (released Jan 2016)
#if (SDL_MAJOR_VERSION >= 2) && (SDL_MINOR_VERSION >= 0) && (SDL_PATCHLEVEL >= 4)
    bool any_mouse_button_down = ImGui::IsAnyMouseDown();
    SDL_CaptureMouse(any_mouse_button_down ? SDL_TRUE : SDL_FALSE);
#endif

    // Update OS/hardware mouse cursor if imgui isn't drawing a software cursor
    ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None)
    {
        SDL_ShowCursor(SDL_FALSE);
    }
    else
    {
        SDL_SetCursor(g_MouseCursors[cursor] ? g_MouseCursors[cursor] : g_MouseCursors[ImGuiMouseCursor_Arrow]);
        SDL_ShowCursor(SDL_TRUE);
    }
}

void ImGui_ImplSDL2_NewFrame(SDL_Window* window)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt());     // Font atlas needs to be built, call renderer _NewFrame() function e.g. ImGui_ImplOpenGL3_NewFrame() 

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GL_GetDrawableSize(window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

    // Setup time step (we don't use SDL_GetTicks() because it is using millisecond resolution)
    static Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64 current_time = SDL_GetPerformanceCounter();
    io.DeltaTime = g_Time > 0 ? (float)((double)(current_time - g_Time) / frequency) : (float)(1.0f / 60.0f);
    g_Time = current_time;

    ImGui_ImplSDL2_UpdateMouse();

    // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
    ImGui::NewFrame();
}

// --------------------------------------------------------------------------------------------------------
// Platform Windows
// --------------------------------------------------------------------------------------------------------

#include "imgui_internal.h"

#define SDL_HAS_WINDOW_OPACITY  SDL_VERSION_ATLEAST(2,0,5)

struct ImGuiPlatformDataSDL2
{
    SDL_Window*     Window;
    Uint32          WindowID;
    SDL_GLContext   GLContext;

    ImGuiPlatformDataSDL2() { Window = NULL; WindowID = 0; GLContext = NULL; }
    ~ImGuiPlatformDataSDL2() { IM_ASSERT(Window == NULL && GLContext == NULL); }
};

static void ImGui_ImplSDL2_CreateViewport(ImGuiViewport* viewport)
{
    ImGuiPlatformDataSDL2* data = IM_NEW(ImGuiPlatformDataSDL2)();
    viewport->PlatformUserData = data;

    // Share GL resources with main context
    // FIXME-PLATFORM
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGuiPlatformDataSDL2* main_viewport_data = (ImGuiPlatformDataSDL2*)main_viewport->PlatformUserData;
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    SDL_GL_MakeCurrent(main_viewport_data->Window, main_viewport_data->GLContext);

    // We don't enable SDL_WINDOW_RESIZABLE because it enforce windows decorations
    Uint32 sdl_flags = 0;
    sdl_flags |= SDL_WINDOW_OPENGL; // FIXME-PLATFORM
    sdl_flags |= SDL_WINDOW_HIDDEN;
    sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? SDL_WINDOW_BORDERLESS : 0;
    sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? 0 : SDL_WINDOW_RESIZABLE;
    data->Window = SDL_CreateWindow("No Title Yet", 0, 0, (int)viewport->Size.x, (int)viewport->Size.y, sdl_flags);
    data->GLContext = SDL_GL_CreateContext(data->Window);
    viewport->PlatformHandle = (void*)data->Window;
}

static void ImGui_ImplSDL2_DestroyViewport(ImGuiViewport* viewport)
{
    if (ImGuiPlatformDataSDL2* data = (ImGuiPlatformDataSDL2*)viewport->PlatformUserData)
    {
        if (data->GLContext)
            SDL_GL_DeleteContext(data->GLContext);
        data->GLContext = NULL;
        if (data->Window)
            SDL_DestroyWindow(data->Window);
        data->Window = NULL;
        IM_DELETE(data);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = NULL;
}

static void ImGui_ImplSDL2_ShowWindow(ImGuiViewport* viewport)
{
    ImGuiPlatformDataSDL2* data = (ImGuiPlatformDataSDL2*)viewport->PlatformUserData;
#if defined(_WIN32)
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(data->Window, &info))
    {
        HWND hwnd = info.info.win.window;

        // SDL hack: Hide icon from task bar
        // Note: SDL 2.0.6+ has a SDL_WINDOW_SKIP_TASKBAR flag which is supported under Windows but the way it create the window breaks our seamless transition.
        if (viewport->Flags & ImGuiViewportFlags_NoDecoration)
        {
            LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
            ex_style &= ~WS_EX_APPWINDOW;
            ex_style |= WS_EX_TOOLWINDOW;
            ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    }

        // SDL hack: SDL always activate/focus windows :/
        if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
        {
            ::ShowWindow(hwnd, SW_SHOWNA);
            return;
        }
}

#endif
    SDL_ShowWindow(data->Window);
}

static ImVec2 ImGui_ImplSDL2_GetWindowPos(ImGuiViewport* viewport)
{
    ImGuiPlatformDataSDL2* data = (ImGuiPlatformDataSDL2*)viewport->PlatformUserData;
    int x = 0, y = 0;
    SDL_GetWindowPosition(data->Window, &x, &y);
    return ImVec2((float)x, (float)y);
}

static void ImGui_ImplSDL2_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    ImGuiPlatformDataSDL2* data = (ImGuiPlatformDataSDL2*)viewport->PlatformUserData;
    SDL_SetWindowPosition(data->Window, (int)pos.x, (int)pos.y);
}

static ImVec2 ImGui_ImplSDL2_GetWindowSize(ImGuiViewport* viewport)
{
    ImGuiPlatformDataSDL2* data = (ImGuiPlatformDataSDL2*)viewport->PlatformUserData;
    int w = 0, h = 0;
    SDL_GetWindowSize(data->Window, &w, &h);
    return ImVec2((float)w, (float)h);
}

static void ImGui_ImplSDL2_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiPlatformDataSDL2* data = (ImGuiPlatformDataSDL2*)viewport->PlatformUserData;
    SDL_SetWindowSize(data->Window, (int)size.x, (int)size.y);
}

static void ImGui_ImplSDL2_SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    ImGuiPlatformDataSDL2* data = (ImGuiPlatformDataSDL2*)viewport->PlatformUserData;
    SDL_SetWindowTitle(data->Window, title);
}

static void ImGui_ImplSDL2_RenderViewport(ImGuiViewport* viewport)
{
    ImGuiPlatformDataSDL2* data = (ImGuiPlatformDataSDL2*)viewport->PlatformUserData;
    SDL_GL_MakeCurrent(data->Window, data->GLContext);
}

static void ImGui_ImplSDL2_SwapBuffers(ImGuiViewport* viewport)
{
    ImGuiPlatformDataSDL2* data = (ImGuiPlatformDataSDL2*)viewport->PlatformUserData;
    SDL_GL_MakeCurrent(data->Window, data->GLContext);  // FIXME-PLATFORM2
    SDL_GL_SwapWindow(data->Window);
}

static void ImGui_ImplSDL2_InitPlatformInterface(SDL_Window* window, void* sdl_gl_context)
{
    // Register platform interface (will be coupled with a renderer interface)
    ImGuiIO& io = ImGui::GetIO();
    io.PlatformInterface.CreateViewport = ImGui_ImplSDL2_CreateViewport;
    io.PlatformInterface.DestroyViewport = ImGui_ImplSDL2_DestroyViewport;
    io.PlatformInterface.ShowWindow = ImGui_ImplSDL2_ShowWindow;
    io.PlatformInterface.SetWindowPos = ImGui_ImplSDL2_SetWindowPos;
    io.PlatformInterface.GetWindowPos = ImGui_ImplSDL2_GetWindowPos;
    io.PlatformInterface.SetWindowSize = ImGui_ImplSDL2_SetWindowSize;
    io.PlatformInterface.GetWindowSize = ImGui_ImplSDL2_GetWindowSize;
    io.PlatformInterface.SetWindowTitle = ImGui_ImplSDL2_SetWindowTitle;
    io.PlatformInterface.RenderViewport = ImGui_ImplSDL2_RenderViewport;
    io.PlatformInterface.SwapBuffers = ImGui_ImplSDL2_SwapBuffers;

    io.ConfigFlags |= SDL_HAS_WINDOW_OPACITY ? ImGuiConfigFlags_PlatformHasWindowAlpha : 0;

    // Register main window handle
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGuiPlatformDataSDL2* data = IM_NEW(ImGuiPlatformDataSDL2)();
    data->Window = window;
    data->WindowID = SDL_GetWindowID(window);
    data->GLContext = sdl_gl_context;
    main_viewport->PlatformUserData = data;
    main_viewport->PlatformHandle = data->Window;
}

static void ImGui_ImplSDL2_ShutdownPlatformInterface()
{
    ImGuiIO& io = ImGui::GetIO();
    memset(&io.PlatformInterface, 0, sizeof(io.PlatformInterface));
}
