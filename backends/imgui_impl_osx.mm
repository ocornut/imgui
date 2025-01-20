// dear imgui: Platform Backend for OSX / Cocoa
// This needs to be used along with a Renderer (e.g. OpenGL2, OpenGL3, Vulkan, Metal..)
// - Not well tested. If you want a portable application, prefer using the GLFW or SDL platform Backends on Mac.
// - Requires linking with the GameController framework ("-framework GameController").

// Implemented features:
//  [X] Platform: Clipboard support is part of core Dear ImGui (no specific code in this backend).
//  [X] Platform: Mouse support. Can discriminate Mouse/Pen.
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy kVK_* values are obsolete since 1.87 and not supported since 1.91.5]
//  [X] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors). Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: IME support.
//  [x] Platform: Multi-viewport / platform windows.
// Issues:
//  [ ] Platform: Multi-viewport: Window size not correctly reported when enabling io.ConfigViewportsNoDecoration
//  [ ] Platform: Multi-viewport: ParentViewportID not honored, and so io.ConfigViewportsNoDefaultParent has no effect (minor).

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#import "imgui.h"
#ifndef IMGUI_DISABLE
#import "imgui_impl_osx.h"
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <GameController/GameController.h>
#import <time.h>

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2025-XX-XX: Added support for multiple windows via the ImGuiPlatformIO interface.
//  2025-01-20: Removed notification observer when shutting down. (#8331)
//  2024-08-22: moved some OS/backend related function pointers from ImGuiIO to ImGuiPlatformIO:
//               - io.GetClipboardTextFn    -> platform_io.Platform_GetClipboardTextFn
//               - io.SetClipboardTextFn    -> platform_io.Platform_SetClipboardTextFn
//               - io.PlatformSetImeDataFn  -> platform_io.Platform_SetImeDataFn
//  2024-07-02: Update for io.SetPlatformImeDataFn() -> io.PlatformSetImeDataFn() renaming in main library.
//  2023-10-05: Inputs: Added support for extra ImGuiKey values: F13 to F20 function keys. Stopped mapping F13 into PrintScreen.
//  2023-04-09: Inputs: Added support for io.AddMouseSourceEvent() to discriminate ImGuiMouseSource_Mouse/ImGuiMouseSource_Pen.
//  2023-02-01: Fixed scroll wheel scaling for devices emitting events with hasPreciseScrollingDeltas==false (e.g. non-Apple mices).
//  2022-11-02: Fixed mouse coordinates before clicking the host window.
//  2022-10-06: Fixed mouse inputs on flipped views.
//  2022-09-26: Inputs: Renamed ImGuiKey_ModXXX introduced in 1.87 to ImGuiMod_XXX (old names still supported).
//  2022-05-03: Inputs: Removed ImGui_ImplOSX_HandleEvent() from backend API in favor of backend automatically handling event capture.
//  2022-04-27: Misc: Store backend data in a per-context struct, allowing to use this backend with multiple contexts.
//  2022-03-22: Inputs: Monitor NSKeyUp events to catch missing keyUp for key when user press Cmd + key
//  2022-02-07: Inputs: Forward keyDown/keyUp events to OS when unused by dear imgui.
//  2022-01-31: Fixed building with old Xcode versions that are missing gamepad features.
//  2022-01-26: Inputs: replaced short-lived io.AddKeyModsEvent() (added two weeks ago) with io.AddKeyEvent() using ImGuiKey_ModXXX flags. Sorry for the confusion.
//  2021-01-20: Inputs: calling new io.AddKeyAnalogEvent() for gamepad support, instead of writing directly to io.NavInputs[].
//  2022-01-17: Inputs: calling new io.AddMousePosEvent(), io.AddMouseButtonEvent(), io.AddMouseWheelEvent() API (1.87+).
//  2022-01-12: Inputs: Added basic Platform IME support, hooking the io.SetPlatformImeDataFn() function.
//  2022-01-10: Inputs: calling new io.AddKeyEvent(), io.AddKeyModsEvent() + io.SetKeyEventNativeData() API (1.87+). Support for full ImGuiKey range.
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

#define APPLE_HAS_BUTTON_OPTIONS (__IPHONE_OS_VERSION_MIN_REQUIRED >= 130000 || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500 || __TV_OS_VERSION_MIN_REQUIRED >= 130000)
#define APPLE_HAS_CONTROLLER     (__IPHONE_OS_VERSION_MIN_REQUIRED >= 140000 || __MAC_OS_X_VERSION_MIN_REQUIRED >= 110000 || __TV_OS_VERSION_MIN_REQUIRED >= 140000)
#define APPLE_HAS_THUMBSTICKS    (__IPHONE_OS_VERSION_MIN_REQUIRED >= 120100 || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101401 || __TV_OS_VERSION_MIN_REQUIRED >= 120100)

@class ImGuiObserver;
@class KeyEventResponder;

// Data
struct ImGui_ImplOSX_Data
{
    CFTimeInterval              Time;
    NSCursor*                   MouseCursors[ImGuiMouseCursor_COUNT];
    bool                        MouseCursorHidden;
    ImGuiObserver*              Observer;
    KeyEventResponder*          KeyEventResponder;
    NSTextInputContext*         InputContext;
    id                          Monitor;
    NSWindow*                   Window;

    ImGui_ImplOSX_Data()        { memset((void*)this, 0, sizeof(*this)); }
};

static ImGui_ImplOSX_Data*      ImGui_ImplOSX_GetBackendData()      { return (ImGui_ImplOSX_Data*)ImGui::GetIO().BackendPlatformUserData; }
static void                     ImGui_ImplOSX_DestroyBackendData()  { IM_DELETE(ImGui_ImplOSX_GetBackendData()); }

static inline CFTimeInterval    GetMachAbsoluteTimeInSeconds()      { return (CFTimeInterval)(double)(clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1e9); }

// Forward Declarations
static void ImGui_ImplOSX_InitMultiViewportSupport();
static void ImGui_ImplOSX_ShutdownMultiViewportSupport();
static void ImGui_ImplOSX_UpdateMonitors();
static void ImGui_ImplOSX_AddTrackingArea(NSView* _Nonnull view);
static bool ImGui_ImplOSX_HandleEvent(NSEvent* event, NSView* view);

// Undocumented methods for creating cursors.
@interface NSCursor()
+ (id)_windowResizeNorthWestSouthEastCursor;
+ (id)_windowResizeNorthEastSouthWestCursor;
+ (id)_windowResizeNorthSouthCursor;
+ (id)_windowResizeEastWestCursor;
@end

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
{
    float _posX;
    float _posY;
    NSRect _imeRect;
}

#pragma mark - Public

- (void)setImePosX:(float)posX imePosY:(float)posY
{
    _posX = posX;
    _posY = posY;
}

- (void)updateImePosWithView:(NSView *)view
{
    NSWindow* window = view.window;
    if (!window)
        return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        NSRect frame = window.frame;
        NSRect contentRect = window.contentLayoutRect;
        if (window.styleMask & NSWindowStyleMaskFullSizeContentView) // No title bar windows should be considered.
            contentRect = frame;

        NSRect firstScreenFrame = NSScreen.screens[0].frame;
        _imeRect = NSMakeRect(_posX, _posY, 0, 0);
        _imeRect.origin.y = firstScreenFrame.size.height - _imeRect.size.height - _imeRect.origin.y; // Opposite of ConvertNSRect()
    }
    else
    {
        NSRect contentRect = [window contentRectForFrameRect:window.frame];
        NSRect rect = NSMakeRect(_posX, contentRect.size.height - _posY, 0, 0);
        _imeRect = [window convertRectToScreen:rect];
    }
}

- (void)viewDidMoveToWindow
{
    // Ensure self is a first responder to receive the input events.
    [self.window makeFirstResponder:self];
}

- (void)keyDown:(NSEvent*)event
{
    if (!ImGui_ImplOSX_HandleEvent(event, self))
        [super keyDown:event];

    // Call to the macOS input manager system.
    [self interpretKeyEvents:@[event]];
}

- (void)keyUp:(NSEvent*)event
{
    if (!ImGui_ImplOSX_HandleEvent(event, self))
        [super keyUp:event];
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
    return _imeRect;
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

@interface ImGuiObserver : NSObject

- (void)onApplicationBecomeActive:(NSNotification*)aNotification;
- (void)onApplicationBecomeInactive:(NSNotification*)aNotification;
- (void)displaysDidChange:(NSNotification*)aNotification;

@end

@implementation ImGuiObserver

- (void)onApplicationBecomeActive:(NSNotification*)aNotification
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(true);
}

- (void)onApplicationBecomeInactive:(NSNotification*)aNotification
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(false);
}

- (void)displaysDidChange:(NSNotification*)aNotification
{
    ImGui_ImplOSX_UpdateMonitors();
}

@end

// Functions

// Not static to allow third-party code to use that if they want to (but undocumented)
ImGuiKey ImGui_ImplOSX_KeyCodeToImGuiKey(int key_code);
ImGuiKey ImGui_ImplOSX_KeyCodeToImGuiKey(int key_code)
{
    switch (key_code)
    {
        case kVK_ANSI_A: return ImGuiKey_A;
        case kVK_ANSI_S: return ImGuiKey_S;
        case kVK_ANSI_D: return ImGuiKey_D;
        case kVK_ANSI_F: return ImGuiKey_F;
        case kVK_ANSI_H: return ImGuiKey_H;
        case kVK_ANSI_G: return ImGuiKey_G;
        case kVK_ANSI_Z: return ImGuiKey_Z;
        case kVK_ANSI_X: return ImGuiKey_X;
        case kVK_ANSI_C: return ImGuiKey_C;
        case kVK_ANSI_V: return ImGuiKey_V;
        case kVK_ANSI_B: return ImGuiKey_B;
        case kVK_ANSI_Q: return ImGuiKey_Q;
        case kVK_ANSI_W: return ImGuiKey_W;
        case kVK_ANSI_E: return ImGuiKey_E;
        case kVK_ANSI_R: return ImGuiKey_R;
        case kVK_ANSI_Y: return ImGuiKey_Y;
        case kVK_ANSI_T: return ImGuiKey_T;
        case kVK_ANSI_1: return ImGuiKey_1;
        case kVK_ANSI_2: return ImGuiKey_2;
        case kVK_ANSI_3: return ImGuiKey_3;
        case kVK_ANSI_4: return ImGuiKey_4;
        case kVK_ANSI_6: return ImGuiKey_6;
        case kVK_ANSI_5: return ImGuiKey_5;
        case kVK_ANSI_Equal: return ImGuiKey_Equal;
        case kVK_ANSI_9: return ImGuiKey_9;
        case kVK_ANSI_7: return ImGuiKey_7;
        case kVK_ANSI_Minus: return ImGuiKey_Minus;
        case kVK_ANSI_8: return ImGuiKey_8;
        case kVK_ANSI_0: return ImGuiKey_0;
        case kVK_ANSI_RightBracket: return ImGuiKey_RightBracket;
        case kVK_ANSI_O: return ImGuiKey_O;
        case kVK_ANSI_U: return ImGuiKey_U;
        case kVK_ANSI_LeftBracket: return ImGuiKey_LeftBracket;
        case kVK_ANSI_I: return ImGuiKey_I;
        case kVK_ANSI_P: return ImGuiKey_P;
        case kVK_ANSI_L: return ImGuiKey_L;
        case kVK_ANSI_J: return ImGuiKey_J;
        case kVK_ANSI_Quote: return ImGuiKey_Apostrophe;
        case kVK_ANSI_K: return ImGuiKey_K;
        case kVK_ANSI_Semicolon: return ImGuiKey_Semicolon;
        case kVK_ANSI_Backslash: return ImGuiKey_Backslash;
        case kVK_ANSI_Comma: return ImGuiKey_Comma;
        case kVK_ANSI_Slash: return ImGuiKey_Slash;
        case kVK_ANSI_N: return ImGuiKey_N;
        case kVK_ANSI_M: return ImGuiKey_M;
        case kVK_ANSI_Period: return ImGuiKey_Period;
        case kVK_ANSI_Grave: return ImGuiKey_GraveAccent;
        case kVK_ANSI_KeypadDecimal: return ImGuiKey_KeypadDecimal;
        case kVK_ANSI_KeypadMultiply: return ImGuiKey_KeypadMultiply;
        case kVK_ANSI_KeypadPlus: return ImGuiKey_KeypadAdd;
        case kVK_ANSI_KeypadClear: return ImGuiKey_NumLock;
        case kVK_ANSI_KeypadDivide: return ImGuiKey_KeypadDivide;
        case kVK_ANSI_KeypadEnter: return ImGuiKey_KeypadEnter;
        case kVK_ANSI_KeypadMinus: return ImGuiKey_KeypadSubtract;
        case kVK_ANSI_KeypadEquals: return ImGuiKey_KeypadEqual;
        case kVK_ANSI_Keypad0: return ImGuiKey_Keypad0;
        case kVK_ANSI_Keypad1: return ImGuiKey_Keypad1;
        case kVK_ANSI_Keypad2: return ImGuiKey_Keypad2;
        case kVK_ANSI_Keypad3: return ImGuiKey_Keypad3;
        case kVK_ANSI_Keypad4: return ImGuiKey_Keypad4;
        case kVK_ANSI_Keypad5: return ImGuiKey_Keypad5;
        case kVK_ANSI_Keypad6: return ImGuiKey_Keypad6;
        case kVK_ANSI_Keypad7: return ImGuiKey_Keypad7;
        case kVK_ANSI_Keypad8: return ImGuiKey_Keypad8;
        case kVK_ANSI_Keypad9: return ImGuiKey_Keypad9;
        case kVK_Return: return ImGuiKey_Enter;
        case kVK_Tab: return ImGuiKey_Tab;
        case kVK_Space: return ImGuiKey_Space;
        case kVK_Delete: return ImGuiKey_Backspace;
        case kVK_Escape: return ImGuiKey_Escape;
        case kVK_CapsLock: return ImGuiKey_CapsLock;
        case kVK_Control: return ImGuiKey_LeftCtrl;
        case kVK_Shift: return ImGuiKey_LeftShift;
        case kVK_Option: return ImGuiKey_LeftAlt;
        case kVK_Command: return ImGuiKey_LeftSuper;
        case kVK_RightControl: return ImGuiKey_RightCtrl;
        case kVK_RightShift: return ImGuiKey_RightShift;
        case kVK_RightOption: return ImGuiKey_RightAlt;
        case kVK_RightCommand: return ImGuiKey_RightSuper;
//      case kVK_Function: return ImGuiKey_;
//      case kVK_VolumeUp: return ImGuiKey_;
//      case kVK_VolumeDown: return ImGuiKey_;
//      case kVK_Mute: return ImGuiKey_;
        case kVK_F1: return ImGuiKey_F1;
        case kVK_F2: return ImGuiKey_F2;
        case kVK_F3: return ImGuiKey_F3;
        case kVK_F4: return ImGuiKey_F4;
        case kVK_F5: return ImGuiKey_F5;
        case kVK_F6: return ImGuiKey_F6;
        case kVK_F7: return ImGuiKey_F7;
        case kVK_F8: return ImGuiKey_F8;
        case kVK_F9: return ImGuiKey_F9;
        case kVK_F10: return ImGuiKey_F10;
        case kVK_F11: return ImGuiKey_F11;
        case kVK_F12: return ImGuiKey_F12;
        case kVK_F13: return ImGuiKey_F13;
        case kVK_F14: return ImGuiKey_F14;
        case kVK_F15: return ImGuiKey_F15;
        case kVK_F16: return ImGuiKey_F16;
        case kVK_F17: return ImGuiKey_F17;
        case kVK_F18: return ImGuiKey_F18;
        case kVK_F19: return ImGuiKey_F19;
        case kVK_F20: return ImGuiKey_F20;
        case 0x6E: return ImGuiKey_Menu;
        case kVK_Help: return ImGuiKey_Insert;
        case kVK_Home: return ImGuiKey_Home;
        case kVK_PageUp: return ImGuiKey_PageUp;
        case kVK_ForwardDelete: return ImGuiKey_Delete;
        case kVK_End: return ImGuiKey_End;
        case kVK_PageDown: return ImGuiKey_PageDown;
        case kVK_LeftArrow: return ImGuiKey_LeftArrow;
        case kVK_RightArrow: return ImGuiKey_RightArrow;
        case kVK_DownArrow: return ImGuiKey_DownArrow;
        case kVK_UpArrow: return ImGuiKey_UpArrow;
        default: return ImGuiKey_None;
    }
}

#ifdef IMGUI_IMPL_METAL_CPP_EXTENSIONS

IMGUI_IMPL_API bool ImGui_ImplOSX_Init(void* _Nonnull view) {
    return ImGui_ImplOSX_Init((__bridge NSView*)(view));
}

IMGUI_IMPL_API void ImGui_ImplOSX_NewFrame(void* _Nullable view) {
    return ImGui_ImplOSX_NewFrame((__bridge NSView*)(view));
}

#endif


bool ImGui_ImplOSX_Init(NSView* view)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    // Setup backend capabilities flags
    ImGui_ImplOSX_Data* bd = IM_NEW(ImGui_ImplOSX_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_osx";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;      // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data (optional)

    bd->Observer = [ImGuiObserver new];
    bd->Window = view.window ?: NSApp.orderedWindows.firstObject;
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = main_viewport->PlatformHandleRaw = (__bridge_retained void*)bd->Window;
    ImGui_ImplOSX_UpdateMonitors();
    ImGui_ImplOSX_InitMultiViewportSupport();

    // Load cursors. Some of them are undocumented.
    bd->MouseCursorHidden = false;
    bd->MouseCursors[ImGuiMouseCursor_Arrow] = [NSCursor arrowCursor];
    bd->MouseCursors[ImGuiMouseCursor_TextInput] = [NSCursor IBeamCursor];
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = [NSCursor closedHandCursor];
    bd->MouseCursors[ImGuiMouseCursor_Hand] = [NSCursor pointingHandCursor];
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = [NSCursor operationNotAllowedCursor];
    bd->MouseCursors[ImGuiMouseCursor_ResizeNS] = [NSCursor respondsToSelector:@selector(_windowResizeNorthSouthCursor)] ? [NSCursor _windowResizeNorthSouthCursor] : [NSCursor resizeUpDownCursor];
    bd->MouseCursors[ImGuiMouseCursor_ResizeEW] = [NSCursor respondsToSelector:@selector(_windowResizeEastWestCursor)] ? [NSCursor _windowResizeEastWestCursor] : [NSCursor resizeLeftRightCursor];
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = [NSCursor respondsToSelector:@selector(_windowResizeNorthEastSouthWestCursor)] ? [NSCursor _windowResizeNorthEastSouthWestCursor] : [NSCursor closedHandCursor];
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = [NSCursor respondsToSelector:@selector(_windowResizeNorthWestSouthEastCursor)] ? [NSCursor _windowResizeNorthWestSouthEastCursor] : [NSCursor closedHandCursor];

    // Note that imgui.cpp also include default OSX clipboard handlers which can be enabled
    // by adding '#define IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS' in imconfig.h and adding '-framework ApplicationServices' to your linker command-line.
    // Since we are already in ObjC land here, it is easy for us to add a clipboard handler using the NSPasteboard api.
    platform_io.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* str) -> void
    {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
        [pasteboard setString:[NSString stringWithUTF8String:str] forType:NSPasteboardTypeString];
    };

    platform_io.Platform_GetClipboardTextFn = [](ImGuiContext*) -> const char*
    {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        NSString* available = [pasteboard availableTypeFromArray: [NSArray arrayWithObject:NSPasteboardTypeString]];
        if (![available isEqualToString:NSPasteboardTypeString])
            return nullptr;

        NSString* string = [pasteboard stringForType:NSPasteboardTypeString];
        if (string == nil)
            return nullptr;

        const char* string_c = (const char*)[string UTF8String];
        size_t string_len = strlen(string_c);
        static ImVector<char> s_clipboard;
        s_clipboard.resize((int)string_len + 1);
        strcpy(s_clipboard.Data, string_c);
        return s_clipboard.Data;
    };

    [[NSNotificationCenter defaultCenter] addObserver:bd->Observer
                                             selector:@selector(onApplicationBecomeActive:)
                                                 name:NSApplicationDidBecomeActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:bd->Observer
                                             selector:@selector(onApplicationBecomeInactive:)
                                                 name:NSApplicationDidResignActiveNotification
                                               object:nil];

    // Add the NSTextInputClient to the view hierarchy,
    // to receive keyboard events and translate them to input text.
    bd->KeyEventResponder = [[KeyEventResponder alloc] initWithFrame:NSZeroRect];
    bd->InputContext = [[NSTextInputContext alloc] initWithClient:bd->KeyEventResponder];
    [view addSubview:bd->KeyEventResponder];
    ImGui_ImplOSX_AddTrackingArea(view);

    platform_io.Platform_SetImeDataFn = [](ImGuiContext*, ImGuiViewport* viewport, ImGuiPlatformImeData* data) -> void
    {
        ImGui_ImplOSX_Data* bd = ImGui_ImplOSX_GetBackendData();
        if (data->WantVisible)
        {
            [bd->InputContext activate];
        }
        else
        {
            [bd->InputContext discardMarkedText];
            [bd->InputContext invalidateCharacterCoordinates];
            [bd->InputContext deactivate];
        }
        [bd->KeyEventResponder setImePosX:data->InputPos.x imePosY:data->InputPos.y + data->InputLineHeight];
    };

    return true;
}

void ImGui_ImplOSX_Shutdown()
{
    ImGui_ImplOSX_Data* bd = ImGui_ImplOSX_GetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");

    [[NSNotificationCenter defaultCenter] removeObserver:bd->Observer];
    bd->Observer = nullptr;
    if (bd->Monitor != nullptr)
    {
        [NSEvent removeMonitor:bd->Monitor];
        bd->Monitor = nullptr;
    }

    ImGui_ImplOSX_ShutdownMultiViewportSupport();
    ImGui_ImplOSX_DestroyBackendData();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_PlatformHasViewports);
}

static void ImGui_ImplOSX_UpdateMouseCursor()
{
    ImGui_ImplOSX_Data* bd = ImGui_ImplOSX_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        if (!bd->MouseCursorHidden)
        {
            bd->MouseCursorHidden = true;
            [NSCursor hide];
        }
    }
    else
    {
        NSCursor* desired = bd->MouseCursors[imgui_cursor] ?: bd->MouseCursors[ImGuiMouseCursor_Arrow];
        // -[NSCursor set] generates measureable overhead if called unconditionally.
        if (desired != NSCursor.currentCursor)
        {
            [desired set];
        }
        if (bd->MouseCursorHidden)
        {
            bd->MouseCursorHidden = false;
            [NSCursor unhide];
        }
    }
}

static void ImGui_ImplOSX_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0) // FIXME: Technically feeding gamepad shouldn't depend on this now that they are regular inputs.
        return;

#if APPLE_HAS_CONTROLLER
    GCController* controller = GCController.current;
#else
    GCController* controller = GCController.controllers.firstObject;
#endif
    if (controller == nil || controller.extendedGamepad == nil)
    {
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
        return;
    }

    GCExtendedGamepad* gp = controller.extendedGamepad;

    // Update gamepad inputs
    #define IM_SATURATE(V)                        (V < 0.0f ? 0.0f : V > 1.0f ? 1.0f : V)
    #define MAP_BUTTON(KEY_NO, BUTTON_NAME)       { io.AddKeyEvent(KEY_NO, gp.BUTTON_NAME.isPressed); }
    #define MAP_ANALOG(KEY_NO, AXIS_NAME, V0, V1) { float vn = (float)(gp.AXIS_NAME.value - V0) / (float)(V1 - V0); vn = IM_SATURATE(vn); io.AddKeyAnalogEvent(KEY_NO, vn > 0.1f, vn); }
    const float thumb_dead_zone = 0.0f;

#if APPLE_HAS_BUTTON_OPTIONS
    MAP_BUTTON(ImGuiKey_GamepadBack,            buttonOptions);
#endif
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft,        buttonX);              // Xbox X, PS Square
    MAP_BUTTON(ImGuiKey_GamepadFaceRight,       buttonB);              // Xbox B, PS Circle
    MAP_BUTTON(ImGuiKey_GamepadFaceUp,          buttonY);              // Xbox Y, PS Triangle
    MAP_BUTTON(ImGuiKey_GamepadFaceDown,        buttonA);              // Xbox A, PS Cross
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft,        dpad.left);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight,       dpad.right);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp,          dpad.up);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown,        dpad.down);
    MAP_ANALOG(ImGuiKey_GamepadL1,              leftShoulder, 0.0f, 1.0f);
    MAP_ANALOG(ImGuiKey_GamepadR1,              rightShoulder, 0.0f, 1.0f);
    MAP_ANALOG(ImGuiKey_GamepadL2,              leftTrigger,  0.0f, 1.0f);
    MAP_ANALOG(ImGuiKey_GamepadR2,              rightTrigger, 0.0f, 1.0f);
#if APPLE_HAS_THUMBSTICKS
    MAP_BUTTON(ImGuiKey_GamepadL3,              leftThumbstickButton);
    MAP_BUTTON(ImGuiKey_GamepadR3,              rightThumbstickButton);
#endif
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,      leftThumbstick.xAxis,  -thumb_dead_zone, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight,     leftThumbstick.xAxis,  +thumb_dead_zone, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,        leftThumbstick.yAxis,  +thumb_dead_zone, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,      leftThumbstick.yAxis,  -thumb_dead_zone, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,      rightThumbstick.xAxis, -thumb_dead_zone, -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight,     rightThumbstick.xAxis, +thumb_dead_zone, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,        rightThumbstick.yAxis, +thumb_dead_zone, +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,      rightThumbstick.yAxis, -thumb_dead_zone, -1.0f);
    #undef MAP_BUTTON
    #undef MAP_ANALOG

    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
}

static void ImGui_ImplOSX_UpdateImePosWithView(NSView* view)
{
    ImGui_ImplOSX_Data* bd = ImGui_ImplOSX_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput)
        [bd->KeyEventResponder updateImePosWithView:view];
}

void ImGui_ImplOSX_NewFrame(NSView* view)
{
    ImGui_ImplOSX_Data* bd = ImGui_ImplOSX_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplOSX_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size
    if (view)
    {
        const float dpi = (float)[view.window backingScaleFactor];
        io.DisplaySize = ImVec2((float)view.bounds.size.width, (float)view.bounds.size.height);
        io.DisplayFramebufferScale = ImVec2(dpi, dpi);
    }

    // Setup time step
    if (bd->Time == 0.0)
        bd->Time = GetMachAbsoluteTimeInSeconds();

    double current_time = GetMachAbsoluteTimeInSeconds();
    io.DeltaTime = (float)(current_time - bd->Time);
    bd->Time = current_time;

    ImGui_ImplOSX_UpdateMouseCursor();
    ImGui_ImplOSX_UpdateGamepads();
    ImGui_ImplOSX_UpdateImePosWithView(view);
}

// Must only be called for a mouse event, otherwise an exception occurs
// (Note that NSEventTypeScrollWheel is considered "other input". Oddly enough an exception does not occur with it, but the value will sometimes be wrong!)
static ImGuiMouseSource GetMouseSource(NSEvent* event)
{
    switch (event.subtype)
    {
        case NSEventSubtypeTabletPoint:
            return ImGuiMouseSource_Pen;
        // macOS considers input from relative touch devices (like the trackpad or Apple Magic Mouse) to be touch input.
        // This doesn't really make sense for Dear ImGui, which expects absolute touch devices only.
        // There does not seem to be a simple way to disambiguate things here so we consider NSEventSubtypeTouch events to always come from mice.
        // See https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/EventOverview/HandlingTouchEvents/HandlingTouchEvents.html#//apple_ref/doc/uid/10000060i-CH13-SW24
        //case NSEventSubtypeTouch:
        //    return ImGuiMouseSource_TouchScreen;
        case NSEventSubtypeMouseEvent:
        default:
            return ImGuiMouseSource_Mouse;
    }
}

static bool ImGui_ImplOSX_HandleEvent(NSEvent* event, NSView* view)
{
    ImGuiIO& io = ImGui::GetIO();

    if (event.type == NSEventTypeLeftMouseDown || event.type == NSEventTypeRightMouseDown || event.type == NSEventTypeOtherMouseDown)
    {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < ImGuiMouseButton_COUNT)
        {
            io.AddMouseSourceEvent(GetMouseSource(event));
            io.AddMouseButtonEvent(button, true);
        }
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeLeftMouseUp || event.type == NSEventTypeRightMouseUp || event.type == NSEventTypeOtherMouseUp)
    {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < ImGuiMouseButton_COUNT)
        {
            io.AddMouseSourceEvent(GetMouseSource(event));
            io.AddMouseButtonEvent(button, false);
        }
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeMouseMoved || event.type == NSEventTypeLeftMouseDragged || event.type == NSEventTypeRightMouseDragged || event.type == NSEventTypeOtherMouseDragged)
    {
        NSPoint mousePoint;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            mousePoint = NSEvent.mouseLocation;
            mousePoint.y = CGDisplayPixelsHigh(kCGDirectMainDisplay) - mousePoint.y; // Normalize y coordinate to top-left of main display.
        }
        else
        {
            mousePoint = event.locationInWindow;
            if (event.window == nil)
                mousePoint = [[view window] convertPointFromScreen:mousePoint];
            mousePoint = [view convertPoint:mousePoint fromView:nil]; // Convert to local coordinates of view
            if ([view isFlipped])
                mousePoint = NSMakePoint(mousePoint.x, mousePoint.y);
            else
                mousePoint = NSMakePoint(mousePoint.x, view.bounds.size.height - mousePoint.y);
        }
        io.AddMouseSourceEvent(GetMouseSource(event));
        io.AddMousePosEvent((float)mousePoint.x, (float)mousePoint.y);
        return io.WantCaptureMouse;
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
                wheel_dx *= 0.01;
                wheel_dy *= 0.01;
            }
        }
        else
        #endif // MAC_OS_X_VERSION_MAX_ALLOWED
        {
            wheel_dx = [event deltaX] * 0.1;
            wheel_dy = [event deltaY] * 0.1;
        }
        if (wheel_dx != 0.0 || wheel_dy != 0.0)
            io.AddMouseWheelEvent((float)wheel_dx, (float)wheel_dy);

        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeKeyDown || event.type == NSEventTypeKeyUp)
    {
        if ([event isARepeat])
            return io.WantCaptureKeyboard;

        int key_code = (int)[event keyCode];
        ImGuiKey key = ImGui_ImplOSX_KeyCodeToImGuiKey(key_code);
        io.AddKeyEvent(key, event.type == NSEventTypeKeyDown);
        io.SetKeyEventNativeData(key, key_code, -1); // To support legacy indexing (<1.87 user code)

        return io.WantCaptureKeyboard;
    }

    if (event.type == NSEventTypeFlagsChanged)
    {
        unsigned short key_code = [event keyCode];
        NSEventModifierFlags modifier_flags = [event modifierFlags];

        io.AddKeyEvent(ImGuiMod_Shift, (modifier_flags & NSEventModifierFlagShift)   != 0);
        io.AddKeyEvent(ImGuiMod_Ctrl,  (modifier_flags & NSEventModifierFlagControl) != 0);
        io.AddKeyEvent(ImGuiMod_Alt,   (modifier_flags & NSEventModifierFlagOption)  != 0);
        io.AddKeyEvent(ImGuiMod_Super, (modifier_flags & NSEventModifierFlagCommand) != 0);

        ImGuiKey key = ImGui_ImplOSX_KeyCodeToImGuiKey(key_code);
        if (key != ImGuiKey_None)
        {
            // macOS does not generate down/up event for modifiers. We're trying
            // to use hardware dependent masks to extract that information.
            // 'imgui_mask' is left as a fallback.
            NSEventModifierFlags mask = 0;
            switch (key)
            {
                case ImGuiKey_LeftCtrl:   mask = 0x0001; break;
                case ImGuiKey_RightCtrl:  mask = 0x2000; break;
                case ImGuiKey_LeftShift:  mask = 0x0002; break;
                case ImGuiKey_RightShift: mask = 0x0004; break;
                case ImGuiKey_LeftSuper:  mask = 0x0008; break;
                case ImGuiKey_RightSuper: mask = 0x0010; break;
                case ImGuiKey_LeftAlt:    mask = 0x0020; break;
                case ImGuiKey_RightAlt:   mask = 0x0040; break;
                default:
                    return io.WantCaptureKeyboard;
            }

            NSEventModifierFlags modifier_flags = [event modifierFlags];
            io.AddKeyEvent(key, (modifier_flags & mask) != 0);
            io.SetKeyEventNativeData(key, key_code, -1); // To support legacy indexing (<1.87 user code)
        }

        return io.WantCaptureKeyboard;
    }

    return false;
}

static void ImGui_ImplOSX_AddTrackingArea(NSView* _Nonnull view)
{
    // If we want to receive key events, we either need to be in the responder chain of the key view,
    // or else we can install a local monitor. The consequence of this heavy-handed approach is that
    // we receive events for all controls, not just Dear ImGui widgets. If we had native controls in our
    // window, we'd want to be much more careful than just ingesting the complete event stream.
    // To match the behavior of other backends, we pass every event down to the OS.
    ImGui_ImplOSX_Data* bd = ImGui_ImplOSX_GetBackendData();
    if (bd->Monitor)
        return;
    NSEventMask eventMask = 0;
    eventMask |= NSEventMaskMouseMoved | NSEventMaskScrollWheel;
    eventMask |= NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged;
    eventMask |= NSEventMaskRightMouseDown | NSEventMaskRightMouseUp | NSEventMaskRightMouseDragged;
    eventMask |= NSEventMaskOtherMouseDown | NSEventMaskOtherMouseUp | NSEventMaskOtherMouseDragged;
    eventMask |= NSEventMaskKeyDown | NSEventMaskKeyUp | NSEventMaskFlagsChanged;
    bd->Monitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask
                                                        handler:^NSEvent* _Nullable(NSEvent* event)
    {
        ImGui_ImplOSX_HandleEvent(event, view);
        return event;
    }];
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

@interface ImGui_ImplOSX_Window: NSWindow
@end

@implementation ImGui_ImplOSX_Window

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

@end

static void ConvertNSRect(NSRect* r)
{
    NSRect firstScreenFrame = NSScreen.screens[0].frame;
    IM_ASSERT(firstScreenFrame.origin.x == 0 && firstScreenFrame.origin.y == 0);
    r->origin.y = firstScreenFrame.size.height - r->origin.y - r->size.height;
}

static void ImGui_ImplOSX_CreateWindow(ImGuiViewport* viewport)
{
    ImGui_ImplOSX_Data* bd = ImGui_ImplOSX_GetBackendData();
    ImGuiViewportDataOSX* data = IM_NEW(ImGuiViewportDataOSX)();
    viewport->PlatformUserData = data;

    NSScreen* screen = bd->Window.screen;
    NSRect rect = NSMakeRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);
    ConvertNSRect(&rect);

    NSWindowStyleMask styleMask = 0;
    if (viewport->Flags & ImGuiViewportFlags_NoDecoration)
        styleMask |= NSWindowStyleMaskBorderless;
    else
        styleMask |= NSWindowStyleMaskTitled | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;

    NSWindow* window = [[ImGui_ImplOSX_Window alloc] initWithContentRect:rect
                                                               styleMask:styleMask
                                                                 backing:NSBackingStoreBuffered
                                                                   defer:YES
                                                                  screen:screen];
    if (viewport->Flags & ImGuiViewportFlags_TopMost)
        [window setLevel:NSFloatingWindowLevel];

    window.title = @"Untitled";
    window.opaque = YES;

    KeyEventResponder* view = [[KeyEventResponder alloc] initWithFrame:rect];
    if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_6 && ceil(NSAppKitVersionNumber) < NSAppKitVersionNumber10_15)
        [view setWantsBestResolutionOpenGLSurface:YES];

    window.contentView = view;

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
            window.contentView = nil;
            window.contentViewController = nil;
            [window orderOut:nil];
        }
        data->Window = nil;
        IM_DELETE(data);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = viewport->PlatformHandleRaw = nullptr;
}

static void ImGui_ImplOSX_ShowWindow(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
        [data->Window orderFront:nil];
    else
        [data->Window makeKeyAndOrderFront:nil];

    [data->Window setIsVisible:YES];
}

static ImVec2 ImGui_ImplOSX_GetWindowPos(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    NSWindow* window = data->Window;
    NSRect frame = window.frame;
    NSRect contentRect = window.contentLayoutRect;
    if (window.styleMask & NSWindowStyleMaskFullSizeContentView) // No title bar windows should be considered.
        contentRect = frame;

    NSRect firstScreenFrame = NSScreen.screens[0].frame;
    return ImVec2(frame.origin.x, firstScreenFrame.size.height - frame.origin.y - contentRect.size.height);
}

static void ImGui_ImplOSX_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    NSWindow* window = data->Window;
    NSSize size = window.frame.size;

    NSRect r = NSMakeRect(pos.x, pos.y, size.width, size.height);
    ConvertNSRect(&r);
    [window setFrameOrigin:r.origin];
}

static ImVec2 ImGui_ImplOSX_GetWindowSize(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    NSWindow* window = data->Window;
    NSSize size = window.contentLayoutRect.size;
    return ImVec2(size.width, size.height);
}

static void ImGui_ImplOSX_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    NSWindow* window = data->Window;
    NSRect rect = window.frame;
    rect.origin.y -= (size.y - rect.size.height);
    rect.size.width = size.x;
    rect.size.height = size.y;
    [window setFrame:rect display:YES];
}

static void ImGui_ImplOSX_SetWindowFocus(ImGuiViewport* viewport)
{
    ImGui_ImplOSX_Data* bd = ImGui_ImplOSX_GetBackendData();
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);
    [data->Window makeKeyAndOrderFront:bd->Window];
}

static bool ImGui_ImplOSX_GetWindowFocus(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    return data->Window.isKeyWindow;
}

static bool ImGui_ImplOSX_GetWindowMinimized(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    return data->Window.isMiniaturized;
}

static void ImGui_ImplOSX_SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    data->Window.title = [NSString stringWithUTF8String:title];
}

static void ImGui_ImplOSX_SetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);
    IM_ASSERT(alpha >= 0.0f && alpha <= 1.0f);

    data->Window.alphaValue = alpha;
}

static float ImGui_ImplOSX_GetWindowDpiScale(ImGuiViewport* viewport)
{
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)viewport->PlatformUserData;
    IM_ASSERT(data->Window != 0);

    return data->Window.backingScaleFactor;
}

static void ImGui_ImplOSX_UpdateMonitors()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Monitors.resize(0);

    NSRect firstScreenFrame = NSScreen.screens[0].frame;
    IM_ASSERT(firstScreenFrame.origin.x == 0 && firstScreenFrame.origin.y == 0);

    for (NSScreen* screen in NSScreen.screens)
    {
        NSRect frame = screen.frame;
        NSRect visibleFrame = screen.visibleFrame;
        ConvertNSRect(&frame);
        ConvertNSRect(&visibleFrame);

        ImGuiPlatformMonitor imgui_monitor;
        imgui_monitor.MainPos = ImVec2(frame.origin.x, frame.origin.y);
        imgui_monitor.MainSize = ImVec2(frame.size.width, frame.size.height);
        imgui_monitor.WorkPos = ImVec2(visibleFrame.origin.x, visibleFrame.origin.y);
        imgui_monitor.WorkSize = ImVec2(visibleFrame.size.width, visibleFrame.size.height);
        imgui_monitor.DpiScale = screen.backingScaleFactor;
        imgui_monitor.PlatformHandle = (__bridge_retained void*)screen;

        platform_io.Monitors.push_back(imgui_monitor);
    }
}

static void ImGui_ImplOSX_InitMultiViewportSupport()
{
    ImGui_ImplOSX_Data* bd = ImGui_ImplOSX_GetBackendData();

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
    platform_io.Platform_GetWindowDpiScale = ImGui_ImplOSX_GetWindowDpiScale; // FIXME-DPI

    // Register main window handle (which is owned by the main application, not by us)
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGuiViewportDataOSX* data = IM_NEW(ImGuiViewportDataOSX)();
    data->Window = bd->Window;
    data->WindowOwned = false;
    main_viewport->PlatformUserData = data;
    main_viewport->PlatformHandle = (__bridge void*)bd->Window;

    [NSNotificationCenter.defaultCenter addObserver:bd->Observer
                                           selector:@selector(displaysDidChange:)
                                               name:NSApplicationDidChangeScreenParametersNotification
                                             object:nil];
}

static void ImGui_ImplOSX_ShutdownMultiViewportSupport()
{
    ImGui_ImplOSX_Data* bd = ImGui_ImplOSX_GetBackendData();
    [NSNotificationCenter.defaultCenter removeObserver:bd->Observer
                                                  name:NSApplicationDidChangeScreenParametersNotification
                                                object:nil];
    bd->Observer = nullptr;
    bd->Window = nullptr;
    if (bd->Monitor != nullptr)
    {
        [NSEvent removeMonitor:bd->Monitor];
        bd->Monitor = nullptr;
    }

    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGuiViewportDataOSX* data = (ImGuiViewportDataOSX*)main_viewport->PlatformUserData;
    IM_DELETE(data);
    main_viewport->PlatformUserData = nullptr;
    ImGui::DestroyPlatformWindows();
}

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
