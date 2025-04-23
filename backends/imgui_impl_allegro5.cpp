// dear imgui: Renderer + Platform Backend for Allegro 5
// (Info: Allegro 5 is a cross-platform general purpose library for handling windows, inputs, graphics, etc.)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'ALLEGRO_BITMAP*' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy ALLEGRO_KEY_* values are obsolete since 1.87 and not supported since 1.91.5]
//  [X] Platform: Clipboard support (from Allegro 5.1.12).
//  [X] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors). Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
// Missing features or Issues:
//  [ ] Renderer: The renderer is suboptimal as we need to convert vertices manually.
//  [ ] Platform: Missing gamepad support.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2025-02-18: Added ImGuiMouseCursor_Wait and ImGuiMouseCursor_Progress mouse cursor support.
//  2025-01-06: Avoid calling al_set_mouse_cursor() repeatedly since it appears to leak on on X11 (#8256).
//  2024-08-22: moved some OS/backend related function pointers from ImGuiIO to ImGuiPlatformIO:
//               - io.GetClipboardTextFn    -> platform_io.Platform_GetClipboardTextFn
//               - io.SetClipboardTextFn    -> platform_io.Platform_SetClipboardTextFn
//  2022-11-30: Renderer: Restoring using al_draw_indexed_prim() when Allegro version is >= 5.2.5.
//  2022-10-11: Using 'nullptr' instead of 'NULL' as per our switch to C++11.
//  2022-09-26: Inputs: Renamed ImGuiKey_ModXXX introduced in 1.87 to ImGuiMod_XXX (old names still supported).
//  2022-01-26: Inputs: replaced short-lived io.AddKeyModsEvent() (added two weeks ago) with io.AddKeyEvent() using ImGuiKey_ModXXX flags. Sorry for the confusion.
//  2022-01-17: Inputs: calling new io.AddMousePosEvent(), io.AddMouseButtonEvent(), io.AddMouseWheelEvent() API (1.87+).
//  2022-01-17: Inputs: always calling io.AddKeyModsEvent() next and before key event (not in NewFrame) to fix input queue with very low framerates.
//  2022-01-10: Inputs: calling new io.AddKeyEvent(), io.AddKeyModsEvent() + io.SetKeyEventNativeData() API (1.87+). Support for full ImGuiKey range.
//  2021-12-08: Renderer: Fixed mishandling of the ImDrawCmd::IdxOffset field! This is an old bug but it never had an effect until some internal rendering changes in 1.86.
//  2021-08-17: Calling io.AddFocusEvent() on ALLEGRO_EVENT_DISPLAY_SWITCH_OUT/ALLEGRO_EVENT_DISPLAY_SWITCH_IN events.
//  2021-06-29: Reorganized backend to pull data from a single structure to facilitate usage with multiple-contexts (all g_XXXX access changed to bd->XXXX).
//  2021-05-19: Renderer: Replaced direct access to ImDrawCmd::TextureId with a call to ImDrawCmd::GetTexID(). (will become a requirement)
//  2021-02-18: Change blending equation to preserve alpha in output buffer.
//  2020-08-10: Inputs: Fixed horizontal mouse wheel direction.
//  2019-12-05: Inputs: Added support for ImGuiMouseCursor_NotAllowed mouse cursor.
//  2019-07-21: Inputs: Added mapping for ImGuiKey_KeyPadEnter.
//  2019-05-11: Inputs: Don't filter character value from ALLEGRO_EVENT_KEY_CHAR before calling AddInputCharacter().
//  2019-04-30: Renderer: Added support for special ImDrawCallback_ResetRenderState callback to reset render state.
//  2018-11-30: Platform: Added touchscreen support.
//  2018-11-30: Misc: Setting up io.BackendPlatformName/io.BackendRendererName so they can be displayed in the About Window.
//  2018-06-13: Platform: Added clipboard support (from Allegro 5.1.12).
//  2018-06-13: Renderer: Use draw_data->DisplayPos and draw_data->DisplaySize to setup projection matrix and clipping rectangle.
//  2018-06-13: Renderer: Stopped using al_draw_indexed_prim() as it is buggy in Allegro's DX9 backend.
//  2018-06-13: Renderer: Backup/restore transform and clipping rectangle.
//  2018-06-11: Misc: Setup io.BackendFlags ImGuiBackendFlags_HasMouseCursors flag + honor ImGuiConfigFlags_NoMouseCursorChange flag.
//  2018-04-18: Misc: Renamed file from imgui_impl_a5.cpp to imgui_impl_allegro5.cpp.
//  2018-04-18: Misc: Added support for 32-bit vertex indices to avoid conversion at runtime. Added imconfig_allegro5.h to enforce 32-bit indices when included from imgui.h.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback and exposed ImGui_ImplAllegro5_RenderDrawData() in the .h file so you can call it yourself.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_allegro5.h"
#include <stdint.h>     // uint64_t
#include <cstring>      // memcpy

// Allegro
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#ifdef _WIN32
#include <allegro5/allegro_windows.h>
#endif
#define ALLEGRO_HAS_CLIPBOARD           ((ALLEGRO_VERSION_INT & ~ALLEGRO_UNSTABLE_BIT) >= ((5 << 24) | (1 << 16) | (12 << 8))) // Clipboard only supported from Allegro 5.1.12
#define ALLEGRO_HAS_DRAW_INDEXED_PRIM   ((ALLEGRO_VERSION_INT & ~ALLEGRO_UNSTABLE_BIT) >= ((5 << 24) | (2 << 16) | ( 5 << 8))) // DX9 implementation of al_draw_indexed_prim() got fixed in Allegro 5.2.5

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127) // condition expression is constant
#endif

struct ImDrawVertAllegro
{
    ImVec2          pos;
    ImVec2          uv;
    ALLEGRO_COLOR   col;
};

// FIXME-OPT: Unfortunately Allegro doesn't support 32-bit packed colors so we have to convert them to 4 float as well..
// FIXME-OPT: Consider inlining al_map_rgba()?
// see https://github.com/liballeg/allegro5/blob/master/src/pixels.c#L554
// and https://github.com/liballeg/allegro5/blob/master/include/allegro5/internal/aintern_pixels.h
#define DRAW_VERT_IMGUI_TO_ALLEGRO(DST, SRC)  { (DST)->pos = (SRC)->pos; (DST)->uv = (SRC)->uv; unsigned char* c = (unsigned char*)&(SRC)->col; (DST)->col = al_map_rgba(c[0], c[1], c[2], c[3]); }

// Allegro Data
struct ImGui_ImplAllegro5_Data
{
    ALLEGRO_DISPLAY*            Display;
    ALLEGRO_BITMAP*             Texture;
    double                      Time;
    ALLEGRO_MOUSE_CURSOR*       MouseCursorInvisible;
    ALLEGRO_VERTEX_DECL*        VertexDecl;
    char*                       ClipboardTextData;
    ImGuiMouseCursor            LastCursor;

    ImVector<ImDrawVertAllegro> BufVertices;
    ImVector<int>               BufIndices;

    ImGui_ImplAllegro5_Data()   { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
static ImGui_ImplAllegro5_Data* ImGui_ImplAllegro5_GetBackendData()     { return ImGui::GetCurrentContext() ? (ImGui_ImplAllegro5_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr; }

static void ImGui_ImplAllegro5_SetupRenderState(ImDrawData* draw_data)
{
    // Setup blending
    al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

    // Setup orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    {
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        ALLEGRO_TRANSFORM transform;
        al_identity_transform(&transform);
        al_use_transform(&transform);
        al_orthographic_transform(&transform, L, T, 1.0f, R, B, -1.0f);
        al_use_projection_transform(&transform);
    }
}

// Render function.
void ImGui_ImplAllegro5_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    // Backup Allegro state that will be modified
    ImGui_ImplAllegro5_Data* bd = ImGui_ImplAllegro5_GetBackendData();
    ALLEGRO_TRANSFORM last_transform = *al_get_current_transform();
    ALLEGRO_TRANSFORM last_projection_transform = *al_get_current_projection_transform();
    int last_clip_x, last_clip_y, last_clip_w, last_clip_h;
    al_get_clipping_rectangle(&last_clip_x, &last_clip_y, &last_clip_w, &last_clip_h);
    int last_blender_op, last_blender_src, last_blender_dst;
    al_get_blender(&last_blender_op, &last_blender_src, &last_blender_dst);

    // Setup desired render state
    ImGui_ImplAllegro5_SetupRenderState(draw_data);

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* draw_list = draw_data->CmdLists[n];

        ImVector<ImDrawVertAllegro>& vertices = bd->BufVertices;
#if ALLEGRO_HAS_DRAW_INDEXED_PRIM
        vertices.resize(draw_list->VtxBuffer.Size);
        for (int i = 0; i < draw_list->VtxBuffer.Size; i++)
        {
            const ImDrawVert* src_v = &draw_list->VtxBuffer[i];
            ImDrawVertAllegro* dst_v = &vertices[i];
            DRAW_VERT_IMGUI_TO_ALLEGRO(dst_v, src_v);
        }
        const int* indices = nullptr;
        if (sizeof(ImDrawIdx) == 2)
        {
            // FIXME-OPT: Allegro doesn't support 16-bit indices.
            // You can '#define ImDrawIdx int' in imconfig.h to request Dear ImGui to output 32-bit indices.
            // Otherwise, we convert them from 16-bit to 32-bit at runtime here, which works perfectly but is a little wasteful.
            bd->BufIndices.resize(draw_list->IdxBuffer.Size);
            for (int i = 0; i < draw_list->IdxBuffer.Size; ++i)
                bd->BufIndices[i] = (int)draw_list->IdxBuffer.Data[i];
            indices = bd->BufIndices.Data;
        }
        else if (sizeof(ImDrawIdx) == 4)
        {
            indices = (const int*)draw_list->IdxBuffer.Data;
        }
#else
        // Allegro's implementation of al_draw_indexed_prim() for DX9 was broken until 5.2.5. Unindex buffers ourselves while converting vertex format.
        vertices.resize(draw_list->IdxBuffer.Size);
        for (int i = 0; i < draw_list->IdxBuffer.Size; i++)
        {
            const ImDrawVert* src_v = &draw_list->VtxBuffer[draw_list->IdxBuffer[i]];
            ImDrawVertAllegro* dst_v = &vertices[i];
            DRAW_VERT_IMGUI_TO_ALLEGRO(dst_v, src_v);
        }
#endif

        // Render command lists
        ImVec2 clip_off = draw_data->DisplayPos;
        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplAllegro5_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(draw_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
                ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle, Draw
                ALLEGRO_BITMAP* texture = (ALLEGRO_BITMAP*)pcmd->GetTexID();
                al_set_clipping_rectangle(clip_min.x, clip_min.y, clip_max.x - clip_min.x, clip_max.y - clip_min.y);
#if ALLEGRO_HAS_DRAW_INDEXED_PRIM
                al_draw_indexed_prim(&vertices[0], bd->VertexDecl, texture, &indices[pcmd->IdxOffset], pcmd->ElemCount, ALLEGRO_PRIM_TRIANGLE_LIST);
#else
                al_draw_prim(&vertices[0], bd->VertexDecl, texture, pcmd->IdxOffset, pcmd->IdxOffset + pcmd->ElemCount, ALLEGRO_PRIM_TRIANGLE_LIST);
#endif
            }
        }
    }

    // Restore modified Allegro state
    al_set_blender(last_blender_op, last_blender_src, last_blender_dst);
    al_set_clipping_rectangle(last_clip_x, last_clip_y, last_clip_w, last_clip_h);
    al_use_transform(&last_transform);
    al_use_projection_transform(&last_projection_transform);
}

bool ImGui_ImplAllegro5_CreateDeviceObjects()
{
    // Build texture atlas
    ImGui_ImplAllegro5_Data* bd = ImGui_ImplAllegro5_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create texture
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
    int flags = al_get_new_bitmap_flags();
    int fmt = al_get_new_bitmap_format();
    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
    al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
    ALLEGRO_BITMAP* img = al_create_bitmap(width, height);
    al_set_new_bitmap_flags(flags);
    al_set_new_bitmap_format(fmt);
    if (!img)
        return false;

    ALLEGRO_LOCKED_REGION* locked_img = al_lock_bitmap(img, al_get_bitmap_format(img), ALLEGRO_LOCK_WRITEONLY);
    if (!locked_img)
    {
        al_destroy_bitmap(img);
        return false;
    }
    memcpy(locked_img->data, pixels, sizeof(int) * width * height);
    al_unlock_bitmap(img);

    // Convert software texture to hardware texture.
    ALLEGRO_BITMAP* cloned_img = al_clone_bitmap(img);
    al_destroy_bitmap(img);
    if (!cloned_img)
        return false;

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)cloned_img);
    bd->Texture = cloned_img;

    // Create an invisible mouse cursor
    // Because al_hide_mouse_cursor() seems to mess up with the actual inputs..
    ALLEGRO_BITMAP* mouse_cursor = al_create_bitmap(8, 8);
    bd->MouseCursorInvisible = al_create_mouse_cursor(mouse_cursor, 0, 0);
    al_destroy_bitmap(mouse_cursor);

    return true;
}

void ImGui_ImplAllegro5_InvalidateDeviceObjects()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplAllegro5_Data* bd = ImGui_ImplAllegro5_GetBackendData();
    if (bd->Texture)
    {
        io.Fonts->SetTexID(0);
        al_destroy_bitmap(bd->Texture);
        bd->Texture = nullptr;
    }
    if (bd->MouseCursorInvisible)
    {
        al_destroy_mouse_cursor(bd->MouseCursorInvisible);
        bd->MouseCursorInvisible = nullptr;
    }
}

#if ALLEGRO_HAS_CLIPBOARD
static const char* ImGui_ImplAllegro5_GetClipboardText(ImGuiContext*)
{
    ImGui_ImplAllegro5_Data* bd = ImGui_ImplAllegro5_GetBackendData();
    if (bd->ClipboardTextData)
        al_free(bd->ClipboardTextData);
    bd->ClipboardTextData = al_get_clipboard_text(bd->Display);
    return bd->ClipboardTextData;
}

static void ImGui_ImplAllegro5_SetClipboardText(ImGuiContext*, const char* text)
{
    ImGui_ImplAllegro5_Data* bd = ImGui_ImplAllegro5_GetBackendData();
    al_set_clipboard_text(bd->Display, text);
}
#endif

// Not static to allow third-party code to use that if they want to (but undocumented)
ImGuiKey ImGui_ImplAllegro5_KeyCodeToImGuiKey(int key_code);
ImGuiKey ImGui_ImplAllegro5_KeyCodeToImGuiKey(int key_code)
{
    switch (key_code)
    {
        case ALLEGRO_KEY_TAB: return ImGuiKey_Tab;
        case ALLEGRO_KEY_LEFT: return ImGuiKey_LeftArrow;
        case ALLEGRO_KEY_RIGHT: return ImGuiKey_RightArrow;
        case ALLEGRO_KEY_UP: return ImGuiKey_UpArrow;
        case ALLEGRO_KEY_DOWN: return ImGuiKey_DownArrow;
        case ALLEGRO_KEY_PGUP: return ImGuiKey_PageUp;
        case ALLEGRO_KEY_PGDN: return ImGuiKey_PageDown;
        case ALLEGRO_KEY_HOME: return ImGuiKey_Home;
        case ALLEGRO_KEY_END: return ImGuiKey_End;
        case ALLEGRO_KEY_INSERT: return ImGuiKey_Insert;
        case ALLEGRO_KEY_DELETE: return ImGuiKey_Delete;
        case ALLEGRO_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case ALLEGRO_KEY_SPACE: return ImGuiKey_Space;
        case ALLEGRO_KEY_ENTER: return ImGuiKey_Enter;
        case ALLEGRO_KEY_ESCAPE: return ImGuiKey_Escape;
        case ALLEGRO_KEY_QUOTE: return ImGuiKey_Apostrophe;
        case ALLEGRO_KEY_COMMA: return ImGuiKey_Comma;
        case ALLEGRO_KEY_MINUS: return ImGuiKey_Minus;
        case ALLEGRO_KEY_FULLSTOP: return ImGuiKey_Period;
        case ALLEGRO_KEY_SLASH: return ImGuiKey_Slash;
        case ALLEGRO_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case ALLEGRO_KEY_EQUALS: return ImGuiKey_Equal;
        case ALLEGRO_KEY_OPENBRACE: return ImGuiKey_LeftBracket;
        case ALLEGRO_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case ALLEGRO_KEY_CLOSEBRACE: return ImGuiKey_RightBracket;
        case ALLEGRO_KEY_TILDE: return ImGuiKey_GraveAccent;
        case ALLEGRO_KEY_CAPSLOCK: return ImGuiKey_CapsLock;
        case ALLEGRO_KEY_SCROLLLOCK: return ImGuiKey_ScrollLock;
        case ALLEGRO_KEY_NUMLOCK: return ImGuiKey_NumLock;
        case ALLEGRO_KEY_PRINTSCREEN: return ImGuiKey_PrintScreen;
        case ALLEGRO_KEY_PAUSE: return ImGuiKey_Pause;
        case ALLEGRO_KEY_PAD_0: return ImGuiKey_Keypad0;
        case ALLEGRO_KEY_PAD_1: return ImGuiKey_Keypad1;
        case ALLEGRO_KEY_PAD_2: return ImGuiKey_Keypad2;
        case ALLEGRO_KEY_PAD_3: return ImGuiKey_Keypad3;
        case ALLEGRO_KEY_PAD_4: return ImGuiKey_Keypad4;
        case ALLEGRO_KEY_PAD_5: return ImGuiKey_Keypad5;
        case ALLEGRO_KEY_PAD_6: return ImGuiKey_Keypad6;
        case ALLEGRO_KEY_PAD_7: return ImGuiKey_Keypad7;
        case ALLEGRO_KEY_PAD_8: return ImGuiKey_Keypad8;
        case ALLEGRO_KEY_PAD_9: return ImGuiKey_Keypad9;
        case ALLEGRO_KEY_PAD_DELETE: return ImGuiKey_KeypadDecimal;
        case ALLEGRO_KEY_PAD_SLASH: return ImGuiKey_KeypadDivide;
        case ALLEGRO_KEY_PAD_ASTERISK: return ImGuiKey_KeypadMultiply;
        case ALLEGRO_KEY_PAD_MINUS: return ImGuiKey_KeypadSubtract;
        case ALLEGRO_KEY_PAD_PLUS: return ImGuiKey_KeypadAdd;
        case ALLEGRO_KEY_PAD_ENTER: return ImGuiKey_KeypadEnter;
        case ALLEGRO_KEY_PAD_EQUALS: return ImGuiKey_KeypadEqual;
        case ALLEGRO_KEY_LCTRL: return ImGuiKey_LeftCtrl;
        case ALLEGRO_KEY_LSHIFT: return ImGuiKey_LeftShift;
        case ALLEGRO_KEY_ALT: return ImGuiKey_LeftAlt;
        case ALLEGRO_KEY_LWIN: return ImGuiKey_LeftSuper;
        case ALLEGRO_KEY_RCTRL: return ImGuiKey_RightCtrl;
        case ALLEGRO_KEY_RSHIFT: return ImGuiKey_RightShift;
        case ALLEGRO_KEY_ALTGR: return ImGuiKey_RightAlt;
        case ALLEGRO_KEY_RWIN: return ImGuiKey_RightSuper;
        case ALLEGRO_KEY_MENU: return ImGuiKey_Menu;
        case ALLEGRO_KEY_0: return ImGuiKey_0;
        case ALLEGRO_KEY_1: return ImGuiKey_1;
        case ALLEGRO_KEY_2: return ImGuiKey_2;
        case ALLEGRO_KEY_3: return ImGuiKey_3;
        case ALLEGRO_KEY_4: return ImGuiKey_4;
        case ALLEGRO_KEY_5: return ImGuiKey_5;
        case ALLEGRO_KEY_6: return ImGuiKey_6;
        case ALLEGRO_KEY_7: return ImGuiKey_7;
        case ALLEGRO_KEY_8: return ImGuiKey_8;
        case ALLEGRO_KEY_9: return ImGuiKey_9;
        case ALLEGRO_KEY_A: return ImGuiKey_A;
        case ALLEGRO_KEY_B: return ImGuiKey_B;
        case ALLEGRO_KEY_C: return ImGuiKey_C;
        case ALLEGRO_KEY_D: return ImGuiKey_D;
        case ALLEGRO_KEY_E: return ImGuiKey_E;
        case ALLEGRO_KEY_F: return ImGuiKey_F;
        case ALLEGRO_KEY_G: return ImGuiKey_G;
        case ALLEGRO_KEY_H: return ImGuiKey_H;
        case ALLEGRO_KEY_I: return ImGuiKey_I;
        case ALLEGRO_KEY_J: return ImGuiKey_J;
        case ALLEGRO_KEY_K: return ImGuiKey_K;
        case ALLEGRO_KEY_L: return ImGuiKey_L;
        case ALLEGRO_KEY_M: return ImGuiKey_M;
        case ALLEGRO_KEY_N: return ImGuiKey_N;
        case ALLEGRO_KEY_O: return ImGuiKey_O;
        case ALLEGRO_KEY_P: return ImGuiKey_P;
        case ALLEGRO_KEY_Q: return ImGuiKey_Q;
        case ALLEGRO_KEY_R: return ImGuiKey_R;
        case ALLEGRO_KEY_S: return ImGuiKey_S;
        case ALLEGRO_KEY_T: return ImGuiKey_T;
        case ALLEGRO_KEY_U: return ImGuiKey_U;
        case ALLEGRO_KEY_V: return ImGuiKey_V;
        case ALLEGRO_KEY_W: return ImGuiKey_W;
        case ALLEGRO_KEY_X: return ImGuiKey_X;
        case ALLEGRO_KEY_Y: return ImGuiKey_Y;
        case ALLEGRO_KEY_Z: return ImGuiKey_Z;
        case ALLEGRO_KEY_F1: return ImGuiKey_F1;
        case ALLEGRO_KEY_F2: return ImGuiKey_F2;
        case ALLEGRO_KEY_F3: return ImGuiKey_F3;
        case ALLEGRO_KEY_F4: return ImGuiKey_F4;
        case ALLEGRO_KEY_F5: return ImGuiKey_F5;
        case ALLEGRO_KEY_F6: return ImGuiKey_F6;
        case ALLEGRO_KEY_F7: return ImGuiKey_F7;
        case ALLEGRO_KEY_F8: return ImGuiKey_F8;
        case ALLEGRO_KEY_F9: return ImGuiKey_F9;
        case ALLEGRO_KEY_F10: return ImGuiKey_F10;
        case ALLEGRO_KEY_F11: return ImGuiKey_F11;
        case ALLEGRO_KEY_F12: return ImGuiKey_F12;
        default: return ImGuiKey_None;
    }
}

bool ImGui_ImplAllegro5_Init(ALLEGRO_DISPLAY* display)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    // Setup backend capabilities flags
    ImGui_ImplAllegro5_Data* bd = IM_NEW(ImGui_ImplAllegro5_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = io.BackendRendererName = "imgui_impl_allegro5";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;       // We can honor GetMouseCursor() values (optional)

    bd->Display = display;
    bd->LastCursor = ALLEGRO_SYSTEM_MOUSE_CURSOR_NONE;

    // Create custom vertex declaration.
    // Unfortunately Allegro doesn't support 32-bit packed colors so we have to convert them to 4 floats.
    // We still use a custom declaration to use 'ALLEGRO_PRIM_TEX_COORD' instead of 'ALLEGRO_PRIM_TEX_COORD_PIXEL' else we can't do a reliable conversion.
    ALLEGRO_VERTEX_ELEMENT elems[] =
    {
        { ALLEGRO_PRIM_POSITION, ALLEGRO_PRIM_FLOAT_2, offsetof(ImDrawVertAllegro, pos) },
        { ALLEGRO_PRIM_TEX_COORD, ALLEGRO_PRIM_FLOAT_2, offsetof(ImDrawVertAllegro, uv) },
        { ALLEGRO_PRIM_COLOR_ATTR, 0, offsetof(ImDrawVertAllegro, col) },
        { 0, 0, 0 }
    };
    bd->VertexDecl = al_create_vertex_decl(elems, sizeof(ImDrawVertAllegro));

#if ALLEGRO_HAS_CLIPBOARD
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_SetClipboardTextFn = ImGui_ImplAllegro5_SetClipboardText;
    platform_io.Platform_GetClipboardTextFn = ImGui_ImplAllegro5_GetClipboardText;
#endif

    return true;
}

void ImGui_ImplAllegro5_Shutdown()
{
    ImGui_ImplAllegro5_Data* bd = ImGui_ImplAllegro5_GetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplAllegro5_InvalidateDeviceObjects();
    if (bd->VertexDecl)
        al_destroy_vertex_decl(bd->VertexDecl);
    if (bd->ClipboardTextData)
        al_free(bd->ClipboardTextData);

    io.BackendPlatformName = io.BackendRendererName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_HasMouseCursors;
    IM_DELETE(bd);
}

// ev->keyboard.modifiers seems always zero so using that...
static void ImGui_ImplAllegro5_UpdateKeyModifiers()
{
    ImGuiIO& io = ImGui::GetIO();
    ALLEGRO_KEYBOARD_STATE keys;
    al_get_keyboard_state(&keys);
    io.AddKeyEvent(ImGuiMod_Ctrl, al_key_down(&keys, ALLEGRO_KEY_LCTRL) || al_key_down(&keys, ALLEGRO_KEY_RCTRL));
    io.AddKeyEvent(ImGuiMod_Shift, al_key_down(&keys, ALLEGRO_KEY_LSHIFT) || al_key_down(&keys, ALLEGRO_KEY_RSHIFT));
    io.AddKeyEvent(ImGuiMod_Alt, al_key_down(&keys, ALLEGRO_KEY_ALT) || al_key_down(&keys, ALLEGRO_KEY_ALTGR));
    io.AddKeyEvent(ImGuiMod_Super, al_key_down(&keys, ALLEGRO_KEY_LWIN) || al_key_down(&keys, ALLEGRO_KEY_RWIN));
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
bool ImGui_ImplAllegro5_ProcessEvent(ALLEGRO_EVENT* ev)
{
    ImGui_ImplAllegro5_Data* bd = ImGui_ImplAllegro5_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplAllegro5_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    switch (ev->type)
    {
    case ALLEGRO_EVENT_MOUSE_AXES:
        if (ev->mouse.display == bd->Display)
        {
            io.AddMousePosEvent(ev->mouse.x, ev->mouse.y);
            io.AddMouseWheelEvent(-ev->mouse.dw, ev->mouse.dz);
        }
        return true;
    case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
    case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
        if (ev->mouse.display == bd->Display && ev->mouse.button > 0 && ev->mouse.button <= 5)
            io.AddMouseButtonEvent(ev->mouse.button - 1, ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN);
        return true;
    case ALLEGRO_EVENT_TOUCH_MOVE:
        if (ev->touch.display == bd->Display)
            io.AddMousePosEvent(ev->touch.x, ev->touch.y);
        return true;
    case ALLEGRO_EVENT_TOUCH_BEGIN:
    case ALLEGRO_EVENT_TOUCH_END:
    case ALLEGRO_EVENT_TOUCH_CANCEL:
        if (ev->touch.display == bd->Display && ev->touch.primary)
            io.AddMouseButtonEvent(0, ev->type == ALLEGRO_EVENT_TOUCH_BEGIN);
        return true;
    case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
        if (ev->mouse.display == bd->Display)
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        return true;
    case ALLEGRO_EVENT_KEY_CHAR:
        if (ev->keyboard.display == bd->Display)
            if (ev->keyboard.unichar != 0)
                io.AddInputCharacter((unsigned int)ev->keyboard.unichar);
        return true;
    case ALLEGRO_EVENT_KEY_DOWN:
    case ALLEGRO_EVENT_KEY_UP:
        if (ev->keyboard.display == bd->Display)
        {
            ImGui_ImplAllegro5_UpdateKeyModifiers();
            ImGuiKey key = ImGui_ImplAllegro5_KeyCodeToImGuiKey(ev->keyboard.keycode);
            io.AddKeyEvent(key, (ev->type == ALLEGRO_EVENT_KEY_DOWN));
            io.SetKeyEventNativeData(key, ev->keyboard.keycode, -1); // To support legacy indexing (<1.87 user code)
        }
        return true;
    case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
        if (ev->display.source == bd->Display)
            io.AddFocusEvent(false);
        return true;
    case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
        if (ev->display.source == bd->Display)
        {
            io.AddFocusEvent(true);
#if defined(ALLEGRO_UNSTABLE)
            al_clear_keyboard_state(bd->Display);
#endif
        }
        return true;
    }
    return false;
}

static void ImGui_ImplAllegro5_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    ImGui_ImplAllegro5_Data* bd = ImGui_ImplAllegro5_GetBackendData();
    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();

    // Hide OS mouse cursor if imgui is drawing it
    if (io.MouseDrawCursor)
        imgui_cursor = ImGuiMouseCursor_None;

    if (bd->LastCursor == imgui_cursor)
        return;
    bd->LastCursor = imgui_cursor;
    if (imgui_cursor == ImGuiMouseCursor_None)
    {
        al_set_mouse_cursor(bd->Display, bd->MouseCursorInvisible);
    }
    else
    {
        ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_TextInput:    cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT; break;
        case ImGuiMouseCursor_ResizeAll:    cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_MOVE; break;
        case ImGuiMouseCursor_ResizeNS:     cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_N; break;
        case ImGuiMouseCursor_ResizeEW:     cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_E; break;
        case ImGuiMouseCursor_ResizeNESW:   cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NE; break;
        case ImGuiMouseCursor_ResizeNWSE:   cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NW; break;
        case ImGuiMouseCursor_Wait:         cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY; break;
        case ImGuiMouseCursor_Progress:     cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_PROGRESS; break;
        case ImGuiMouseCursor_NotAllowed:   cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_UNAVAILABLE; break;
        }
        al_set_system_mouse_cursor(bd->Display, cursor_id);
    }
}

void ImGui_ImplAllegro5_NewFrame()
{
    ImGui_ImplAllegro5_Data* bd = ImGui_ImplAllegro5_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplAllegro5_Init()?");

    if (!bd->Texture)
        ImGui_ImplAllegro5_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    w = al_get_display_width(bd->Display);
    h = al_get_display_height(bd->Display);
    io.DisplaySize = ImVec2((float)w, (float)h);

    // Setup time step
    double current_time = al_get_time();
    io.DeltaTime = bd->Time > 0.0 ? (float)(current_time - bd->Time) : (float)(1.0f / 60.0f);
    bd->Time = current_time;

    // Setup mouse cursor shape
    ImGui_ImplAllegro5_UpdateMouseCursor();
}

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
