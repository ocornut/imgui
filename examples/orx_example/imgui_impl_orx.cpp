// ImGui ORX binding by Denis Brachet (aka ainvar)
// In this binding, ImTextureID is used to store an orxBITMAP pointer. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "imgui_impl_orx.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>
#include <map>

extern "C" 
{
    #include <orx.h>
}

//////////////////////////////////////////////////////////////////////////
//                              DEFINES
//////////////////////////////////////////////////////////////////////////

//! Maximum number of mouse buttons handled by ImGui: left, right, middle, extra1, extra2
#define MAX_IMGUI_MOUSE_BUTTONS         5


//////////////////////////////////////////////////////////////////////////
//                              DATA TYPES
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//! Struct that contains the mapping between orx and ImGUI mouse buttons and the current state
struct ImGui_Orx_MouseButtonState
    {
    //! Mouse button code in orx sytem
    orxMOUSE_BUTTON		m_Button;
    //! Mouse button code in ImGui system
    unsigned char		m_ImGUIButton;
    //! Last read pressure state
    bool				m_Pressed;
    };

//////////////////////////////////////////////////////////////////////////
//! Simple clipboard implementation
class ImGui_Orx_Clipboard
    {
    public:
        //! Sets the text associated to the given user data
        void SetText(void * user_data, const char * text) { m_ClipboardMap[user_data] = text; }
        //! Retrieves the text associated to the given user data
        const char * GetText(void * user_data) const {
            auto it = m_ClipboardMap.find(user_data);
            if (it != m_ClipboardMap.end())
                return (it->second).c_str();
            return nullptr;
            }

    protected:
        //! Text mapped to user data
        std::map<void *, std::string>   m_ClipboardMap;
    };

//////////////////////////////////////////////////////////////////////////
//! Static module data
struct ImGui_Orx_ModuleData
    {
    ImGui_Orx_ModuleData() :
        m_FontTexture(nullptr),
        m_Time(orxFLOAT_0),
        m_pstMainClock(nullptr)
        {}

    //! Current font texture
    orxBITMAP *                 m_FontTexture;
    //! Clipboard object
    ImGui_Orx_Clipboard         m_Clipboard;
    //! Last render time
    double                      m_Time;
    //! Last state of mouse buttons and mapping between orx and ImGui
    ImGui_Orx_MouseButtonState  m_ButtonStates[MAX_IMGUI_MOUSE_BUTTONS];
    //! clock used to check wheel delta
    orxCLOCK *                  m_pstMainClock;
    };


//////////////////////////////////////////////////////////////////////////
//                              INTERNAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////

//! Called once to initialize the font texture
static orxBITMAP * ImGui_Orx_InitFontTexture(ImGuiIO & io);
//! Updates mouse state
static void ImGui_Orx_UpdateMouse(ImGuiIO & io);
//! Updates keyboard state
static void ImGui_Orx_UpdateKeyboard(ImGuiIO & io);
//! Updates mouse wheel state, called using gModuleData.m_pstMainClock
static void orxFASTCALL ImGui_Orx_MouseWheelUpdate(const orxCLOCK_INFO *_pstClockInfo, void *_pContext);
//! Called by ImGui to retrieve last clipboard test
static const char* ImGui_Orx_GetClipboardText(void* user_data);
//! Called by ImGui to set clipboard text
static void ImGui_Orx_SetClipboardText(void* user_data, const char* text);
//! Creates objects (font texture)
static void ImGui_Orx_CreateDeviceObjects();

//////////////////////////////////////////////////////////////////////////
//                              GLOBAL DATA
//////////////////////////////////////////////////////////////////////////

//! Global module data instance
static ImGui_Orx_ModuleData gModuleData;

//////////////////////////////////////////////////////////////////////////
bool ImGui_Orx_Init()
    {
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = orxKEYBOARD_KEY_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = orxKEYBOARD_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = orxKEYBOARD_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = orxKEYBOARD_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = orxKEYBOARD_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = orxKEYBOARD_KEY_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = orxKEYBOARD_KEY_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = orxKEYBOARD_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = orxKEYBOARD_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = orxKEYBOARD_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = orxKEYBOARD_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = orxKEYBOARD_KEY_RETURN;
    io.KeyMap[ImGuiKey_Escape] = orxKEYBOARD_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = orxKEYBOARD_KEY_A;
    io.KeyMap[ImGuiKey_C] = orxKEYBOARD_KEY_C;
    io.KeyMap[ImGuiKey_V] = orxKEYBOARD_KEY_V;
    io.KeyMap[ImGuiKey_X] = orxKEYBOARD_KEY_X;
    io.KeyMap[ImGuiKey_Y] = orxKEYBOARD_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = orxKEYBOARD_KEY_Z;

    io.RenderDrawListsFn = nullptr/*ImGui_Orx_RenderDrawLists*/;      // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = ImGui_Orx_SetClipboardText;
    io.GetClipboardTextFn = ImGui_Orx_GetClipboardText;
    io.ClipboardUserData = nullptr;

    gModuleData.m_ButtonStates[0] = { orxMOUSE_BUTTON_LEFT		, 0		, false };
    gModuleData.m_ButtonStates[1] = { orxMOUSE_BUTTON_RIGHT		, 1		, false };
    gModuleData.m_ButtonStates[2] = { orxMOUSE_BUTTON_MIDDLE	, 2		, false };
    gModuleData.m_ButtonStates[3] = { orxMOUSE_BUTTON_EXTRA_1	, 3		, false };
    gModuleData.m_ButtonStates[4] = { orxMOUSE_BUTTON_EXTRA_2	, 4		, false };

    gModuleData.m_pstMainClock = orxClock_FindFirst(orx2F(-1.0f), orxCLOCK_TYPE_CORE);
    if (gModuleData.m_pstMainClock != orxNULL)
        orxClock_Register(gModuleData.m_pstMainClock, ImGui_Orx_MouseWheelUpdate, orxNULL, orxMODULE_ID_INPUT, orxCLOCK_PRIORITY_HIGH);

#ifdef _WIN32
    io.ImeWindowHandle = /*glfwGetWin32Window(gModuleData.m_Window)*/nullptr;
#endif

    return true;
    }

//////////////////////////////////////////////////////////////////////////
void ImGui_Orx_Shutdown()
    {
    if (gModuleData.m_pstMainClock != orxNULL)
        orxClock_Unregister(gModuleData.m_pstMainClock, ImGui_Orx_MouseWheelUpdate);

    ImGui_Orx_InvalidateDeviceObjects();
//    ImGui::Shutdown();
    }

//////////////////////////////////////////////////////////////////////////
void ImGui_Orx_NewFrame()
    {
    if (!gModuleData.m_FontTexture)
        ImGui_Orx_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    orxFLOAT display_w, display_h;

    /* get screen size */
    orxDisplay_GetScreenSize(&display_w, &display_h);

    io.DisplaySize = ImVec2(display_w, display_h);
    io.DisplayFramebufferScale = ImVec2(1, 1);

    // Setup time step
    double current_time = orxSystem_GetSystemTime();
    io.DeltaTime = gModuleData.m_Time > 0.0 ? (float)(current_time - gModuleData.m_Time) : (float)(1.0f / 60.0f);
    gModuleData.m_Time = current_time;

    ImGui_Orx_UpdateMouse(io);
    ImGui_Orx_UpdateKeyboard(io);

    // Start the frame
    ImGui::NewFrame();
    }

//////////////////////////////////////////////////////////////////////////
void ImGui_Orx_CreateDeviceObjects()
    {
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();

    // Store our identifier
    gModuleData.m_FontTexture = ImGui_Orx_InitFontTexture(io);
    io.Fonts->TexID = (void *)gModuleData.m_FontTexture;
    }

//////////////////////////////////////////////////////////////////////////
void ImGui_Orx_InvalidateDeviceObjects()
    {
    if (gModuleData.m_FontTexture)
        {
        orxDisplay_DeleteBitmap(gModuleData.m_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        gModuleData.m_FontTexture = 0;
        }
    }

//////////////////////////////////////////////////////////////////////////
void ImGui_Orx_Render(void * pvViewport, ImDrawData* draw_data)
    {
    orxVIEWPORT * pstViewport = (orxVIEWPORT *)pvViewport;

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Gets screen bitmap
    orxBITMAP * pstScreen = orxDisplay_GetScreenBitmap();
    // Restores screen as destination bitmap
    orxDisplay_SetDestinationBitmaps(&pstScreen, 1);
    // Restores screen bitmap clipping
    orxDisplay_SetBitmapClipping(orxDisplay_GetScreenBitmap(), 0, 0, orxF2U(fb_width), orxF2U(fb_height));

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
        const ImDrawList * cmd_list = draw_data->CmdLists[n];
        const ImDrawVert * vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx * idx_buffer = cmd_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
                {
                pcmd->UserCallback(cmd_list, pcmd);
                }
            else
                {
                orxCUSTOM_MESH customMesh = { 0 };
                customMesh.pstBitmap = (orxBITMAP *)pcmd->TextureId;
                customMesh.u32BitmapClipTLX = (int)pcmd->ClipRect.x;
                customMesh.u32BitmapClipTLY = (int)(fb_height - pcmd->ClipRect.w);
                customMesh.u32BitmapClipBRX = (int)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                customMesh.u32BitmapClipBRY = (int)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                customMesh.eSmoothing = orxDISPLAY_SMOOTHING_ON;
                customMesh.eBlendMode = orxDISPLAY_BLEND_MODE_ALPHA;
                customMesh.eDrawMode = orxDISPLAY_DRAW_MODE_TRIANGLES;
                customMesh.u32VertexNumber = cmd_list->VtxBuffer.size();
                customMesh.astVertexList = (orxDISPLAY_VERTEX *)vtx_buffer;
                customMesh.u32IndexesCount = cmd_list->IdxBuffer.size();
                customMesh.au16IndexList = idx_buffer;
                customMesh.u32ElementCount = pcmd->ElemCount;

                orxDisplay_DrawCustomMesh(&customMesh);
                }

            idx_buffer += pcmd->ElemCount;
            }
        }
    }

//////////////////////////////////////////////////////////////////////////
static const char* ImGui_Orx_GetClipboardText(void* user_data)
    {
    return gModuleData.m_Clipboard.GetText(user_data);
    }

//////////////////////////////////////////////////////////////////////////
static void ImGui_Orx_SetClipboardText(void* user_data, const char* text)
    {
    gModuleData.m_Clipboard.SetText(user_data, text);
    }

//////////////////////////////////////////////////////////////////////////
static orxBITMAP * ImGui_Orx_InitFontTexture(ImGuiIO & io)
    {
    orxTEXTURE *pstTexture;
    orxBITMAP  *pstBitmap;

    unsigned char* pixels;
    int u32Width, u32Height;

    // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. 
    // If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.
    io.Fonts->GetTexDataAsRGBA32(&pixels, &u32Width, &u32Height);

    // Creates background texture
    pstTexture = orxTexture_Create();
    pstBitmap = orxDisplay_CreateBitmap(u32Width, u32Height);
    orxDisplay_SetBitmapData(pstBitmap, pixels, u32Width * u32Height * sizeof(orxRGBA));
    orxTexture_LinkBitmap(pstTexture, pstBitmap, "Texture", orxTRUE);

    return pstBitmap;
    }

//////////////////////////////////////////////////////////////////////////
static void orxFASTCALL ImGui_Orx_MouseWheelUpdate(const orxCLOCK_INFO *_pstClockInfo, void *_pContext)
    {
    orxFLOAT wheel_delta = orxMouse_GetWheelDelta();
    if (wheel_delta != orxFLOAT_0)
        ImGui::GetIO().MouseWheel = orxMouse_GetWheelDelta();
    }

//////////////////////////////////////////////////////////////////////////
void ImGui_Orx_UpdateMouse(ImGuiIO & io)
    {
    // Get current mouse position
    orxVECTOR pos;
    orxMouse_GetPosition(&pos);
    io.MousePos = ImVec2(pos.fX, pos.fY);

    // test the wheel
    if (orxMouse_IsButtonPressed(orxMOUSE_BUTTON_WHEEL_UP) || orxMouse_IsButtonPressed(orxMOUSE_BUTTON_WHEEL_DOWN))
        io.MouseWheel = orxMouse_GetWheelDelta();

    // poll for special keys
    for (int a = 0; a < MAX_IMGUI_MOUSE_BUTTONS; a++)
        {
        ImGui_Orx_MouseButtonState & state = gModuleData.m_ButtonStates[a];
        // check if it's presses
        bool pressed = orxMouse_IsButtonPressed(state.m_Button) == orxTRUE;
        // check if the state changed from last time
        if (pressed != state.m_Pressed)
            {
            // yes, then update new state...
            state.m_Pressed = pressed;
            // ...and tell it to the canvas!
            io.MouseDown[state.m_ImGUIButton] = state.m_Pressed;
            }
        }
    }

//////////////////////////////////////////////////////////////////////////
void ImGui_Orx_UpdateKeyboard(ImGuiIO & io)
    {
    for (int key = orxKEYBOARD_KEY_A; key < orxKEYBOARD_KEY_NUMBER; key++)
        {
        bool pressed = orxKeyboard_IsKeyPressed((orxKEYBOARD_KEY)key) == orxTRUE;
        io.KeysDown[key] = pressed;
        }

    io.KeyCtrl = io.KeysDown[orxKEYBOARD_KEY_LCTRL] || io.KeysDown[orxKEYBOARD_KEY_RCTRL];
    io.KeyShift = io.KeysDown[orxKEYBOARD_KEY_LSHIFT] || io.KeysDown[orxKEYBOARD_KEY_RSHIFT];
    io.KeyAlt = io.KeysDown[orxKEYBOARD_KEY_LALT] || io.KeysDown[orxKEYBOARD_KEY_RALT];
    io.KeySuper = io.KeysDown[orxKEYBOARD_KEY_LSYSTEM] || io.KeysDown[orxKEYBOARD_KEY_RSYSTEM];

    io.AddInputCharactersUTF8(orxKeyboard_ReadString());
    }

