// dear imgui: Platform Backend for Xlib
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan..)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Mouse support.
//  [X] Platform: Keyboard support.
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
// Issues:
//  [ ] Platform: Missing touchscreen support.
//  [ ] Platform: Missing gamepad support.
//  [ ] Platform: Missing IME support.

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

#include <X11/Xlib.h>

IMGUI_IMPL_API bool     ImGui_ImplXlib_Init(Display* d, Window w);
IMGUI_IMPL_API void     ImGui_ImplXlib_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplXlib_NewFrame();
IMGUI_IMPL_API bool     ImGui_ImplXlib_ProcessEvent(XEvent* event);

struct ImGui_ImplXlib_Data
{
    Display*        Dpy;
    Window          Win;
    int             Xi2Opcode;
    XIM             IM;
    XIC             IC;
    timespec        Time;
    int             MouseButtonsDown;
    Cursor          MouseCursors[ImGuiMouseCursor_COUNT];
    Cursor          LastMouseCursor;
    char*           ClipboardTextData;
    bool            SelectionWaiting;
    bool            WantUpdateMonitors;
    Atom            XA_CLIPBOARD;
    Atom            XA_SELECTION;
    Atom            XA_TARGETS;
    Atom            XA_INCR;
    Atom*           XA_MIME;
    void*           RendererCtx;
    unsigned int    MimeCount;

    ImGui_ImplXlib_Data()   { memset((void*)this, 0, sizeof(*this)); }
};

ImGui_ImplXlib_Data* ImGui_ImplXlib_GetBackendData();
void ImGui_ImplXlib_InitMultiViewportSupport();

#endif // #ifndef IMGUI_DISABLE
