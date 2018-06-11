// ImGui Platform Binding for: OSX / Cocoa
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3, Vulkan..)

// FIXME-OSX: Keyboard mapping is weird.
// FIXME-OSX: Mouse cursor shape support (see https://github.com/glfw/glfw/issues/427)
// FIXME-OSX: Test renderer backend combination, e.g. OpenGL3

#include "imgui.h"
#include "imgui_impl_osx.h"
#import <Cocoa/Cocoa.h>

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2018-XX-XX: Initial version.

// Data
static clock_t  g_Time = 0;

// Functions
bool ImGui_ImplOSX_Init()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup back-end capabilities flags
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    //io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_Tab] = 9;
    io.KeyMap[ImGuiKey_LeftArrow] = ImGuiKey_LeftArrow;
    io.KeyMap[ImGuiKey_RightArrow] = ImGuiKey_RightArrow;
    io.KeyMap[ImGuiKey_UpArrow] = ImGuiKey_UpArrow;
    io.KeyMap[ImGuiKey_DownArrow] = ImGuiKey_DownArrow;
    io.KeyMap[ImGuiKey_PageUp] = ImGuiKey_PageUp;
    io.KeyMap[ImGuiKey_PageDown] = ImGuiKey_PageDown;
    io.KeyMap[ImGuiKey_Home] = ImGuiKey_Home;
    io.KeyMap[ImGuiKey_End] = ImGuiKey_End;
    io.KeyMap[ImGuiKey_Insert] = ImGuiKey_Insert;
    io.KeyMap[ImGuiKey_Delete] = ImGuiKey_Delete;
    io.KeyMap[ImGuiKey_Backspace] = 127;
    io.KeyMap[ImGuiKey_Space] = 32;
    io.KeyMap[ImGuiKey_Enter] = 13;
    io.KeyMap[ImGuiKey_Escape] = 27;
    io.KeyMap[ImGuiKey_A] = 'a';
    io.KeyMap[ImGuiKey_C] = 'c';
    io.KeyMap[ImGuiKey_V] = 'v';
    io.KeyMap[ImGuiKey_X] = 'x';
    io.KeyMap[ImGuiKey_Y] = 'y';
    io.KeyMap[ImGuiKey_Z] = 'z';
    
    // Time elapsed since last frame, in seconds
    // (in this sample app we'll override this every frame because our time step is variable)
    io.DeltaTime = 1.0f/60.0f;
    
    return true;
}

void ImGui_ImplOSX_Shutdown()
{
}

void ImGui_ImplOSX_NewFrame(NSOpenGLView* view)
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size
    const float dpi = [view.window backingScaleFactor];
    io.DisplaySize = ImVec2((float)view.bounds.size.width, (float)view.bounds.size.height);
    io.DisplayFramebufferScale = ImVec2(dpi, dpi);

    // Setup time step
    if (g_Time == 0)
        g_Time = clock();
    clock_t current_time = clock();
    io.DeltaTime = (double)(current_time - g_Time) / CLOCKS_PER_SEC;
    g_Time = current_time;

    NSWindow* main_window = [view window];
    NSPoint mouse_pos = [main_window mouseLocationOutsideOfEventStream];
    mouse_pos = [view convertPoint:mouse_pos fromView:nil];
    io.MousePos = ImVec2(mouse_pos.x, mouse_pos.y - 1);

    // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
    ImGui::NewFrame();
}

// FIXME-OSX: Store in io.KeysDown[]
static bool mapKeymap(int* keymap)
{
    if (*keymap == NSUpArrowFunctionKey)
        *keymap = ImGuiKey_LeftArrow;
    else if (*keymap == NSDownArrowFunctionKey)
        *keymap = ImGuiKey_DownArrow;
    else if (*keymap == NSLeftArrowFunctionKey)
        *keymap = ImGuiKey_LeftArrow;
    else if (*keymap == NSRightArrowFunctionKey)
        *keymap = ImGuiKey_RightArrow;
    else if (*keymap == NSHomeFunctionKey)
        *keymap = ImGuiKey_Home;
    else if (*keymap == NSEndFunctionKey)
        *keymap = ImGuiKey_End;
    else if (*keymap == NSDeleteFunctionKey)
        *keymap = ImGuiKey_Delete;
    else if (*keymap == 25) // SHIFT + TAB
        *keymap = 9; // TAB
    else
        return true;
    
    return false;
}

static void ResetKeys()
{
    // FIXME-OSX: Mapping
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[io.KeyMap[ImGuiKey_A]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_C]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_V]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_X]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Y]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Z]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_LeftArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_RightArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Tab]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_UpArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_DownArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Tab]] = false;
}

bool ImGui_ImplOSX_HandleEvent(NSEvent* event)
{
    ImGuiIO& io = ImGui::GetIO();

    if (event.type == NSEventTypeLeftMouseDown)
    {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown))
            io.MouseDown[button] = true;
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeLeftMouseUp)
    {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown))
            io.MouseDown[button] = false;
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeScrollWheel)
    {
        double wheel_dx = 0.0;
        double wheel_dy = 0.0;

        #if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
        if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_6)
        {
            wheel_dx = [event scrollingDeltaX];
            wheel_dy = [event scrollingDeltaY];
            if ([event hasPreciseScrollingDeltas])
            {
                wheel_dx *= 0.1;
                wheel_dy *= 0.1;
            }
        }
        else
        #endif /*MAC_OS_X_VERSION_MAX_ALLOWED*/
        {
            wheel_dx = [event deltaX];
            wheel_dy = [event deltaY];
        }

        if (fabs(wheel_dx) > 0.0)
            io.MouseWheelH += wheel_dx * 0.1f;
        if (fabs(wheel_dy) > 0.0)
            io.MouseWheel += wheel_dy * 0.1f;
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeKeyDown)
    {
        // FIXME-OSX: Try to store native NS keys in KeyDown[]
        NSString* str = [event characters];
        int len = (int)[str length];
        for (int i = 0; i < len; i++)
        {
            int keymap = [str characterAtIndex:i];
            if (mapKeymap(&keymap) && !io.KeyCtrl)
                io.AddInputCharacter(keymap);
            if (keymap < 512)
            {
                // We must reset in case we're pressing a sequence of special keys while keeping the command pressed
                if (io.KeyCtrl)
                    ResetKeys();
                io.KeysDown[keymap] = true;
            }
        }
        return io.WantCaptureKeyboard;
    }

    if (event.type == NSEventTypeKeyUp)
    {
        NSString* str = [event characters];
        int len = (int)[str length];
        for (int i = 0; i < len; i++)
        {
            int keymap = [str characterAtIndex:i];
            mapKeymap(&keymap);
            if (keymap < 512)
                io.KeysDown[keymap] = false;
        }
        return io.WantCaptureKeyboard;
    }

    if (event.type == NSEventTypeFlagsChanged)
    {
        unsigned int flags;
        flags = [event modifierFlags] & NSDeviceIndependentModifierFlagsMask;
        ImGuiIO& io = ImGui::GetIO();
        bool wasKeyShift= io.KeyShift;
        bool wasKeyCtrl = io.KeyCtrl;
        io.KeyShift     = flags & NSShiftKeyMask;
        io.KeyCtrl      = flags & NSCommandKeyMask;
        bool keyShiftReleased = wasKeyShift && !io.KeyShift;
        bool keyCtrlReleased  = wasKeyCtrl  && !io.KeyCtrl;

        // We must reset them as we will not receive any keyUp event if they where pressed during shift or command
        if (keyShiftReleased || keyCtrlReleased)
            ResetKeys();
        return io.WantCaptureKeyboard;
    }
    
    return false;
}
