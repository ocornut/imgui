// dear imgui: Renderer + Platform Backend for Marmalade + IwGx
// Marmalade code: Copyright (C) 2015 by Giovanni Zito (this file is part of Dear ImGui)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'CIwTexture*' as ImTextureID. Read the FAQ about ImTextureID!
// Missing features:
//  [ ] Renderer: Clipping rectangles are not honored.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this. 
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2021-05-19: Renderer: Replaced direct access to ImDrawCmd::TextureId with a call to ImDrawCmd::GetTexID(). (will become a requirement)
//  2019-07-21: Inputs: Added mapping for ImGuiKey_KeyPadEnter.
//  2019-05-11: Inputs: Don't filter value from character callback before calling AddInputCharacter().
//  2018-11-30: Misc: Setting up io.BackendPlatformName/io.BackendRendererName so they can be displayed in the About Window.
//  2018-02-16: Misc: Obsoleted the io.RenderDrawListsFn callback and exposed ImGui_Marmalade_RenderDrawData() in the .h file so you can call it yourself.
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.

#include "imgui.h"
#include "imgui_impl_marmalade.h"

#include <s3eClipboard.h>
#include <s3ePointer.h>
#include <s3eKeyboard.h>
#include <IwTexture.h>
#include <IwGx.h>

// Data
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static CIwTexture*  g_FontTexture = NULL;
static char*        g_ClipboardText = NULL;
static bool         g_osdKeyboardEnabled = false;

// use this setting to scale the interface - e.g. on device you could use 2 or 3 scale factor
static ImVec2       g_RenderScale = ImVec2(1.0f, 1.0f);

// Render function.
void ImGui_Marmalade_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
        const int nVert = cmd_list->VtxBuffer.Size;
        CIwFVec2* pVertStream = IW_GX_ALLOC(CIwFVec2, nVert);
        CIwFVec2* pUVStream = IW_GX_ALLOC(CIwFVec2, nVert);
        CIwColour* pColStream = IW_GX_ALLOC(CIwColour, nVert);

        for (int i = 0; i < nVert; i++)
        {
            // FIXME-OPT: optimize multiplication on GPU using vertex shader/projection matrix.
            pVertStream[i].x = cmd_list->VtxBuffer[i].pos.x * g_RenderScale.x;
            pVertStream[i].y = cmd_list->VtxBuffer[i].pos.y * g_RenderScale.y;
            pUVStream[i].x = cmd_list->VtxBuffer[i].uv.x;
            pUVStream[i].y = cmd_list->VtxBuffer[i].uv.y;
            pColStream[i] = cmd_list->VtxBuffer[i].col;
        }

        IwGxSetVertStreamScreenSpace(pVertStream, nVert);
        IwGxSetUVStream(pUVStream);
        IwGxSetColStream(pColStream, nVert);
        IwGxSetNormStream(0);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // FIXME: Not honoring ClipRect fields.
                CIwMaterial* pCurrentMaterial = IW_GX_ALLOC_MATERIAL();
                pCurrentMaterial->SetShadeMode(CIwMaterial::SHADE_FLAT);
                pCurrentMaterial->SetCullMode(CIwMaterial::CULL_NONE);
                pCurrentMaterial->SetFiltering(false);
                pCurrentMaterial->SetAlphaMode(CIwMaterial::ALPHA_BLEND);
                pCurrentMaterial->SetDepthWriteMode(CIwMaterial::DEPTH_WRITE_NORMAL);
                pCurrentMaterial->SetAlphaTestMode(CIwMaterial::ALPHATEST_DISABLED);
                pCurrentMaterial->SetTexture((CIwTexture*)pcmd->GetTexID());
                IwGxSetMaterial(pCurrentMaterial);
                IwGxDrawPrims(IW_GX_TRI_LIST, (uint16*)idx_buffer, pcmd->ElemCount);
            }
            idx_buffer += pcmd->ElemCount;
        }
        IwGxFlush();
    }

    // TODO: restore modified state (i.e. mvp matrix)
}

static const char* ImGui_Marmalade_GetClipboardText(void* /*user_data*/)
{
    if (!s3eClipboardAvailable())
        return NULL;

    if (int size = s3eClipboardGetText(NULL, 0))
    {
        if (g_ClipboardText)
            delete[] g_ClipboardText;
        g_ClipboardText = new char[size];
        g_ClipboardText[0] = '\0';
        s3eClipboardGetText(g_ClipboardText, size);
    }

    return g_ClipboardText;
}

static void ImGui_Marmalade_SetClipboardText(void* /*user_data*/, const char* text)
{
    if (s3eClipboardAvailable())
        s3eClipboardSetText(text);
}

int32 ImGui_Marmalade_PointerButtonEventCallback(void* system_data, void* user_data)
{
    // pEvent->m_Button is of type s3ePointerButton and indicates which mouse
    // button was pressed.  For touchscreen this should always have the value
    // S3E_POINTER_BUTTON_SELECT
    s3ePointerEvent* pEvent = (s3ePointerEvent*)system_data;

    if (pEvent->m_Pressed == 1)
    {
        if (pEvent->m_Button == S3E_POINTER_BUTTON_LEFTMOUSE)
            g_MousePressed[0] = true;
        if (pEvent->m_Button == S3E_POINTER_BUTTON_RIGHTMOUSE)
            g_MousePressed[1] = true;
        if (pEvent->m_Button == S3E_POINTER_BUTTON_MIDDLEMOUSE)
            g_MousePressed[2] = true;
        if (pEvent->m_Button == S3E_POINTER_BUTTON_MOUSEWHEELUP)
            io.MouseWheel += pEvent->m_y;
        if (pEvent->m_Button == S3E_POINTER_BUTTON_MOUSEWHEELDOWN)
            io.MouseWheel += pEvent->m_y;
    }

    return 0;
}

int32 ImGui_Marmalade_KeyCallback(void* system_data, void* user_data)
{
    ImGuiIO& io = ImGui::GetIO();
    s3eKeyboardEvent* e = (s3eKeyboardEvent*)system_data;
    if (e->m_Pressed == 1)
        io.KeysDown[e->m_Key] = true;
    if (e->m_Pressed == 0)
        io.KeysDown[e->m_Key] = false;

    io.KeyCtrl = s3eKeyboardGetState(s3eKeyLeftControl) == S3E_KEY_STATE_DOWN || s3eKeyboardGetState(s3eKeyRightControl) == S3E_KEY_STATE_DOWN;
    io.KeyShift = s3eKeyboardGetState(s3eKeyLeftShift) == S3E_KEY_STATE_DOWN || s3eKeyboardGetState(s3eKeyRightShift) == S3E_KEY_STATE_DOWN;
    io.KeyAlt = s3eKeyboardGetState(s3eKeyLeftAlt) == S3E_KEY_STATE_DOWN || s3eKeyboardGetState(s3eKeyRightAlt) == S3E_KEY_STATE_DOWN;
    io.KeySuper = s3eKeyboardGetState(s3eKeyLeftWindows) == S3E_KEY_STATE_DOWN || s3eKeyboardGetState(s3eKeyRightWindows) == S3E_KEY_STATE_DOWN;

    return 0;
}

int32 ImGui_Marmalade_CharCallback(void* system_data, void* user_data)
{
    ImGuiIO& io = ImGui::GetIO();
    s3eKeyboardCharEvent* e = (s3eKeyboardCharEvent*)system_data;
    io.AddInputCharacter((unsigned int)e->m_Char);

    return 0;
}

bool ImGui_Marmalade_CreateDeviceObjects()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    g_FontTexture = new CIwTexture();
    g_FontTexture->SetModifiable(true);
    CIwImage& image = g_FontTexture->GetImage();
    image.SetFormat(CIwImage::ARGB_8888);
    image.SetWidth(width);
    image.SetHeight(height);
    image.SetBuffers();                                    // allocates and own buffers
    image.ReadTexels(pixels);
    g_FontTexture->SetMipMapping(false);
    g_FontTexture->SetFiltering(false);
    g_FontTexture->Upload();

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)g_FontTexture);

    return true;
}

void    ImGui_Marmalade_InvalidateDeviceObjects()
{
    if (g_ClipboardText)
    {
        delete[] g_ClipboardText;
        g_ClipboardText = NULL;
    }

    if (g_FontTexture)
    {
        ImGui::GetIO().Fonts->SetTexID(0);
        delete g_FontTexture;
        g_FontTexture = NULL;
    }
}

bool    ImGui_Marmalade_Init(bool install_callbacks)
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = io.BackendRendererName = "imgui_impl_marmalade";

    // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = s3eKeyTab
    io.KeyMap[ImGuiKey_LeftArrow] = s3eKeyLeft;
    io.KeyMap[ImGuiKey_RightArrow] = s3eKeyRight;
    io.KeyMap[ImGuiKey_UpArrow] = s3eKeyUp;
    io.KeyMap[ImGuiKey_DownArrow] = s3eKeyDown;
    io.KeyMap[ImGuiKey_PageUp] = s3eKeyPageUp;
    io.KeyMap[ImGuiKey_PageDown] = s3eKeyPageDown;
    io.KeyMap[ImGuiKey_Home] = s3eKeyHome;
    io.KeyMap[ImGuiKey_End] = s3eKeyEnd;
    io.KeyMap[ImGuiKey_Insert] = s3eKeyInsert;
    io.KeyMap[ImGuiKey_Delete] = s3eKeyDelete;
    io.KeyMap[ImGuiKey_Backspace] = s3eKeyBackspace;
    io.KeyMap[ImGuiKey_Space] = s3eKeySpace;
    io.KeyMap[ImGuiKey_Enter] = s3eKeyEnter;
    io.KeyMap[ImGuiKey_Escape] = s3eKeyEsc;
    io.KeyMap[ImGuiKey_KeyPadEnter] = s3eKeyNumPadEnter;
    io.KeyMap[ImGuiKey_A] = s3eKeyA;
    io.KeyMap[ImGuiKey_C] = s3eKeyC;
    io.KeyMap[ImGuiKey_V] = s3eKeyV;
    io.KeyMap[ImGuiKey_X] = s3eKeyX;
    io.KeyMap[ImGuiKey_Y] = s3eKeyY;
    io.KeyMap[ImGuiKey_Z] = s3eKeyZ;

    io.SetClipboardTextFn = ImGui_Marmalade_SetClipboardText;
    io.GetClipboardTextFn = ImGui_Marmalade_GetClipboardText;

    if (install_callbacks)
    {
        s3ePointerRegister(S3E_POINTER_BUTTON_EVENT, ImGui_Marmalade_PointerButtonEventCallback, 0);
        s3eKeyboardRegister(S3E_KEYBOARD_KEY_EVENT, ImGui_Marmalade_KeyCallback, 0);
        s3eKeyboardRegister(S3E_KEYBOARD_CHAR_EVENT, ImGui_Marmalade_CharCallback, 0);
    }

    return true;
}

void ImGui_Marmalade_Shutdown()
{
    ImGui_Marmalade_InvalidateDeviceObjects();
}

void ImGui_Marmalade_NewFrame()
{
    if (!g_FontTexture)
        ImGui_Marmalade_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w = IwGxGetScreenWidth(), h = IwGxGetScreenHeight();
    io.DisplaySize = ImVec2((float)w, (float)h);
    // For retina display or other situations where window coordinates are different from framebuffer coordinates. User storage only, presently not used by ImGui.
    io.DisplayFramebufferScale = g_scale;

    // Setup time step
    double current_time = s3eTimerGetUST() / 1000.0f;
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
    g_Time = current_time;

    double mouse_x, mouse_y;
    mouse_x = s3ePointerGetX();
    mouse_y = s3ePointerGetY();
    io.MousePos = ImVec2((float)mouse_x / g_scale.x, (float)mouse_y / g_scale.y); // Mouse position (set to -FLT_MAX,-FLT_MAX if no mouse / on another screen, etc.)

    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = g_MousePressed[i] || s3ePointerGetState((s3ePointerButton)i) != S3E_POINTER_STATE_UP;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        g_MousePressed[i] = false;
    }

    // TODO: Hide OS mouse cursor if ImGui is drawing it
    // s3ePointerSetInt(S3E_POINTER_HIDE_CURSOR,(io.MouseDrawCursor ? 0 : 1));

    // Show/hide OSD keyboard
    if (io.WantTextInput)
    {
        // Some text input widget is active?
        if (!g_osdKeyboardEnabled)
        {
            g_osdKeyboardEnabled = true;
            s3eKeyboardSetInt(S3E_KEYBOARD_GET_CHAR, 1);    // show OSD keyboard
        }
    }
    else
    {
        // No text input widget is active
        if (g_osdKeyboardEnabled)
        {
            g_osdKeyboardEnabled = false;
            s3eKeyboardSetInt(S3E_KEYBOARD_GET_CHAR, 0);    // hide OSD keyboard
        }
    }
}
