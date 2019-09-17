// dear imgui: Platform Binding for OSX / Cocoa
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3, Vulkan, Metal..)
// [ALPHA] Early bindings, not well tested. If you want a portable application, prefer using the GLFW or SDL platform bindings on Mac.

// Implemented features:
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: OSX clipboard is supported within core Dear ImGui (no specific code in this back-end).
// Issues:
//  [ ] Platform: Keys are all generally very broken. Best using [event keycode] and not [event characters]..
//  [X] Platform: Multi-viewport / platform windows.

#include "imgui.h"
#include "imgui_impl_osx.h"
#import <Cocoa/Cocoa.h>

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2019-09-07: Implement Multi-viewport.
//  2019-07-21: Readded clipboard handlers as they are not enabled by default in core imgui.cpp (reverted 2019-05-18 change).
//  2019-05-28: Inputs: Added mouse cursor shape and visibility support.
//  2019-05-18: Misc: Removed clipboard handlers as they are now supported by core imgui.cpp.
//  2019-05-11: Inputs: Don't filter character values before calling AddInputCharacter() apart from 0xF700..0xFFFF range.
//  2018-11-30: Misc: Setting up io.BackendPlatformName so it can be displayed in the About Window.
//  2018-07-07: Initial version.

// Data
static NSWindow*        g_Window = nil;
static CFAbsoluteTime   g_Time = 0.0;
static NSCursor*        g_MouseCursors[ImGuiMouseCursor_COUNT] = { 0 };
static bool             g_MouseCursorHidden = false;
static bool             g_WantUpdateMonitors = true;

// Forward Declarations
static void ImGui_ImplOSX_InitPlatformInterface();
static void ImGui_ImplOSX_ShutdownPlatformInterface();
static void ImGui_ImplOSX_UpdateMonitors();

// Undocumented methods for creating cursors.
@interface NSCursor()
+ (id)_windowResizeNorthWestSouthEastCursor;
+ (id)_windowResizeNorthEastSouthWestCursor;
+ (id)_windowResizeNorthSouthCursor;
+ (id)_windowResizeEastWestCursor;
@end

// Functions
bool ImGui_ImplOSX_Init(NSView* view)
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup back-end capabilities flags
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    io.BackendPlatformName = "imgui_impl_osx";

    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    g_Window = [view window];
    if (g_Window == nil)
    {
        g_Window = [NSApp orderedWindows].firstObject;
    }
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = main_viewport->PlatformHandleRaw = (__bridge_retained void*)g_Window;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        ImGui_ImplOSX_InitPlatformInterface();

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    const int offset_for_function_keys = 256 - 0xF700;
    io.KeyMap[ImGuiKey_Tab]             = '\t';
    io.KeyMap[ImGuiKey_LeftArrow]       = NSLeftArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_RightArrow]      = NSRightArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_UpArrow]         = NSUpArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_DownArrow]       = NSDownArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_PageUp]          = NSPageUpFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_PageDown]        = NSPageDownFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Home]            = NSHomeFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_End]             = NSEndFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Insert]          = NSInsertFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Delete]          = NSDeleteFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Backspace]       = 127;
    io.KeyMap[ImGuiKey_Space]           = 32;
    io.KeyMap[ImGuiKey_Enter]           = 13;
    io.KeyMap[ImGuiKey_Escape]          = 27;
    io.KeyMap[ImGuiKey_KeyPadEnter]     = 13;
    io.KeyMap[ImGuiKey_A]               = 'A';
    io.KeyMap[ImGuiKey_C]               = 'C';
    io.KeyMap[ImGuiKey_V]               = 'V';
    io.KeyMap[ImGuiKey_X]               = 'X';
    io.KeyMap[ImGuiKey_Y]               = 'Y';
    io.KeyMap[ImGuiKey_Z]               = 'Z';

    // Load cursors. Some of them are undocumented.
    g_MouseCursorHidden = false;
    g_MouseCursors[ImGuiMouseCursor_Arrow] = [NSCursor arrowCursor];
    g_MouseCursors[ImGuiMouseCursor_TextInput] = [NSCursor IBeamCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeAll] = [NSCursor closedHandCursor];
    g_MouseCursors[ImGuiMouseCursor_Hand] = [NSCursor pointingHandCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeNS] = [NSCursor respondsToSelector:@selector(_windowResizeNorthSouthCursor)] ? [NSCursor _windowResizeNorthSouthCursor] : [NSCursor resizeUpDownCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeEW] = [NSCursor respondsToSelector:@selector(_windowResizeEastWestCursor)] ? [NSCursor _windowResizeEastWestCursor] : [NSCursor resizeLeftRightCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = [NSCursor respondsToSelector:@selector(_windowResizeNorthEastSouthWestCursor)] ? [NSCursor _windowResizeNorthEastSouthWestCursor] : [NSCursor closedHandCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = [NSCursor respondsToSelector:@selector(_windowResizeNorthWestSouthEastCursor)] ? [NSCursor _windowResizeNorthWestSouthEastCursor] : [NSCursor closedHandCursor];

    // Note that imgui.cpp also include default OSX clipboard handlers which can be enabled
    // by adding '#define IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS' in imconfig.h and adding '-framework ApplicationServices' to your linker command-line.
    // Since we are already in ObjC land here, it is easy for us to add a clipboard handler using the NSPasteboard api.
    io.SetClipboardTextFn = [](void*, const char* str) -> void
    {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
        [pasteboard setString:[NSString stringWithUTF8String:str] forType:NSPasteboardTypeString];
    };

    io.GetClipboardTextFn = [](void*) -> const char*
    {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        NSString* available = [pasteboard availableTypeFromArray: [NSArray arrayWithObject:NSPasteboardTypeString]];
        if (![available isEqualToString:NSPasteboardTypeString])
            return NULL;

        NSString* string = [pasteboard stringForType:NSPasteboardTypeString];
        if (string == nil)
            return NULL;

        const char* string_c = (const char*)[string UTF8String];
        size_t string_len = strlen(string_c);
        static ImVector<char> s_clipboard;
        s_clipboard.resize((int)string_len + 1);
        strcpy(s_clipboard.Data, string_c);
        return s_clipboard.Data;
    };

    return true;
}

void ImGui_ImplOSX_Shutdown()
{
    ImGui_ImplOSX_ShutdownPlatformInterface();
    g_Window = nil;
}

static void ImGui_ImplOSX_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        if (!g_MouseCursorHidden)
        {
            g_MouseCursorHidden = true;
            [NSCursor hide];
        }
    }
    else
    {
        // Show OS mouse cursor
        [g_MouseCursors[g_MouseCursors[imgui_cursor] ? imgui_cursor : ImGuiMouseCursor_Arrow] set];
        if (g_MouseCursorHidden)
        {
            g_MouseCursorHidden = false;
            [NSCursor unhide];
        }
    }
}

void ImGui_ImplOSX_NewFrame(NSView* view)
{
    // Setup display size
    ImGuiIO& io = ImGui::GetIO();
    NSSize size = [view bounds].size;
    NSSize scale = [view convertSizeToBacking:NSMakeSize(1, 1)];
    io.DisplaySize = ImVec2(size.width, size.height);
    io.DisplayFramebufferScale = ImVec2(scale.width, scale.height);
    if (g_WantUpdateMonitors)
        ImGui_ImplOSX_UpdateMonitors();

    // Setup time step
    if (g_Time == 0.0)
        g_Time = CFAbsoluteTimeGetCurrent();
    CFAbsoluteTime current_time = CFAbsoluteTimeGetCurrent();
    io.DeltaTime = current_time - g_Time;
    g_Time = current_time;

    ImGui_ImplOSX_UpdateMouseCursor();
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
        NSSize size;
        NSPoint mousePoint;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            NSScreen* screen = [[view window] screen];
            size = [screen frame].size;
            mousePoint = [NSEvent mouseLocation];
        }
        else
        {
            size = [view bounds].size;
            mousePoint = event.locationInWindow;
        }
        mousePoint = NSMakePoint(mousePoint.x, size.height - mousePoint.y);
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

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the back-end to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

struct ImGuiViewportDataOSX
{
    NSWindow*               Window;
    bool                    WindowOwned;
    
    ImGuiViewportDataOSX()  { WindowOwned = false; }
    ~ImGuiViewportDataOSX() { IM_ASSERT(Window == nil); }
};

@interface ImGui_ImplOSX_View : NSOpenGLView
@end

@implementation ImGui_ImplOSX_View
-(void)keyUp:(NSEvent *)event           { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)keyDown:(NSEvent *)event         { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)flagsChanged:(NSEvent *)event    { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)mouseDown:(NSEvent *)event       { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)mouseUp:(NSEvent *)event         { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)mouseMoved:(NSEvent *)event      { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)mouseDragged:(NSEvent *)event    { ImGui_ImplOSX_HandleEvent(event, self); }
-(void)scrollWheel:(NSEvent *)event     { ImGui_ImplOSX_HandleEvent(event, self); }
@end

static void ImGui_ImplOSX_CreateWindow(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = IM_NEW(ImGuiViewportDataOSX)();
    viewport->PlatformUserData = data;

    NSScreen* screen = [g_Window screen];
    NSSize size = [screen frame].size;
    NSRect rect = NSMakeRect(viewport->Pos.x, size.height - viewport->Pos.y - viewport->Size.y, viewport->Size.x, viewport->Size.y);

    NSWindow* window = [[NSWindow alloc] initWithContentRect:rect styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:YES];
    [window setTitle:@"Untitled"];
    [window setAcceptsMouseMovedEvents:YES];
    [window setOpaque:NO];
    [window orderFront:NSApp];
    [window setLevel:NSFloatingWindowLevel];

    ImGui_ImplOSX_View* view = [[ImGui_ImplOSX_View alloc] initWithFrame:window.frame];
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_6)
        [view setWantsBestResolutionOpenGLSurface:YES];
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    [window setContentView:view];

    data->Window = window;
    data->WindowOwned = true;
    viewport->PlatformRequestResize = false;
    viewport->PlatformHandle = viewport->PlatformHandleRaw = (__bridge_retained void*)window;
}

static void ImGui_ImplOSX_DestroyWindow(ImGuiViewport* viewport)
{
    NSWindow* window = (__bridge_transfer NSWindow*)viewport->PlatformHandleRaw;
    window = nil;

    if (ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData)
    {
        NSWindow* window = data->Window;
        if (window != nil && data->WindowOwned)
        {
            [window setContentView:nil];
            [window orderOut:nil];
        }
        data->Window = nil;
        IM_DELETE(data);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = viewport->PlatformHandleRaw = NULL;
}

static void ImGui_ImplOSX_ShowWindow(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != nil);
}

static void ImGui_ImplOSX_UpdateWindow(ImGuiViewport* viewport)
{
}

static ImVec2 ImGui_ImplOSX_GetWindowPos(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    NSWindow* window = data->Window;
    NSScreen* screen = [window screen];
    NSSize size = [screen frame].size;
    NSRect frame = [window frame];
    NSRect rect = [window contentLayoutRect];
    return ImVec2(frame.origin.x, size.height - frame.origin.y - rect.size.height);
}

static void ImGui_ImplOSX_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    NSWindow* window = data->Window;
    NSScreen* screen = [window screen];
    NSSize size = [screen frame].size;
    NSRect rect = [window contentLayoutRect];
    NSRect origin = NSMakeRect(pos.x, size.height - pos.y - rect.size.height, 0, 0);
    [window setFrameOrigin:origin.origin];
}

static ImVec2 ImGui_ImplOSX_GetWindowSize(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    NSWindow* window = data->Window;
    NSSize size = [window contentLayoutRect].size;
    return ImVec2(size.width, size.width);
}

static void ImGui_ImplOSX_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    NSWindow* window = data->Window;
    NSRect rect = [window frame];
    rect.origin.y -= (size.y - rect.size.height);
    rect.size.width = size.x;
    rect.size.height = size.y;
    [window setFrame:rect display:YES];
}

static void ImGui_ImplOSX_SetWindowFocus(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    [data->Window orderFront:NSApp];
}

static bool ImGui_ImplOSX_GetWindowFocus(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    return [NSApp orderedWindows].firstObject == data->Window;
}

static bool ImGui_ImplOSX_GetWindowMinimized(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    return [data->Window isMiniaturized];
}

static void ImGui_ImplOSX_SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    [data->Window setTitle:[NSString stringWithUTF8String:title]];
}

static void ImGui_ImplOSX_SetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);
    IM_ASSERT(alpha >= 0.0f && alpha <= 1.0f);

    [data->Window setAlphaValue:alpha];
}

static float ImGui_ImplOSX_GetWindowDpiScale(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    return [data->Window backingScaleFactor];
}

// FIXME-DPI: Testing DPI related ideas
static void ImGui_ImplOSX_OnChangedViewport(ImGuiViewport* viewport)
{
    (void)viewport;
#if 0
    ImGuiStyle default_style;
    //default_style.WindowPadding = ImVec2(0, 0);
    //default_style.WindowBorderSize = 0.0f;
    //default_style.ItemSpacing.y = 3.0f;
    //default_style.FramePadding = ImVec2(0, 0);
    default_style.ScaleAllSizes(viewport->DpiScale);
    ImGuiStyle& style = ImGui::GetStyle();
    style = default_style;
#endif
}

static void ImGui_ImplOSX_UpdateMonitors()
{
    ImGui::GetPlatformIO().Monitors.resize(0);

    NSArray* array = [NSScreen screens];
    for (NSUInteger i = 0; i < array.count; ++i)
    {
        NSScreen* screen = array[i];
        NSRect frame = [screen frame];
        NSRect visibleFrame = [screen visibleFrame];

        ImGuiPlatformMonitor imgui_monitor;
        imgui_monitor.MainPos = ImVec2(frame.origin.x, frame.origin.y);
        imgui_monitor.MainSize = ImVec2(frame.size.width, frame.size.height);
        imgui_monitor.WorkPos = ImVec2(visibleFrame.origin.x, visibleFrame.origin.y);
        imgui_monitor.WorkSize = ImVec2(visibleFrame.size.width, visibleFrame.size.height);
        imgui_monitor.DpiScale = [screen backingScaleFactor];

        ImGuiPlatformIO& io = ImGui::GetPlatformIO();
        io.Monitors.push_back(imgui_monitor);
    }

    g_WantUpdateMonitors = false;
}

static void ImGui_ImplOSX_InitPlatformInterface()
{
    ImGui_ImplOSX_UpdateMonitors();

    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow = ImGui_ImplOSX_CreateWindow;
    platform_io.Platform_DestroyWindow = ImGui_ImplOSX_DestroyWindow;
    platform_io.Platform_ShowWindow = ImGui_ImplOSX_ShowWindow;
    platform_io.Platform_SetWindowPos = ImGui_ImplOSX_SetWindowPos;
    platform_io.Platform_GetWindowPos = ImGui_ImplOSX_GetWindowPos;
    platform_io.Platform_SetWindowSize = ImGui_ImplOSX_SetWindowSize;
    platform_io.Platform_GetWindowSize = ImGui_ImplOSX_GetWindowSize;
    platform_io.Platform_SetWindowFocus = ImGui_ImplOSX_SetWindowFocus;
    platform_io.Platform_GetWindowFocus = ImGui_ImplOSX_GetWindowFocus;
    platform_io.Platform_GetWindowMinimized = ImGui_ImplOSX_GetWindowMinimized;
    platform_io.Platform_SetWindowTitle = ImGui_ImplOSX_SetWindowTitle;
    platform_io.Platform_SetWindowAlpha = ImGui_ImplOSX_SetWindowAlpha;
    platform_io.Platform_UpdateWindow = ImGui_ImplOSX_UpdateWindow;
    platform_io.Platform_GetWindowDpiScale = ImGui_ImplOSX_GetWindowDpiScale; // FIXME-DPI
    platform_io.Platform_OnChangedViewport = ImGui_ImplOSX_OnChangedViewport; // FIXME-DPI

    // Register main window handle (which is owned by the main application, not by us)
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGuiViewportDataOSX* data = IM_NEW(ImGuiViewportDataOSX)();
    data->Window = g_Window;
    data->WindowOwned = false;
    main_viewport->PlatformUserData = data;
    main_viewport->PlatformHandle = (__bridge void*)g_Window;
}

static void ImGui_ImplOSX_ShutdownPlatformInterface()
{
    
}

