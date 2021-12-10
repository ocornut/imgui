// dear imgui: Platform Backend for OSX / Cocoa
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3, Vulkan, Metal..)
// [ALPHA] Early backend, not well tested. If you want a portable application, prefer using the GLFW or SDL platform Backends on Mac.

// Implemented features:
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: OSX clipboard is supported within core Dear ImGui (no specific code in this backend).
//  [X] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Keyboard arrays indexed using kVK_* codes, e.g. ImGui::IsKeyPressed(kVK_Space).

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#import "imgui.h"
#import "imgui_impl_osx.h"
#import <Cocoa/Cocoa.h>
#import <mach/mach_time.h>
#import <Carbon/Carbon.h>
#import <GameController/GameController.h>

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2021-12-13: *BREAKING CHANGE* Add NSView parameter to ImGui_ImplOSX_Init(). Generally fix keyboard support. Using kVK_* codes for keyboard keys.
//  2021-12-13: Add game controller support.
//  2021-09-21: Use mach_absolute_time as CFAbsoluteTimeGetCurrent can jump backwards.
//  2021-08-17: Calling io.AddFocusEvent() on NSApplicationDidBecomeActiveNotification/NSApplicationDidResignActiveNotification events.
//  2021-06-23: Inputs: Added a fix for shortcuts using CTRL key instead of CMD key.
//  2021-04-19: Inputs: Added a fix for keys remaining stuck in pressed state when CMD-tabbing into different application.
//  2021-01-27: Inputs: Added a fix for mouse position not being reported when mouse buttons other than left one are down.
//  2020-10-28: Inputs: Added a fix for handling keypad-enter key.
//  2020-05-25: Inputs: Added a fix for missing trackpad clicks when done with "soft tap".
//  2019-12-05: Inputs: Added support for ImGuiMouseCursor_NotAllowed mouse cursor.
//  2019-10-11: Inputs:  Fix using Backspace key.
//  2019-07-21: Re-added clipboard handlers as they are not enabled by default in core imgui.cpp (reverted 2019-05-18 change).
//  2019-05-28: Inputs: Added mouse cursor shape and visibility support.
//  2019-05-18: Misc: Removed clipboard handlers as they are now supported by core imgui.cpp.
//  2019-05-11: Inputs: Don't filter character values before calling AddInputCharacter() apart from 0xF700..0xFFFF range.
//  2018-11-30: Misc: Setting up io.BackendPlatformName so it can be displayed in the About Window.
//  2018-07-07: Initial version.

@class ImFocusObserver;
@class KeyEventResponder;

// Data
static double               g_HostClockPeriod = 0.0;
static double               g_Time = 0.0;
static NSCursor*            g_MouseCursors[ImGuiMouseCursor_COUNT] = {};
static bool                 g_MouseCursorHidden = false;
static bool                 g_MouseJustPressed[ImGuiMouseButton_COUNT] = {};
static bool                 g_MouseDown[ImGuiMouseButton_COUNT] = {};
static ImFocusObserver*     g_FocusObserver = nil;
static KeyEventResponder*   g_KeyEventResponder = nil;

// Undocumented methods for creating cursors.
@interface NSCursor()
+ (id)_windowResizeNorthWestSouthEastCursor;
+ (id)_windowResizeNorthEastSouthWestCursor;
+ (id)_windowResizeNorthSouthCursor;
+ (id)_windowResizeEastWestCursor;
@end

static void InitHostClockPeriod()
{
    struct mach_timebase_info info;
    mach_timebase_info(&info);
    g_HostClockPeriod = 1e-9 * ((double)info.denom / (double)info.numer); // Period is the reciprocal of frequency.
}

static double GetMachAbsoluteTimeInSeconds()
{
    return (double)mach_absolute_time() * g_HostClockPeriod;
}

static void resetKeys()
{
    ImGuiIO& io = ImGui::GetIO();
    memset(io.KeysDown, 0, sizeof(io.KeysDown));
    io.KeyCtrl = io.KeyShift = io.KeyAlt = io.KeySuper = false;
}

/**
 KeyEventResponder implements the NSTextInputClient protocol as is required by the macOS text input manager.

 The macOS text input manager is invoked by calling the interpretKeyEvents method from the keyDown method.
 Keyboard events are then evaluated by the macOS input manager and valid text input is passed back via the
 insertText:replacementRange method.

 This is the same approach employed by other cross-platform libraries such as SDL2:
  https://github.com/spurious/SDL-mirror/blob/e17aacbd09e65a4fd1e166621e011e581fb017a8/src/video/cocoa/SDL_cocoakeyboard.m#L53
 and GLFW:
  https://github.com/glfw/glfw/blob/b55a517ae0c7b5127dffa79a64f5406021bf9076/src/cocoa_window.m#L722-L723
 */
@interface KeyEventResponder: NSView<NSTextInputClient>
@end

@implementation KeyEventResponder

- (void)viewDidMoveToWindow
{
    // Ensure self is a first responder to receive the input events.
    [self.window makeFirstResponder:self];
}

- (void)keyDown:(NSEvent*)event
{
    // Call to the macOS input manager system.
    [self interpretKeyEvents:@[event]];
}

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange
{
    ImGuiIO& io = ImGui::GetIO();

    NSString* characters;
    if ([aString isKindOfClass:[NSAttributedString class]])
        characters = [aString string];
    else
        characters = (NSString*)aString;

    io.AddInputCharactersUTF8(characters.UTF8String);
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)doCommandBySelector:(SEL)myselector
{
}

- (nullable NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
    return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
    return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
    return NSZeroRect;
}

- (BOOL)hasMarkedText
{
    return NO;
}

- (NSRange)markedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (NSRange)selectedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (void)setMarkedText:(nonnull id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
}

- (void)unmarkText
{
}

- (nonnull NSArray<NSAttributedStringKey>*)validAttributesForMarkedText
{
    return @[];
}

@end

@interface ImFocusObserver : NSObject

- (void)onApplicationBecomeActive:(NSNotification*)aNotification;
- (void)onApplicationBecomeInactive:(NSNotification*)aNotification;

@end

@implementation ImFocusObserver

- (void)onApplicationBecomeActive:(NSNotification*)aNotification
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(true);
}

- (void)onApplicationBecomeInactive:(NSNotification*)aNotification
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(false);

    // Unfocused applications do not receive input events, therefore we must manually
    // release any pressed keys when application loses focus, otherwise they would remain
    // stuck in a pressed state. https://github.com/ocornut/imgui/issues/3832
    resetKeys();
}

@end

// Functions
bool ImGui_ImplOSX_Init(NSView* view)
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup backend capabilities flags
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    //io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    io.BackendPlatformName = "imgui_impl_osx";

    // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_Tab]             = kVK_Tab;
    io.KeyMap[ImGuiKey_LeftArrow]       = kVK_LeftArrow;
    io.KeyMap[ImGuiKey_RightArrow]      = kVK_RightArrow;
    io.KeyMap[ImGuiKey_UpArrow]         = kVK_UpArrow;
    io.KeyMap[ImGuiKey_DownArrow]       = kVK_DownArrow;
    io.KeyMap[ImGuiKey_PageUp]          = kVK_PageUp;
    io.KeyMap[ImGuiKey_PageDown]        = kVK_PageDown;
    io.KeyMap[ImGuiKey_Home]            = kVK_Home;
    io.KeyMap[ImGuiKey_End]             = kVK_End;
    io.KeyMap[ImGuiKey_Insert]          = kVK_F13;
    io.KeyMap[ImGuiKey_Delete]          = kVK_ForwardDelete;
    io.KeyMap[ImGuiKey_Backspace]       = kVK_Delete;
    io.KeyMap[ImGuiKey_Space]           = kVK_Space;
    io.KeyMap[ImGuiKey_Enter]           = kVK_Return;
    io.KeyMap[ImGuiKey_Escape]          = kVK_Escape;
    io.KeyMap[ImGuiKey_KeyPadEnter]     = kVK_ANSI_KeypadEnter;
    io.KeyMap[ImGuiKey_A]               = kVK_ANSI_A;
    io.KeyMap[ImGuiKey_C]               = kVK_ANSI_C;
    io.KeyMap[ImGuiKey_V]               = kVK_ANSI_V;
    io.KeyMap[ImGuiKey_X]               = kVK_ANSI_X;
    io.KeyMap[ImGuiKey_Y]               = kVK_ANSI_Y;
    io.KeyMap[ImGuiKey_Z]               = kVK_ANSI_Z;

    // Load cursors. Some of them are undocumented.
    g_MouseCursorHidden = false;
    g_MouseCursors[ImGuiMouseCursor_Arrow] = [NSCursor arrowCursor];
    g_MouseCursors[ImGuiMouseCursor_TextInput] = [NSCursor IBeamCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeAll] = [NSCursor closedHandCursor];
    g_MouseCursors[ImGuiMouseCursor_Hand] = [NSCursor pointingHandCursor];
    g_MouseCursors[ImGuiMouseCursor_NotAllowed] = [NSCursor operationNotAllowedCursor];
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

    g_FocusObserver = [[ImFocusObserver alloc] init];
    [[NSNotificationCenter defaultCenter] addObserver:g_FocusObserver
                                             selector:@selector(onApplicationBecomeActive:)
                                                 name:NSApplicationDidBecomeActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:g_FocusObserver
                                             selector:@selector(onApplicationBecomeInactive:)
                                                 name:NSApplicationDidResignActiveNotification
                                               object:nil];

    // Add the NSTextInputClient to the view hierarchy,
    // to receive keyboard events and translate them to input text.
    g_KeyEventResponder = [[KeyEventResponder alloc] initWithFrame:NSZeroRect];
    [view addSubview:g_KeyEventResponder];

    return true;
}

void ImGui_ImplOSX_Shutdown()
{
    g_FocusObserver = NULL;
}

static void ImGui_ImplOSX_UpdateMouseCursorAndButtons()
{
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = g_MouseJustPressed[i] || g_MouseDown[i];
        g_MouseJustPressed[i] = false;
    }

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
        NSCursor* desired = g_MouseCursors[imgui_cursor] ?: g_MouseCursors[ImGuiMouseCursor_Arrow];
        // -[NSCursor set] generates measureable overhead if called unconditionally.
        if (desired != NSCursor.currentCursor)
        {
            [desired set];
        }
        if (g_MouseCursorHidden)
        {
            g_MouseCursorHidden = false;
            [NSCursor unhide];
        }
    }
}

void ImGui_ImplOSX_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    memset(io.NavInputs, 0, sizeof(io.NavInputs));
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    GCController* controller;
    if (@available(macOS 11.0, *))
        controller = GCController.current;
    else
        controller = GCController.controllers.firstObject;

    if (controller == nil || controller.extendedGamepad == nil)
    {
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
        return;
    }

    GCExtendedGamepad* gp = controller.extendedGamepad;

#define MAP_BUTTON(NAV_NO, NAME) { io.NavInputs[NAV_NO] = gp.NAME.isPressed ? 1.0 : 0.0; }
    MAP_BUTTON(ImGuiNavInput_Activate, buttonA);
    MAP_BUTTON(ImGuiNavInput_Cancel, buttonB);
    MAP_BUTTON(ImGuiNavInput_Menu, buttonX);
    MAP_BUTTON(ImGuiNavInput_Input, buttonY);
    MAP_BUTTON(ImGuiNavInput_DpadLeft, dpad.left);
    MAP_BUTTON(ImGuiNavInput_DpadRight, dpad.right);
    MAP_BUTTON(ImGuiNavInput_DpadUp, dpad.up);
    MAP_BUTTON(ImGuiNavInput_DpadDown, dpad.down);
    MAP_BUTTON(ImGuiNavInput_FocusPrev, leftShoulder);
    MAP_BUTTON(ImGuiNavInput_FocusNext, rightShoulder);
    MAP_BUTTON(ImGuiNavInput_TweakSlow, leftTrigger);
    MAP_BUTTON(ImGuiNavInput_TweakFast, rightTrigger);
#undef MAP_BUTTON

    io.NavInputs[ImGuiNavInput_LStickLeft] = gp.leftThumbstick.left.value;
    io.NavInputs[ImGuiNavInput_LStickRight] = gp.leftThumbstick.right.value;
    io.NavInputs[ImGuiNavInput_LStickUp] = gp.leftThumbstick.up.value;
    io.NavInputs[ImGuiNavInput_LStickDown] = gp.leftThumbstick.down.value;

    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
}

void ImGui_ImplOSX_NewFrame(NSView* view)
{
    // Setup display size
    ImGuiIO& io = ImGui::GetIO();
    if (view)
    {
        const float dpi = (float)[view.window backingScaleFactor];
        io.DisplaySize = ImVec2((float)view.bounds.size.width, (float)view.bounds.size.height);
        io.DisplayFramebufferScale = ImVec2(dpi, dpi);
    }

    // Setup time step
    if (g_Time == 0.0)
    {
        InitHostClockPeriod();
        g_Time = GetMachAbsoluteTimeInSeconds();
    }
    double current_time = GetMachAbsoluteTimeInSeconds();
    io.DeltaTime = (float)(current_time - g_Time);
    g_Time = current_time;

    ImGui_ImplOSX_UpdateMouseCursorAndButtons();
    ImGui_ImplOSX_UpdateGamepads();
}

NSString* NSStringFromPhase(NSEventPhase phase)
{
    static NSString* strings[] =
    {
        @"none",
        @"began",
        @"stationary",
        @"changed",
        @"ended",
        @"cancelled",
        @"mayBegin",
    };

    int pos = phase == NSEventPhaseNone ? 0 : __builtin_ctzl((NSUInteger)phase) + 1;

    return strings[pos];
}

bool ImGui_ImplOSX_HandleEvent(NSEvent* event, NSView* view)
{
    ImGuiIO& io = ImGui::GetIO();

    if (event.type == NSEventTypeLeftMouseDown || event.type == NSEventTypeRightMouseDown || event.type == NSEventTypeOtherMouseDown)
    {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < IM_ARRAYSIZE(g_MouseDown))
            g_MouseDown[button] = g_MouseJustPressed[button] = true;
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeLeftMouseUp || event.type == NSEventTypeRightMouseUp || event.type == NSEventTypeOtherMouseUp)
    {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < IM_ARRAYSIZE(g_MouseDown))
            g_MouseDown[button] = false;
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeMouseMoved || event.type == NSEventTypeLeftMouseDragged || event.type == NSEventTypeRightMouseDragged || event.type == NSEventTypeOtherMouseDragged)
    {
        NSPoint mousePoint = event.locationInWindow;
        mousePoint = [view convertPoint:mousePoint fromView:nil];
        mousePoint = NSMakePoint(mousePoint.x, view.bounds.size.height - mousePoint.y);
        io.MousePos = ImVec2((float)mousePoint.x, (float)mousePoint.y);
    }

    if (event.type == NSEventTypeScrollWheel)
    {
        // Ignore canceled events.
        //
        // From macOS 12.1, scrolling with two fingers and then decelerating
        // by tapping two fingers results in two events appearing:
        //
        // 1. A scroll wheel NSEvent, with a phase == NSEventPhaseMayBegin, when the user taps
        // two fingers to decelerate or stop the scroll events.
        //
        // 2. A scroll wheel NSEvent, with a phase == NSEventPhaseCancelled, when the user releases the
        // two-finger tap. It is this event that sometimes contains large values for scrollingDeltaX and
        // scrollingDeltaY. When these are added to the current x and y positions of the scrolling view,
        // it appears to jump up or down. It can be observed in Preview, various JetBrains IDEs and here.
        if (event.phase == NSEventPhaseCancelled)
            return false;

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
        #endif // MAC_OS_X_VERSION_MAX_ALLOWED
        {
            wheel_dx = [event deltaX];
            wheel_dy = [event deltaY];
        }

        //NSLog(@"dx=%0.3ff, dy=%0.3f, phase=%@", wheel_dx, wheel_dy, NSStringFromPhase(event.phase));

        if (fabs(wheel_dx) > 0.0)
            io.MouseWheelH += (float)wheel_dx * 0.1f;
        if (fabs(wheel_dy) > 0.0)
            io.MouseWheel += (float)wheel_dy * 0.1f;
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeKeyDown || event.type == NSEventTypeKeyUp)
    {
        unsigned short code = event.keyCode;
        IM_ASSERT(code >= 0 && code < IM_ARRAYSIZE(io.KeysDown));
        io.KeysDown[code] = event.type == NSEventTypeKeyDown;
        NSEventModifierFlags flags = event.modifierFlags;
        io.KeyCtrl  = (flags & NSEventModifierFlagControl) != 0;
        io.KeyShift = (flags & NSEventModifierFlagShift) != 0;
        io.KeyAlt   = (flags & NSEventModifierFlagOption) != 0;
        io.KeySuper = (flags & NSEventModifierFlagCommand) != 0;
        return io.WantCaptureKeyboard;
    }

    if (event.type == NSEventTypeFlagsChanged)
    {
        NSEventModifierFlags flags = event.modifierFlags;
        switch (event.keyCode)
        {
        case kVK_Control:
            io.KeyCtrl = (flags & NSEventModifierFlagControl) != 0;
            break;
        case kVK_Shift:
            io.KeyShift = (flags & NSEventModifierFlagShift) != 0;
            break;
        case kVK_Option:
            io.KeyAlt = (flags & NSEventModifierFlagOption) != 0;
            break;
        case kVK_Command:
            io.KeySuper = (flags & NSEventModifierFlagCommand) != 0;
            break;
        }
        return io.WantCaptureKeyboard;
    }

    return false;
}
