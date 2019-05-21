// dear imgui: Platform Binding for OSX / Cocoa
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3, Vulkan, Metal..)
// [BETA] Beta bindings, not well tested. If you want a portable application, prefer using the Glfw or SDL platform bindings on Mac.

// Issues:
//  [ ] Platform: Keys are all generally very broken. Best using [event keycode] and not [event characters]..
//  [ ] Platform: Mouse cursor shapes and visibility are not supported (see end of https://github.com/glfw/glfw/issues/427)
//  [ ] Platform: Multi-viewport / platform windows.

#include "imgui.h"
#include "imgui_impl_osx.h"
#import <Cocoa/Cocoa.h>

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2019-05-18: Misc: Removed clipboard handlers as they are now supported by core imgui.cpp.
//  2019-05-11: Inputs: Don't filter character values before calling AddInputCharacter() apart from 0xF700..0xFFFF range.
//  2018-11-30: Misc: Setting up io.BackendPlatformName so it can be displayed in the About Window.
//  2018-07-07: Initial version.

// Data
static CFAbsoluteTime g_Time = 0.0;

// Functions
bool ImGui_ImplOSX_Init()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup back-end capabilities flags
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    //io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    io.BackendPlatformName = "imgui_impl_osx";

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    const int offset_for_function_keys = 256 - 0xF700;
    io.KeyMap[ImGuiKey_Tab]         = '\t';
    io.KeyMap[ImGuiKey_LeftArrow]   = NSLeftArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_RightArrow]  = NSRightArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_UpArrow]     = NSUpArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_DownArrow]   = NSDownArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_PageUp]      = NSPageUpFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_PageDown]    = NSPageDownFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Home]        = NSHomeFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_End]         = NSEndFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Insert]      = NSInsertFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Delete]      = NSDeleteFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Backspace]   = 127;
    io.KeyMap[ImGuiKey_Space]       = 32;
    io.KeyMap[ImGuiKey_Enter]       = 13;
    io.KeyMap[ImGuiKey_Escape]      = 27;
    io.KeyMap[ImGuiKey_A]           = 'A';
    io.KeyMap[ImGuiKey_C]           = 'C';
    io.KeyMap[ImGuiKey_V]           = 'V';
    io.KeyMap[ImGuiKey_X]           = 'X';
    io.KeyMap[ImGuiKey_Y]           = 'Y';
    io.KeyMap[ImGuiKey_Z]           = 'Z';

    // We don't set the io.SetClipboardTextFn/io.GetClipboardTextFn handlers,
    // because imgui.cpp has a default for them that works with OSX.

    return true;
}

void ImGui_ImplOSX_Shutdown()
{
}

void ImGui_ImplOSX_NewFrame(NSView* view)
{
    // Setup display size
    ImGuiIO& io = ImGui::GetIO();
    const float dpi = [view.window backingScaleFactor];
    io.DisplaySize = ImVec2((float)view.bounds.size.width, (float)view.bounds.size.height);
    io.DisplayFramebufferScale = ImVec2(dpi, dpi);

    // Setup time step
    if (g_Time == 0.0)
        g_Time = CFAbsoluteTimeGetCurrent();
    CFAbsoluteTime current_time = CFAbsoluteTimeGetCurrent();
    io.DeltaTime = current_time - g_Time;
    g_Time = current_time;
}

static int mapCharacterToKey(int c)
{
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 'A';
    if (c == 25) // SHIFT+TAB -> TAB
        return 9;
    if (c >= 0 && c < 256)
        return c;
    if (c >= 0xF700 && c < 0xF700 + 256)
        return c - 0xF700 + 256;
    return -1;
}

static void resetKeys()
{
    ImGuiIO& io = ImGui::GetIO();
    for (int n = 0; n < IM_ARRAYSIZE(io.KeysDown); n++)
        io.KeysDown[n] = false;
}

bool ImGui_ImplOSX_HandleEvent(NSEvent* event, NSView* view)
{
    ImGuiIO& io = ImGui::GetIO();

    if (event.type == NSEventTypeLeftMouseDown || event.type == NSEventTypeRightMouseDown || event.type == NSEventTypeOtherMouseDown)
    {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown))
            io.MouseDown[button] = true;
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeLeftMouseUp || event.type == NSEventTypeRightMouseUp || event.type == NSEventTypeOtherMouseUp)
    {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown))
            io.MouseDown[button] = false;
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeMouseMoved || event.type == NSEventTypeLeftMouseDragged)
    {
        NSPoint mousePoint = event.locationInWindow;
        mousePoint = [view convertPoint:mousePoint fromView:nil];
        mousePoint = NSMakePoint(mousePoint.x, view.bounds.size.height - mousePoint.y);
        io.MousePos = ImVec2(mousePoint.x, mousePoint.y);
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

    // FIXME: All the key handling is wrong and broken. Refer to GLFW's cocoa_init.mm and cocoa_window.mm.
    if (event.type == NSEventTypeKeyDown)
    {
        NSString* str = [event characters];
        int len = (int)[str length];
        for (int i = 0; i < len; i++)
        {
            int c = [str characterAtIndex:i];
            if (!io.KeyCtrl && !(c >= 0xF700 && c <= 0xFFFF))
                io.AddInputCharacter((unsigned int)c);

            // We must reset in case we're pressing a sequence of special keys while keeping the command pressed
            int key = mapCharacterToKey(c);
            if (key != -1 && key < 256 && !io.KeyCtrl)
                resetKeys();
            if (key != -1)
                io.KeysDown[key] = true;
        }
        return io.WantCaptureKeyboard;
    }

    if (event.type == NSEventTypeKeyUp)
    {
        NSString* str = [event characters];
        int len = (int)[str length];
        for (int i = 0; i < len; i++)
        {
            int c = [str characterAtIndex:i];
            int key = mapCharacterToKey(c);
            if (key != -1)
                io.KeysDown[key] = false;
        }
        return io.WantCaptureKeyboard;
    }

    if (event.type == NSEventTypeFlagsChanged)
    {
        ImGuiIO& io = ImGui::GetIO();
        unsigned int flags = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

        bool oldKeyCtrl = io.KeyCtrl;
        bool oldKeyShift = io.KeyShift;
        bool oldKeyAlt = io.KeyAlt;
        bool oldKeySuper = io.KeySuper;
        io.KeyCtrl      = flags & NSEventModifierFlagControl;
        io.KeyShift     = flags & NSEventModifierFlagShift;
        io.KeyAlt       = flags & NSEventModifierFlagOption;
        io.KeySuper     = flags & NSEventModifierFlagCommand;

        // We must reset them as we will not receive any keyUp event if they where pressed with a modifier
        if ((oldKeyShift && !io.KeyShift) || (oldKeyCtrl && !io.KeyCtrl) || (oldKeyAlt && !io.KeyAlt) || (oldKeySuper && !io.KeySuper))
            resetKeys();
        return io.WantCaptureKeyboard;
    }

    return false;
}
