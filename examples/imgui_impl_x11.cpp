// dear imgui: Platform Binding for X11 using xcb
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan..)

// This works as is with Vulkan. For OpenGL using GLX, you need a hybrid
// of XLib and xcb using the X11/Xlib-xcb.h header. Use XLib to create the
// GLX context, then use functions in Xlib-xcb.h to convert the XLib
// structures to xcb, which you can then pass unmodified here.
// Requires libxcb, libxcb-xfixes, libxcb-xkb1 libxcb-cursor0, libxcb-keysyms1 and libxcb-randr0

// Implemented features:
//  [X] Platform: Keyboard arrays indexed using XK symbols, e.g. ImGui::IsKeyPressed(XK_space).
//  [X] Platform: Clipboard support
//  [X] Platform: Mouse cursor shape and visibility.
//  [/] Platform: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.
// Missing features:
//  [ ] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.

#include <stdio.h>
#include "imgui.h"
#include <stdlib.h>
#include <time.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xfixes.h>
#include <xcb/xcb_cursor.h>
#include <xcb/randr.h>

#define explicit c_explicit
#include <xcb/xkb.h>
#undef explicit

#ifdef HAS_VULKAN
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>
#include "imgui_impl_vulkan.h"
#endif

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
// 2020-08-24 Basic viewport implementation
// 2020-08-13 Clipboard support
// 2020-08-09 Full mouse cursor support
// 2020-07-30 Initial implementation

enum X11_Atom_
{
    X11_Atom_Clipboard,
    X11_Atom_Targets,
    X11_Atom_Dear_Imgui_Selection,
    X11_Atom_WM_Delete_Window,
    X11_Atom_WM_Protocols,
    X11_Atom_Net_Workarea,
    X11_Atom_Net_Supported,
    X11_Atom_Net_WM_Window_Type,
    X11_Atom_Net_WM_Window_Type_Toolbar,
    X11_Atom_Net_WM_Window_Type_Menu,
    X11_Atom_Net_WM_Window_Type_Dialog,
    X11_Atom_Motif_WM_Hints,
    X11_Atom_COUNT
};

static const char* g_X11AtomNames[X11_Atom_COUNT] = {
    "CLIPBOARD",
    "TARGETS",
    "DEAR_IMGUI_SELECTION",
    "WM_DELETE_WINDOW",
    "WM_PROTOCOLS",
    "_NET_WORKAREA",
    "_NET_SUPPORTED",
    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_WINDOW_TYPE_TOOLBAR",
    "_NET_WM_WINDOW_TYPE_MENU",
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_MOTIF_WM_HINTS"
};

enum Known_Target_
{
    Known_Target_UTF8_String,
    Known_Target_Compound_text,
    Known_Target_Text,
    Known_Target_String,
    Known_Target_Text_Plain_UTF8,
    Known_Target_Text_Plain,
    Known_Target_COUNT
};

static const char* g_KnownTargetNames[Known_Target_COUNT] = {
    "UTF8_STRING",
    "COMPOUND_TEXT",
    "TEXT",
    "STRING",
    "text/plain;charset=utf-8",
    "text/plain"
};

struct ImGuiViewportDataX11
{
    // Avoid excess querying with X by caching the window geometry. We update these values when we receive window events.
    ImVec2 Pos;
    ImVec2 Size;
    ImGuiViewportFlags Flags;

    ImGuiViewportDataX11() { Pos = ImVec2(0, 0); Size = ImVec2(0, 0); Flags = 0; }
    ~ImGuiViewportDataX11() { }
};

struct MotifHints
{
    uint32_t   Flags;
    uint32_t   Functions;
    uint32_t   Decorations;
    int32_t    InputMode;
    uint32_t   Status;
};

// X11 Data
static xcb_connection_t*      g_Connection;
static xcb_connection_t*      g_ClipboardConnection;
static xcb_window_t           g_MainWindow;
static xcb_key_symbols_t*     g_KeySyms;
static xcb_cursor_context_t*  g_CursorContext;
static xcb_screen_t*          g_Screen;
static xcb_window_t           g_ClipboardHandler;
static xcb_atom_t             g_X11Atoms[X11_Atom_COUNT];
static xcb_atom_t             g_KnownTargetAtoms[Known_Target_COUNT];

static timespec               g_LastTime;
static timespec               g_CurrentTime;

static bool                   g_HideXCursor = false;
static ImGuiMouseCursor       g_CurrentCursor = ImGuiMouseCursor_Arrow;
static char*                  g_CurrentClipboardData;

static const char* g_CursorMap[ImGuiMouseCursor_COUNT] = {
    "arrow",               // ImGuiMouseCursor_Arrow
    "xterm",               // ImGuiMouseCursor_TextInput
    "fleur",               // ImGuiMouseCursor_ResizeAll
    "sb_v_double_arrow",   // ImGuiMouseCursor_ResizeNS
    "sb_h_double_arrow",   // ImGuiMouseCursor_ResizeEW
    "bottom_left_corner",  // ImGuiMouseCursor_ResizeNESW
    "bottom_right_corner", // ImGuiMouseCursor_ResizeNWSE
    "hand1",               // ImGuiMouseCursor_Hand
    "circle"               // ImGuiMouseCursor_NotAllowed
};

// Forward Declarations
static void ImGui_ImplX11_SetClipboardText(void* user_data, const char* text);
static void ImGui_ImplX11_InitPlatformInterface();
static const char* ImGui_ImplX11_GetClipboardText(void *user_data);
static void ImGui_ImplX11_ProcessPendingSelectionEvents(bool wait_for_notify=false);
static void ImGui_ImplX11_ProcessViewportMessages();

// Functions
bool    ImGui_ImplX11_Init(xcb_window_t window)
{
    g_Connection = xcb_connect(nullptr, nullptr);
    g_Screen = xcb_setup_roots_iterator(xcb_get_setup(g_Connection)).data;
    g_MainWindow = window;
    xcb_generic_error_t* x_Err = nullptr;

    // Set initial clock value
    clock_gettime(CLOCK_MONOTONIC_RAW, &g_LastTime);

    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;      // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports; // We can create multi-viewports on the Platform side (optional)
    // io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
    io.BackendPlatformName = "imgui_impl_x11";

    // ~~~
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // ~~~
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGuiViewportDataX11* main_viewport_data = IM_NEW(ImGuiViewportDataX11);
    main_viewport->PlatformHandle = main_viewport->PlatformHandleRaw = (void *)(intptr_t)g_MainWindow;
    main_viewport->PlatformUserData = main_viewport_data;

    // Cache atoms we care about
    // Get the atoms for our known selection targets
    xcb_intern_atom_cookie_t target_atom_cookies[Known_Target_COUNT];
    for (int i = 0; i < Known_Target_COUNT; ++i)
        target_atom_cookies[i] = xcb_intern_atom(g_Connection, 0, strlen(g_KnownTargetNames[i]), g_KnownTargetNames[i]);

    for (int i = 0; i < Known_Target_COUNT; ++i)
    {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(g_Connection, target_atom_cookies[i], &x_Err);
        IM_ASSERT((reply && !x_Err) && "Error getting atom reply");
        g_KnownTargetAtoms[i] = reply->atom;
        free(reply);
    }

    // Get all other atoms we want
    xcb_intern_atom_cookie_t atom_cookies[X11_Atom_COUNT];
    for (int i = 0; i < X11_Atom_COUNT; ++i)
        atom_cookies[i] = xcb_intern_atom(g_Connection, 0, strlen(g_X11AtomNames[i]), g_X11AtomNames[i]);

    for (int i = 0; i < X11_Atom_COUNT; ++i)
    {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(g_Connection, atom_cookies[i], &x_Err);
        IM_ASSERT((reply && !x_Err) && "Error getting atom reply");
        g_X11Atoms[i] = reply->atom;
        free(reply);
    }

    // Set initial display size
    xcb_get_geometry_reply_t* resp = xcb_get_geometry_reply(g_Connection, xcb_get_geometry(g_Connection, g_MainWindow), &x_Err);
    IM_ASSERT(!x_Err && "X error querying window geometry");
    io.DisplaySize = ImVec2(resp->width, resp->height);
    main_viewport_data->Pos.x = resp->x;
    main_viewport_data->Pos.y = resp->y;
    main_viewport_data->Size.x = resp->width;
    main_viewport_data->Size.y = resp->height;
    free(resp);

    // Get the current key map
    g_KeySyms = xcb_key_symbols_alloc(g_Connection);

    // Turn off auto key repeat for this session
    // By default X does key repeat as down/up/down/up
    // Unfortunately unlike win32, X has no way of signaling the key event is a repeated key
    // So it's still possible to miss down/up inputs in the same frame
    xcb_xkb_use_extension_cookie_t extension_cookie = xcb_xkb_use_extension(g_Connection, 1, 0);
    xcb_xkb_use_extension_reply_t* extension_reply = xcb_xkb_use_extension_reply(g_Connection, extension_cookie, &x_Err);

    if (!x_Err && extension_reply->supported)
    {
        xcb_discard_reply(g_Connection,
            xcb_xkb_per_client_flags(g_Connection,
                        XCB_XKB_ID_USE_CORE_KBD,
                        XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
                        XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
                        0, 0, 0).sequence);
    }
    free(extension_reply);

    // Notify X for mouse cursor handling
    xcb_discard_reply(g_Connection, xcb_xfixes_query_version(g_Connection, 4, 0).sequence);

    // Cursor context for looking up cursors for the current X cursor theme
    xcb_cursor_context_new(g_Connection, g_Screen, &g_CursorContext);

    // Setting up selection communications
    // Create separate connection and window for handling pasting
    g_ClipboardConnection = xcb_connect(nullptr, nullptr);
    g_ClipboardHandler = xcb_generate_id(g_ClipboardConnection);
    xcb_screen_t *clipboard_screen = xcb_setup_roots_iterator(xcb_get_setup(g_ClipboardConnection)).data;
    xcb_create_window(g_ClipboardConnection, XCB_COPY_FROM_PARENT, g_ClipboardHandler,
                      clipboard_screen->root, 0, 0, 1, 1,
                      0, XCB_WINDOW_CLASS_INPUT_OUTPUT, clipboard_screen->root_visual,
                      0, nullptr);
    const char* clipboard_window_title = "Dear ImGui Requestor Window";
    xcb_change_property(g_ClipboardConnection, XCB_PROP_MODE_REPLACE,
                        g_ClipboardHandler, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                        strlen(clipboard_window_title), clipboard_window_title);

    // Zero initialize internal clipboard data
    g_CurrentClipboardData = strdup("");

    io.GetClipboardTextFn = ImGui_ImplX11_GetClipboardText;
    io.SetClipboardTextFn = ImGui_ImplX11_SetClipboardText;

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
    // X Keyboard non-latin syms have the top high bits set.
    // ImGui enforces the lookup values between 0..512.
    // Therefore we have to remove the high bits to pass this check.
    // There are some unusual key symbols in the 0xFE00 and 0xFD00 range.
    // If you really want to support those check for that in your own xcb event handler.
    // FIXME: Similar issue as OSX binding.
    io.KeyMap[ImGuiKey_Tab] = XK_Tab - 0xFF00;
    io.KeyMap[ImGuiKey_LeftArrow] = XK_Left - 0xFF00;
    io.KeyMap[ImGuiKey_RightArrow] = XK_Right - 0xFF00;
    io.KeyMap[ImGuiKey_UpArrow] = XK_Up - 0xFF00;
    io.KeyMap[ImGuiKey_DownArrow] = XK_Down - 0xFF00;
    io.KeyMap[ImGuiKey_PageUp] = XK_Page_Up - 0xFF00;
    io.KeyMap[ImGuiKey_PageDown] = XK_Page_Up - 0xFF00;
    io.KeyMap[ImGuiKey_Home] = XK_Home - 0xFF00;
    io.KeyMap[ImGuiKey_End] = XK_End - 0xFF00;
    io.KeyMap[ImGuiKey_Insert] = XK_Insert - 0xFF00;
    io.KeyMap[ImGuiKey_Delete] = XK_Delete - 0xFF00;
    io.KeyMap[ImGuiKey_Backspace] = XK_BackSpace - 0xFF00;
    io.KeyMap[ImGuiKey_Space] = XK_space;
    io.KeyMap[ImGuiKey_Enter] = XK_Return - 0xFF00;
    io.KeyMap[ImGuiKey_Escape] = XK_Escape - 0xFF00;
    io.KeyMap[ImGuiKey_KeyPadEnter] = XK_KP_Enter - 0xFF00;
    io.KeyMap[ImGuiKey_A] = XK_a;
    io.KeyMap[ImGuiKey_C] = XK_c;
    io.KeyMap[ImGuiKey_V] = XK_v;
    io.KeyMap[ImGuiKey_X] = XK_x;
    io.KeyMap[ImGuiKey_Y] = XK_y;
    io.KeyMap[ImGuiKey_Z] = XK_z;

    g_HideXCursor = io.MouseDrawCursor;

    ImGui_ImplX11_InitPlatformInterface();

    return true;
}

void    ImGui_ImplX11_Shutdown()
{
    xcb_destroy_window(g_ClipboardConnection, g_ClipboardHandler);
    xcb_cursor_context_free(g_CursorContext);
    xcb_key_symbols_free(g_KeySyms);
    xcb_flush(g_Connection);
    xcb_disconnect(g_ClipboardConnection);
}

void    ImGui_ImplX11_ChangeCursor(const char* name)
{
    xcb_font_t font = xcb_generate_id(g_Connection);
    // There is xcb_xfixes_cursor_change_cursor_by_name. However xcb_cursor_load_cursor guarantees
    // finding the cursor for the current X theme.
    xcb_cursor_t cursor = xcb_cursor_load_cursor(g_CursorContext, name);
    IM_ASSERT(cursor && "X cursor not found!");

    uint32_t value_list = cursor;
    xcb_change_window_attributes(g_Connection, g_MainWindow, XCB_CW_CURSOR, &value_list);
    xcb_free_cursor(g_Connection, cursor);
    xcb_close_font_checked(g_Connection, font);
}

void    ImGui_ImplX11_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (g_HideXCursor != io.MouseDrawCursor) // Change cursor if flag changed last frame
    {
        g_HideXCursor = io.MouseDrawCursor;
        if (g_HideXCursor)
            xcb_xfixes_hide_cursor(g_Connection, g_MainWindow);
        else
            xcb_xfixes_show_cursor(g_Connection, g_MainWindow);
    }

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (g_CurrentCursor != imgui_cursor)
    {
        g_CurrentCursor = imgui_cursor;
        ImGui_ImplX11_ChangeCursor(g_CursorMap[g_CurrentCursor]);
    }

    xcb_flush(g_Connection);
}

void    ImGui_ImplX11_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup time step
    clock_gettime(CLOCK_MONOTONIC_RAW, &g_CurrentTime);
    io.DeltaTime = (g_CurrentTime.tv_sec - g_LastTime.tv_sec) + ((g_CurrentTime.tv_nsec - g_LastTime.tv_nsec) / 1000000000.0f);
    g_LastTime = g_CurrentTime;

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        ImGui_ImplX11_ProcessViewportMessages();

    ImGui_ImplX11_UpdateMouseCursor();
    ImGui_ImplX11_ProcessPendingSelectionEvents();
}

// X11 xcb message handler (process X11 mouse/keyboard inputs, etc.)
// Call from your application's message handler. Returns true if
// ImGui processed the event. You can use this for checking if you
// need further processing for the event.
bool ImGui_ImplX11_ProcessEvent(xcb_generic_event_t* event)
{
    if (ImGui::GetCurrentContext() == NULL)
        return false;

    ImGuiIO& io = ImGui::GetIO();
    switch (event->response_type & ~0x80)
    {
    case XCB_MOTION_NOTIFY:
    {
        xcb_motion_notify_event_t* e = (xcb_motion_notify_event_t*)event;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            io.MousePos = ImVec2(e->root_x, e->root_y);
        else
            io.MousePos = ImVec2(e->event_x, e->event_y);

        return true;
    }
    case XCB_KEY_PRESS:
    {
        xcb_key_press_event_t* e = (xcb_key_press_event_t*)event;
        // since imgui processes modifiers internally by checking io.KeyCtrl and the like
        // we don't need any other xcb key columns besides the shift modifier.
        // without using the shift modifier we will only get the lower case letter
        // i think this may be an issue if both shift and key were pressed in the same frame?
        uint32_t col = io.KeyShift ? 1 : 0;
        xcb_keysym_t k = xcb_key_press_lookup_keysym(g_KeySyms, e, col);

        if (k < 0xFF) // latin-1 range
        {
            io.KeysDown[k] = 1;
            io.AddInputCharacter(k);
        }
        else if (k >= XK_Shift_L && k <= XK_Hyper_R) // modifier keys
        {
            switch(k)
            {
            case XK_Shift_L:
            case XK_Shift_R:
                io.KeyShift = true;
                break;
            case XK_Control_L:
            case XK_Control_R:
                io.KeyCtrl = true;
                break;
            case XK_Meta_L:
            case XK_Meta_R:
            case XK_Alt_L:
            case XK_Alt_R:
                io.KeyAlt = true;
                break;
            case XK_Super_L:
            case XK_Super_R:
                io.KeySuper = true;
                break;
            }
        }
        else if (k >= 0x1000100 && k <= 0x110ffff) // utf range
            io.AddInputCharacterUTF16(k);
        else
            io.KeysDown[k - 0xFF00] = 1;

        return true;
    }
    case XCB_KEY_RELEASE:
    {
        xcb_key_press_event_t* e = (xcb_key_press_event_t*)event;
        xcb_keysym_t k = xcb_key_press_lookup_keysym(g_KeySyms, e, 0);

        if (k < 0xff)
            io.KeysDown[k] = 0;
        else if (k >= XK_Shift_L && k <= XK_Hyper_R) // modifier keys
        {
            switch(k)
            {
            case XK_Shift_L:
            case XK_Shift_R:
                io.KeyShift = false;
                break;
            case XK_Control_L:
            case XK_Control_R:
                io.KeyCtrl = false;
                break;
            case XK_Meta_L:
            case XK_Meta_R:
            case XK_Alt_L:
            case XK_Alt_R:
                io.KeyAlt = false;
                break;
            case XK_Super_L:
            case XK_Super_R:
                io.KeySuper = false;
                break;
            }
        }
        else
            io.KeysDown[k - 0xFF00] = 0;

        return true;
    }
    case XCB_BUTTON_PRESS:
    {
        xcb_button_press_event_t* e = (xcb_button_press_event_t*)event;
        // Get exact coords of the event. It may be separate from the mouse cursor.
        // e.g. touch input
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            io.MousePos = ImVec2(e->root_x, e->root_y);
        else
            io.MousePos = ImVec2(e->event_x, e->event_y);

        // X decided button 4 is mwheel up and 5 is mwheel down
        if (e->detail >= 1 && e->detail <= 3)
            io.MouseDown[e->detail - 1] = true;
        else if (e->detail == 4)
            io.MouseWheel += 1.0;
        else if (e->detail == 5)
            io.MouseWheel -= 1.0;
        return true;
    }
    case XCB_BUTTON_RELEASE:
    {
        xcb_button_release_event_t* e = (xcb_button_release_event_t*)event;

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            io.MousePos = ImVec2(e->root_x, e->root_y);
        else
            io.MousePos = ImVec2(e->event_x, e->event_y);

        if (e->detail >= 1 && e->detail <= 3)
            io.MouseDown[e->detail - 1] = false;
        return true;
    }
    case XCB_ENTER_NOTIFY:
    {
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            // xcb_enter_notify_event_t* e = (xcb_enter_notify_event_t*)event;
            // ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void *)(intptr_t)e->event);
            // if(viewport)
            //     io.MouseHoveredViewport = viewport->ID;
        }
        if (g_HideXCursor)
        {
            xcb_xfixes_hide_cursor(g_Connection, g_MainWindow);
            // Doesn't actually hide the cursor until sending a flush or sending another command like xcb_request_check
            xcb_flush(g_Connection);
        }
        return true;
    }
    case XCB_LEAVE_NOTIFY:
    {
        xcb_xfixes_show_cursor(g_Connection, g_MainWindow);
        xcb_flush(g_Connection);
        return true;
    }
    case XCB_RANDR_SCREEN_CHANGE_NOTIFY: // TODO viewport reconfiguration on resolution change of a monitor
    {
        // xcb_randr_screen_change_notify_event_t *e = (xcb_randr_screen_change_notify_event_t*)event;
        return true;
    }
    case XCB_RANDR_NOTIFY: // TODO viewport reconfiguration on a monitor removal
    {
        xcb_randr_notify_event_t *e = (xcb_randr_notify_event_t*)event;
        if (e->subCode == XCB_RANDR_NOTIFY_OUTPUT_CHANGE && e->u.oc.connection == XCB_RANDR_CONNECTION_DISCONNECTED)
            return true;
    }
    case XCB_CLIENT_MESSAGE:
    {
        xcb_client_message_event_t *e = (xcb_client_message_event_t*)event;
        if (e->window != g_MainWindow && e->data.data32[0] == g_X11Atoms[X11_Atom_WM_Delete_Window])
        {
            ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void *)(intptr_t)e->window);
            IM_ASSERT(viewport && "Missing viewport for window!");
            viewport->PlatformRequestClose = true;
            return true;
        }
        break;
    }
    case XCB_CONFIGURE_NOTIFY:
    {
        xcb_configure_notify_event_t *e = (xcb_configure_notify_event_t*)event;
        ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void *)(intptr_t)e->window);
        if (viewport)
        {
            ImGuiViewportDataX11 *data = (ImGuiViewportDataX11*)viewport->PlatformUserData;
            // As far as I can tell we don't actually know what changed with the window. Perhaps X coalesces them into one event.
            if (data->Pos.x != e->x || data->Pos.y != e->y)
            {
                data->Pos.x = e->x;
                data->Pos.y = e->y;
                viewport->PlatformRequestMove = true;
            }
            if (data->Size.x != e->width || data->Size.y != e->height)
            {
                data->Size.x = e->width;
                data->Size.y = e->height;
                viewport->PlatformRequestResize = true;
            }
            return true;
        }
    }
    }
    return false;
}

void ImGui_ImplX11_ProcessViewportMessages()
{
    xcb_generic_event_t* event;
    while ((event = xcb_poll_for_event(g_Connection)))
    {
        ImGui_ImplX11_ProcessEvent(event);
        free(event);
    }
}

void ImGui_ImplX11_SetWindowDecoration(bool disabled, xcb_window_t window) // Disabled seems logically backwards, however we use the value of ImGuiViewportFlags_NoDecoration
{
    // There are two 'standards' for telling the WM what you want to do with a window, ICCCM and EWMH. Neither one of them has any specification for removing the window decorations.
    // EWMH has specification for window types which the WM interprets however it wants. The only consistent way is setting a very, very old property on the window
    // from the Motif window manager era. Every WM respects that property.
    MotifHints hints;

    hints.Flags = 2;
    hints.Functions = 0;
    hints.Decorations = !disabled;
    hints.InputMode = 0;
    hints.Status = 0;

    xcb_change_property(g_Connection, XCB_PROP_MODE_REPLACE, window,
                        g_X11Atoms[X11_Atom_Motif_WM_Hints], g_X11Atoms[X11_Atom_Motif_WM_Hints],
                        32, 5, &hints );
}

static void ImGui_ImplX11_CreateWindow(ImGuiViewport* viewport)
{
    xcb_window_t window = xcb_generate_id(g_Connection);
    viewport->PlatformHandle = viewport->PlatformHandleRaw = (void *)(intptr_t)window;
    viewport->PlatformUserData = IM_NEW(ImGuiViewportDataX11);
    ((ImGuiViewportDataX11*)viewport->PlatformUserData)->Flags = viewport->Flags;
    uint32_t values[2];

    values[0] = g_Screen->black_pixel;
    values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION |
        XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;

    xcb_create_window(g_Connection,
                      XCB_COPY_FROM_PARENT,
                      window,
                      g_Screen->root,
                      viewport->Pos.x, viewport->Pos.y,
                      viewport->Size.x, viewport->Size.y,
                      10,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      g_Screen->root_visual,
                      XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
                      values);

    // Tell the WM we want notifications when the window closes
    xcb_change_property(g_Connection, XCB_PROP_MODE_REPLACE, window,
                        g_X11Atoms[X11_Atom_WM_Protocols], 4, 32, 1, &g_X11Atoms[X11_Atom_WM_Delete_Window]);

    // Tell the WM this window is a child of the main window. The X11 concept of parent/child doesn't work as the WM frame becomes the parent of any window.
    // EWMH has a specification for a window as 'transient' to another.
    if (viewport->ParentViewportId != 0)
    {
        if (ImGuiViewport* parent_viewport = ImGui::FindViewportByID(viewport->ParentViewportId))
        {
            xcb_window_t parent_window = (xcb_window_t)(intptr_t)parent_viewport->PlatformHandle;
            xcb_change_property(g_Connection, XCB_PROP_MODE_REPLACE, parent_window,
                                XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 32, 1, &g_MainWindow);

        }
    }

    ImGui_ImplX11_SetWindowDecoration((viewport->Flags & ImGuiViewportFlags_NoDecoration), window);
}

static void Imgui_ImplX11_DestroyWindow(ImGuiViewport* viewport)
{
    xcb_window_t window = (xcb_window_t)(intptr_t)viewport->PlatformHandle;
    ImGuiViewportDataX11 *data = (ImGuiViewportDataX11*)viewport->PlatformUserData;
    IM_DELETE(data);
    xcb_destroy_window(g_Connection, window);
    xcb_flush(g_Connection);
    viewport->PlatformUserData = viewport->PlatformHandle = NULL;
}

static ImVec2 Imgui_ImplX11_GetWindowPos(ImGuiViewport* viewport)
{
    return ((ImGuiViewportDataX11*)viewport->PlatformUserData)->Pos;
}

static void ImGui_ImplX11_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    xcb_window_t window = (xcb_window_t)(intptr_t)viewport->PlatformHandle;
    ImGuiViewportDataX11 *data = (ImGuiViewportDataX11*)viewport->PlatformUserData;
    uint32_t dim[] = { (uint32_t)pos.x, (uint32_t)pos.y };
    xcb_configure_window(g_Connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, dim);
    data->Pos = pos;
}

static ImVec2 ImGui_ImplX11_GetWindowSize(ImGuiViewport* viewport)
{
    xcb_window_t window = (xcb_window_t)(intptr_t)viewport->PlatformHandle;
    xcb_generic_error_t* x_Err;
    xcb_get_geometry_reply_t* dim = xcb_get_geometry_reply(g_Connection,
        xcb_get_geometry(g_Connection, window), &x_Err);
    IM_ASSERT(!x_Err && "Error getting window geometry");
    if (!dim)
        return ImVec2(0, 0);
    return ImVec2(dim->width, dim->height);
}

static void ImGui_ImplX11_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    xcb_window_t window = (xcb_window_t)(intptr_t)viewport->PlatformHandle;
    uint32_t dim[] = { (uint32_t)size.x, (uint32_t)size.y };
    xcb_configure_window(g_Connection, window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, dim);
}

static void ImGui_ImplX11_SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    xcb_window_t window = (xcb_window_t)(intptr_t)viewport->PlatformHandle;
    xcb_change_property(g_Connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);
    return;
}

static void ImGui_ImplX11_ShowWindow(ImGuiViewport* viewport)
{
    xcb_window_t window = (xcb_window_t)(intptr_t)viewport->PlatformHandle;
    xcb_map_window(g_Connection, window);
    xcb_flush(g_Connection);
}

#ifdef HAS_VULKAN
static int ImGui_ImplX11_CreateVkSurface(ImGuiViewport* viewport, ImU64 vk_instance, const void* vk_allocator, ImU64* out_vk_surface)
{
    xcb_window_t window = (xcb_window_t)(intptr_t)viewport->PlatformHandle;
    VkXcbSurfaceCreateInfoKHR surface_create_info = { VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR };
    surface_create_info.connection = g_Connection;
    surface_create_info.window = window;
    VkResult err = vkCreateXcbSurfaceKHR((VkInstance)vk_instance, &surface_create_info, (VkAllocationCallbacks*)vk_allocator, (VkSurfaceKHR*)out_vk_surface);
    IM_ASSERT(err == 0 && "Error creating Xcb surface");
    return 0;
}
#endif

static void ImGui_ImplX11_UpdateWindow(ImGuiViewport* viewport)
{
    ImGuiViewportDataX11 *data = (ImGuiViewportDataX11*)viewport->PlatformUserData;
    ImGuiViewportFlags changes = data->Flags ^ viewport->Flags;
    if (changes)
    {
        xcb_window_t window = (xcb_window_t)(intptr_t)viewport->PlatformHandle;
        if (changes & ImGuiViewportFlags_NoDecoration)
        {
            ImGui_ImplX11_SetWindowDecoration((viewport->Flags & ImGuiViewportFlags_NoDecoration), window);
        }
        data->Flags = viewport->Flags;
    }
}

void ImGui_ImplX11_InitPlatformInterface()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow    = ImGui_ImplX11_CreateWindow;
    platform_io.Platform_DestroyWindow   = Imgui_ImplX11_DestroyWindow;
    platform_io.Platform_GetWindowPos    = Imgui_ImplX11_GetWindowPos;
    platform_io.Platform_SetWindowPos    = ImGui_ImplX11_SetWindowPos;
    platform_io.Platform_GetWindowSize   = ImGui_ImplX11_GetWindowSize;
    platform_io.Platform_SetWindowSize   = ImGui_ImplX11_SetWindowSize;
    platform_io.Platform_SetWindowTitle  = ImGui_ImplX11_SetWindowTitle;
    platform_io.Platform_ShowWindow      = ImGui_ImplX11_ShowWindow;
    platform_io.Platform_UpdateWindow    = ImGui_ImplX11_UpdateWindow;
#ifdef HAS_VULKAN
    platform_io.Platform_CreateVkSurface = ImGui_ImplX11_CreateVkSurface;
#endif

    xcb_generic_error_t* x_Err;

    xcb_randr_get_monitors_reply_t* monitors = xcb_randr_get_monitors_reply(g_Connection,
                                                xcb_randr_get_monitors(g_Connection, g_MainWindow, 1), &x_Err);
    IM_ASSERT(!x_Err && "Error requesting monitor layout");
    platform_io.Monitors.resize(0);

    // The _NET_WORKAREA property on the root window supposedly has the information we need about the usable screen area.
    // No platform library really get this right because some WMs refuse to implement the EWMH spec.
    // Some out there window managers like awesome and i3 don't adhere for various reasons.
    // Best we can do for right now is use it if we have it, otherwise set work area = main area which
    // will make the window cursor off center if there are fixed panels for those WMs.
    // Alternate way is get all the windows for a monitor, then subtract all the _WM_STRUT_PARTIAL properties from the main area.
    // awesomewm does the strut area subtraction for influcing placement of it's window management internally.
    // A CSG subtract with the combined strut partial areas could end up a non-square shape area which we are definitely not equipped for.
    // SDL tries to avoid all of this by using the old Xinerama interface which may still be incorrect on some WMs.
    // glfw does something similar to what we do here using Xlib, though it seems like it has bugs and gets it wrong occasionally.
    // The vast majority of the end users will be on Gnome or KDE which both set _NET_WORKAREA correct for at least a decade.
    // TODO(rory) net workarea has a tuple of length for (xpos, ypos, w, h) for each desktop. Need to check it's arrangement
    // for multple monitors or if we need to use the special root window properties for Gnome & KDE.
    // TODO(rory) this is a one and done, need to set up a property change event listner on all the root windows workarea so we can recalculate when it changes
    xcb_get_property_reply_t *property_reply = xcb_get_property_reply(g_Connection,
                                                                      xcb_get_property(g_Connection, false, g_Screen->root,
                                                                                       g_X11Atoms[X11_Atom_Net_Supported],
                                                                                       XCB_ATOM_ATOM, 0, UINT32_MAX - 1), &x_Err);
    IM_ASSERT(!x_Err);
    xcb_atom_t *atoms = (xcb_atom_t *)xcb_get_property_value(property_reply);
    bool found_net_supported = false;
    for (uint32_t i = 0; i < (xcb_get_property_value_length(property_reply) / sizeof(xcb_atom_t)); ++i)
    {
        if (atoms[i] == g_X11Atoms[X11_Atom_Net_Workarea])
        {
            found_net_supported = true;
            break;
        }
    }
    free(property_reply);

    uint32_t *work_areas = 0;
    if (found_net_supported)
    {
        xcb_get_property_reply_t *p_reply = xcb_get_property_reply(g_Connection,
                                                                  xcb_get_property(g_Connection, false, g_Screen->root,
                                                                  g_X11Atoms[X11_Atom_Net_Workarea], XCB_ATOM_CARDINAL,
                                                                  0, UINT32_MAX - 1), &x_Err);
        work_areas = (uint32_t *)xcb_get_property_value(p_reply);
        free(p_reply);
    }

    xcb_randr_monitor_info_iterator_t itr;
    // TODO(rory) translate coordinates when monitors rotated
    for (itr = xcb_randr_get_monitors_monitors_iterator(monitors); itr.rem; xcb_randr_monitor_info_next(&itr))
    {
        ImGuiPlatformMonitor monitor;
        monitor.MainPos = ImVec2(itr.data->x, itr.data->y);
        monitor.MainSize = ImVec2(itr.data->width, itr.data->height);
        monitor.WorkPos = monitor.MainPos;
        monitor.WorkSize = monitor.MainSize;

        if (found_net_supported)
        {
            monitor.WorkPos = ImVec2(itr.data->x + work_areas[0], itr.data->y + work_areas[1]);
            monitor.WorkSize = ImVec2(work_areas[2], work_areas[3]);
        }

        // Assuming square pixels.
        monitor.DpiScale = (itr.data->width / (itr.data->width_in_millimeters / 25.4)) / 96.0;
        platform_io.Monitors.push_back(monitor);
    }
    free(monitors);
}

// Clipboards are a strange thing compared to any other system. There isn't a concept of a global system clipboard.
// X has a concept of 'selections' instead. This is a segement of data 'selected' within a window. It is usually text though
// it can be any arbitrary data. The system identifies selections globally by name with atoms. There are two standard selection names
// in use today, CLIPBOARD and PRIMARY. The CLIPBOARD selection is a direct analog to the concept of copying and pasting. The PRIMARY
// selection is when a user highlights text in a window. The general protocol the middle mouse button 'pastes' the highlighted
// text in a different window. Window managers generally support highlighting and middle mouse button, though sporadically
// enabled by default as it's a strange concept for most people. Historically CLIPBOARD and PRIMARY were treated
// as independent, in practice most applications set the same data for both.
// For copying, instead of calling a function setting the system clipboard data like Win32 SetClipboardData, a window notifies
// X that it has ownership of CLIPBOARD and usually PRIMARY. This is a cooperative system. Whatever window last notified X
// is authoratative.
// When another window wants to paste from CLIPBOARD or PRIMARY, it asks X which window has current ownership of either.
// Then, the window wanting to paste dispatches a SelectionRequest event to the window owning the clipboard. See XCB_SELECTION_REQUEST below
// for processing that event. We process selection requests within the main loop.
// 'Pasting' from another window is called 'converting a selection'. Because ImGui wants the return value of the callback function
// returning the pasted data, we do the selection conversion calls and event handling from a separate hidden window. This is a common
// practice.

// We process all pending selection events every frame. We also check for selection events in GetClipboardText.
// Since GetClipboardText is a synchronous function and getting a selection happens sometime in the future, we need a signal
// for waiting until we get the selection back. wait_for_notify controls if this function blocks until the next
// selection notification event.
void ImGui_ImplX11_ProcessPendingSelectionEvents(bool wait_for_notify)
{
    xcb_generic_event_t* event = wait_for_notify? xcb_wait_for_event(g_ClipboardConnection) : xcb_poll_for_event(g_ClipboardConnection);

    bool found_notify_event = false;
    while (event)
    {
        switch (event->response_type & ~0x80)
        {
        case XCB_SELECTION_NOTIFY:
        {
            xcb_selection_notify_event_t* e = (xcb_selection_notify_event_t*)event;
            found_notify_event = true;
            if (e->property != XCB_NONE) // Other window said they don't handle UTF8_STRING
            {
                // Pull the data from the property we told the other window to set
                xcb_get_property_cookie_t get_property = xcb_get_property(g_ClipboardConnection, true, g_ClipboardHandler,
                                    g_X11Atoms[X11_Atom_Dear_Imgui_Selection], XCB_GET_PROPERTY_TYPE_ANY,
                                    0, UINT32_MAX - 1);
                xcb_get_property_reply_t *property_reply = xcb_get_property_reply(g_ClipboardConnection, get_property, nullptr);
                g_CurrentClipboardData = strdup((const char*)xcb_get_property_value(property_reply));
                free(property_reply);
            }
            break;
        }
        case XCB_SELECTION_REQUEST:
        {
            // The payload of a selection request event contains three necessary components:
            // the ID of the target window as the requestor, the name of the property we change
            // on the target window, and 'target'. You can think of 'target' as a MIME type.
            // We can discard selection requests for targets, or types, types we don't know about.
            // Currently ImGui only knows about text types specified in g_KnownTargetNames.
            // Responding to a selection request event is two steps.
            // First change an internal property of the requesting window with the requested data.
            // Then dispatch an event to the requesting window notifying that we set the data on it
            xcb_selection_request_event_t* e = (xcb_selection_request_event_t *)event;

            // If the 'target' of the request is TARGETS, the requesting window wants to know
            // what are the supported types. Other windows use this internally. For instance, if
            // a window wants audio data and the current selection owner doesn't say they have audio
            // data, then the requesting window usually won't ask for the selection again. Qt apps do this
            // query automatically. Also Qt apps won't ask for the selection again if it doesn't get a reply
            // from TARGETS.
            // Other apps might not care and will always send a selection request
            // every time, specifying what it wants in the 'target' part of the event.
            if (e->target == g_X11Atoms[X11_Atom_Targets])
            {
                xcb_change_property(g_ClipboardConnection, XCB_PROP_MODE_REPLACE,
                                    e->requestor, e->property,
                                    XCB_ATOM_ATOM, sizeof(xcb_atom_t) * 8,
                                    sizeof(xcb_atom_t) * Known_Target_COUNT,
                                    g_KnownTargetAtoms);
            }
            else
            {
                // Can we handle this target?
                bool is_known_target = false;
                for (int i = 0; i < Known_Target_COUNT; ++i)
                {
                    if (g_KnownTargetAtoms[i] == e->target)
                    {
                        is_known_target = true;
                        break;
                    }
                }
                if (is_known_target)
                    xcb_change_property(g_ClipboardConnection, XCB_PROP_MODE_REPLACE, e->requestor,
                                        e->property, e->target, 8,
                                        strlen(g_CurrentClipboardData), g_CurrentClipboardData);
            }

            // Then tell the requesting window we're finished what they asked for
            xcb_selection_notify_event_t notify_event = {};
            notify_event.response_type = XCB_SELECTION_NOTIFY;
            notify_event.time = XCB_CURRENT_TIME;
            notify_event.requestor = e->requestor;
            notify_event.selection = e->selection;
            notify_event.target = e->target;
            notify_event.property = e->property;
            xcb_generic_error_t *err = xcb_request_check(g_ClipboardConnection, xcb_send_event(g_ClipboardConnection, false,
                        e->requestor,
                        XCB_EVENT_MASK_PROPERTY_CHANGE,
                        (const char*)&notify_event));
            IM_ASSERT(!err && "Failed sending event");
            break;
        }
        }
        free(event);
        event = (wait_for_notify && !found_notify_event)? xcb_wait_for_event(g_ClipboardConnection) : xcb_poll_for_event(g_ClipboardConnection);
    }
}

const char* ImGui_ImplX11_GetClipboardText(void *user_data)
{
    xcb_generic_error_t *x_Err;
    xcb_atom_t current_selection_atom = g_X11Atoms[X11_Atom_Clipboard];
    // First try the clipboard selection
    xcb_get_selection_owner_reply_t *selection_reply = xcb_get_selection_owner_reply(g_ClipboardConnection,
                                                                                     xcb_get_selection_owner(g_ClipboardConnection, current_selection_atom),
                                                                                     &x_Err);

    IM_ASSERT(!x_Err && "Error getting selection reply");
    if (selection_reply->owner == 0) // If no clipboard selection owner try the primary selection
    {
        free(selection_reply);
        current_selection_atom = XCB_ATOM_PRIMARY;
        selection_reply = xcb_get_selection_owner_reply(g_ClipboardConnection,
                                            xcb_get_selection_owner(g_ClipboardConnection, current_selection_atom), &x_Err);
    }
    IM_ASSERT(!x_Err && "Error getting selection reply");

    // If there is a registered window for handling selections and we are not the current owner
    if (selection_reply->owner != 0 || selection_reply->owner != g_ClipboardHandler)
    {
        // Tell the current selection owner we want it's data as UTF8_STRING
        x_Err = xcb_request_check(g_ClipboardConnection,
                    xcb_convert_selection_checked(g_ClipboardConnection, g_ClipboardHandler,
                                                    current_selection_atom, g_KnownTargetAtoms[0],
                                                    g_X11Atoms[X11_Atom_Dear_Imgui_Selection], XCB_CURRENT_TIME));

        IM_ASSERT(!x_Err && "Error getting convert selection");
        ImGui_ImplX11_ProcessPendingSelectionEvents(true);
    }

    free(selection_reply);
    return g_CurrentClipboardData;
}

void    ImGui_ImplX11_SetClipboardText(void* user_data, const char* text)
{
    xcb_generic_error_t *x_Err;
    g_CurrentClipboardData = (char *)realloc(g_CurrentClipboardData, strlen(text));
    strcpy(g_CurrentClipboardData, text);
    // Notify that we now own the PRIMARY selection
    xcb_set_selection_owner(g_ClipboardConnection,
                            g_ClipboardHandler,
                            XCB_ATOM_PRIMARY,
                            XCB_CURRENT_TIME);
    xcb_xfixes_select_selection_input(g_ClipboardConnection,
                                      g_ClipboardHandler,
                                      XCB_ATOM_PRIMARY,
                                      XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER);
    xcb_discard_reply(g_ClipboardConnection,
        xcb_get_selection_owner_reply(g_ClipboardConnection, xcb_get_selection_owner(g_ClipboardConnection, XCB_ATOM_PRIMARY), &x_Err)->sequence);
    IM_ASSERT(!x_Err && "Error owning the primary selection");

    // And notify that we now own the CLIPBOARD selection
    xcb_set_selection_owner(g_ClipboardConnection,
                            g_ClipboardHandler,
                            g_X11Atoms[X11_Atom_Clipboard],
                            XCB_CURRENT_TIME);
    xcb_xfixes_select_selection_input(g_ClipboardConnection,
                                      g_ClipboardHandler,
                                      g_X11Atoms[X11_Atom_Clipboard],
                                      XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER);
    xcb_discard_reply(g_ClipboardConnection,
        xcb_get_selection_owner_reply(g_ClipboardConnection, xcb_get_selection_owner(g_ClipboardConnection, g_X11Atoms[X11_Atom_Clipboard]), &x_Err)->sequence);
    IM_ASSERT(!x_Err && "Error owning the clipboard selection");

    xcb_flush(g_ClipboardConnection);
}
