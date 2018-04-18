// ImGui Allegro 5 bindings

// Implemented features:
//  [X] User texture binding. Use 'ALLEGRO_BITMAP*' as ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.
// Missing features:
//  [ ] Clipboard support via al_set_clipboard_text/al_clipboard_has_text.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui, Original code by @birthggd

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2018-04-18: Misc: Renamed file from imgui_impl_a5.cpp to imgui_impl_allegro5.cpp.
//  2018-04-18: Misc: Added support for 32-bits vertex indices to avoid conversion at runtime. Added imconfig_allegro5.h to enforce 32-bit indices when included from imgui.h.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback and exposed ImGui_ImplA5_RenderDrawData() in the .h file so you can call it yourself.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.

#include <stdint.h>     // uint64_t
#include <cstring>      // memcpy
#include "imgui.h"
#include "imgui_impl_allegro5.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#ifdef _WIN32
#include <allegro5/allegro_windows.h>
#endif

// Data
static ALLEGRO_DISPLAY*         g_Display = NULL;
static ALLEGRO_BITMAP*          g_Texture = NULL;
static double                   g_Time = 0.0;
static ALLEGRO_MOUSE_CURSOR*    g_MouseCursorInvisible = NULL;
static ALLEGRO_VERTEX_DECL*     g_VertexDecl = NULL;

struct ImDrawVertAllegro
{
    ImVec2 pos;
    ImVec2 uv;
    ALLEGRO_COLOR col;
};

// Render function.
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
void ImGui_ImplA5_RenderDrawData(ImDrawData* draw_data)
{
    int op, src, dst;
    al_get_blender(&op, &src, &dst);
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        // FIXME-OPT: Unfortunately Allegro doesn't support 32-bits packed colors so we have to convert them to 4 floats
        static ImVector<ImDrawVertAllegro> vertices;
        vertices.resize(cmd_list->VtxBuffer.Size);
        for (int i = 0; i < cmd_list->VtxBuffer.Size; ++i)
        {
            const ImDrawVert &dv = cmd_list->VtxBuffer[i];
            ImDrawVertAllegro v;
            v.pos = dv.pos;
            v.uv = dv.uv;
            unsigned char *c = (unsigned char*)&dv.col;
            v.col = al_map_rgba(c[0], c[1], c[2], c[3]);
            vertices[i] = v;
        }

        const int* indices = NULL;
        if (sizeof(ImDrawIdx) == 2)
        {
            // FIXME-OPT: Unfortunately Allegro doesn't support 16-bit indices.. You can '#define ImDrawIdx  int' in imconfig.h to request ImGui to output 32-bit indices.
            // Otherwise, we convert them from 16-bit to 32-bit at runtime here, which works perfectly but is a little wasteful.
            static ImVector<int> indices_converted;
            indices_converted.resize(cmd_list->IdxBuffer.Size);
            for (int i = 0; i < cmd_list->IdxBuffer.Size; ++i)
                indices_converted[i] = (int)cmd_list->IdxBuffer.Data[i];
            indices = indices_converted.Data;
        }
        else if (sizeof(ImDrawIdx) == 4)
        {
            indices = (const int*)cmd_list->IdxBuffer.Data;
        }

        int idx_offset = 0;
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                ALLEGRO_BITMAP* texture = (ALLEGRO_BITMAP*)pcmd->TextureId;
                al_set_clipping_rectangle(pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z-pcmd->ClipRect.x, pcmd->ClipRect.w-pcmd->ClipRect.y);
                al_draw_indexed_prim(&vertices[0], g_VertexDecl, texture, &indices[idx_offset], pcmd->ElemCount, ALLEGRO_PRIM_TRIANGLE_LIST);
            }
            idx_offset += pcmd->ElemCount;
        }
    }

    // Restore modified state
    al_set_blender(op, src, dst);
    al_set_clipping_rectangle(0, 0, al_get_display_width(g_Display), al_get_display_height(g_Display));
}

bool Imgui_ImplA5_CreateDeviceObjects()
{
    // Build texture atlas
    ImGuiIO &io = ImGui::GetIO();
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Create texture
    int flags = al_get_new_bitmap_flags();
    int fmt = al_get_new_bitmap_format();
    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP|ALLEGRO_MIN_LINEAR|ALLEGRO_MAG_LINEAR);
    al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
    ALLEGRO_BITMAP* img = al_create_bitmap(width, height);
    al_set_new_bitmap_flags(flags);
    al_set_new_bitmap_format(fmt);
    if (!img)
        return false;

    ALLEGRO_LOCKED_REGION *locked_img = al_lock_bitmap(img, al_get_bitmap_format(img), ALLEGRO_LOCK_WRITEONLY);
    if (!locked_img)
    {
        al_destroy_bitmap(img);
        return false;
    }
    memcpy(locked_img->data, pixels, sizeof(int)*width*height);
    al_unlock_bitmap(img);

    // Convert software texture to hardware texture.
    ALLEGRO_BITMAP* cloned_img = al_clone_bitmap(img);
    al_destroy_bitmap(img);
    if (!cloned_img)
        return false;

    // Store our identifier
    io.Fonts->TexID = (void*)cloned_img;
    g_Texture = cloned_img;

    // Create an invisible mouse cursor
    // Because al_hide_mouse_cursor() seems to mess up with the actual inputs..
    ALLEGRO_BITMAP* mouse_cursor = al_create_bitmap(8,8);
    g_MouseCursorInvisible = al_create_mouse_cursor(mouse_cursor, 0, 0);
    al_destroy_bitmap(mouse_cursor);

    return true;
}

void ImGui_ImplA5_InvalidateDeviceObjects()
{
    if (g_Texture)
    {
        al_destroy_bitmap(g_Texture);
        ImGui::GetIO().Fonts->TexID = NULL;
        g_Texture = NULL;
    }
    if (g_MouseCursorInvisible)
    {
        al_destroy_mouse_cursor(g_MouseCursorInvisible);
        g_MouseCursorInvisible = NULL;
    }
}

bool ImGui_ImplA5_Init(ALLEGRO_DISPLAY* display)
{
    g_Display = display;

    // Create custom vertex declaration.
    // Unfortunately Allegro doesn't support 32-bits packed colors so we have to convert them to 4 floats.
    // We still use a custom declaration to use 'ALLEGRO_PRIM_TEX_COORD' instead of 'ALLEGRO_PRIM_TEX_COORD_PIXEL' else we can't do a reliable conversion.
    ALLEGRO_VERTEX_ELEMENT elems[] =
    {
        { ALLEGRO_PRIM_POSITION, ALLEGRO_PRIM_FLOAT_2, IM_OFFSETOF(ImDrawVertAllegro, pos) },
        { ALLEGRO_PRIM_TEX_COORD, ALLEGRO_PRIM_FLOAT_2, IM_OFFSETOF(ImDrawVertAllegro, uv) },
        { ALLEGRO_PRIM_COLOR_ATTR, 0, IM_OFFSETOF(ImDrawVertAllegro, col) },
        { 0, 0, 0 }
    };
    g_VertexDecl = al_create_vertex_decl(elems, sizeof(ImDrawVertAllegro));

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = ALLEGRO_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = ALLEGRO_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = ALLEGRO_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = ALLEGRO_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = ALLEGRO_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = ALLEGRO_KEY_PGUP;
    io.KeyMap[ImGuiKey_PageDown] = ALLEGRO_KEY_PGDN;
    io.KeyMap[ImGuiKey_Home] = ALLEGRO_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = ALLEGRO_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = ALLEGRO_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = ALLEGRO_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = ALLEGRO_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = ALLEGRO_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = ALLEGRO_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = ALLEGRO_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = ALLEGRO_KEY_A;
    io.KeyMap[ImGuiKey_C] = ALLEGRO_KEY_C;
    io.KeyMap[ImGuiKey_V] = ALLEGRO_KEY_V;
    io.KeyMap[ImGuiKey_X] = ALLEGRO_KEY_X;
    io.KeyMap[ImGuiKey_Y] = ALLEGRO_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = ALLEGRO_KEY_Z;

#ifdef _WIN32
    io.ImeWindowHandle = al_get_win_window_handle(g_Display);
#endif

    return true;
}

void ImGui_ImplA5_Shutdown()
{
    ImGui_ImplA5_InvalidateDeviceObjects();
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
bool ImGui_ImplA5_ProcessEvent(ALLEGRO_EVENT *ev)
{
    ImGuiIO &io = ImGui::GetIO();

    switch (ev->type)
    {
    case ALLEGRO_EVENT_MOUSE_AXES:
        io.MouseWheel += ev->mouse.dz;
        io.MouseWheelH += ev->mouse.dw;
        return true;
    case ALLEGRO_EVENT_KEY_CHAR:
        if (ev->keyboard.display == g_Display)
            if (ev->keyboard.unichar > 0 && ev->keyboard.unichar < 0x10000)
                io.AddInputCharacter((unsigned short)ev->keyboard.unichar);
        return true;
    case ALLEGRO_EVENT_KEY_DOWN:
    case ALLEGRO_EVENT_KEY_UP:
        if (ev->keyboard.display == g_Display)
            io.KeysDown[ev->keyboard.keycode] = (ev->type == ALLEGRO_EVENT_KEY_DOWN);
        return true;
    }
    return false;
}

void ImGui_ImplA5_NewFrame()
{
    if (!g_Texture)
        Imgui_ImplA5_CreateDeviceObjects();

    ImGuiIO &io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    w = al_get_display_width(g_Display);
    h = al_get_display_height(g_Display);
    io.DisplaySize = ImVec2((float)w, (float)h);

    // Setup time step
    double current_time = al_get_time();
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    ALLEGRO_KEYBOARD_STATE keys;
    al_get_keyboard_state(&keys);
    io.KeyCtrl = al_key_down(&keys, ALLEGRO_KEY_LCTRL) || al_key_down(&keys, ALLEGRO_KEY_RCTRL);
    io.KeyShift = al_key_down(&keys, ALLEGRO_KEY_LSHIFT) || al_key_down(&keys, ALLEGRO_KEY_RSHIFT);
    io.KeyAlt = al_key_down(&keys, ALLEGRO_KEY_ALT) || al_key_down(&keys, ALLEGRO_KEY_ALTGR);
    io.KeySuper = al_key_down(&keys, ALLEGRO_KEY_LWIN) || al_key_down(&keys, ALLEGRO_KEY_RWIN);

    ALLEGRO_MOUSE_STATE mouse;
    if (keys.display == g_Display)
    {
        al_get_mouse_state(&mouse);
        io.MousePos = ImVec2((float)mouse.x, (float)mouse.y);
    }
    else
    {
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    }

    al_get_mouse_state(&mouse);
    io.MouseDown[0] = mouse.buttons & (1 << 0);
    io.MouseDown[1] = mouse.buttons & (1 << 1);
    io.MouseDown[2] = mouse.buttons & (1 << 2);

    // Hide OS mouse cursor if ImGui is drawing it
    if (io.MouseDrawCursor)
    {
        al_set_mouse_cursor(g_Display, g_MouseCursorInvisible);
    }
    else
    {
        ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT;
        switch (ImGui::GetMouseCursor())
        {
        case ImGuiMouseCursor_TextInput:    cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT; break;
        case ImGuiMouseCursor_ResizeAll:    cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_MOVE; break;
        case ImGuiMouseCursor_ResizeNS:     cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_N; break;
        case ImGuiMouseCursor_ResizeEW:     cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_E; break;
        case ImGuiMouseCursor_ResizeNESW:   cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NE; break;
        case ImGuiMouseCursor_ResizeNWSE:   cursor_id = ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NW; break;
        }
        al_set_system_mouse_cursor(g_Display, cursor_id);
    }

    // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
    ImGui::NewFrame();
}
