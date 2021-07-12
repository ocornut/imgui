// dear imgui: Platform Backend for GLUT/FreeGLUT
// This needs to be used along with a Renderer (e.g. OpenGL2)

// !!! GLUT/FreeGLUT IS OBSOLETE PREHISTORIC SOFTWARE. Using GLUT is not recommended unless you really miss the 90's. !!!
// !!! If someone or something is teaching you GLUT today, you are being abused. Please show some resistance. !!!
// !!! Nowadays, prefer using GLFW or SDL instead!

// Issues:
//  [ ] Platform: GLUT is unable to distinguish e.g. Backspace from CTRL+H or TAB from CTRL+I
//  [ ] Platform: Missing mouse cursor shape/visibility support.
//  [ ] Platform: Missing clipboard support (not supported by Glut).
//  [ ] Platform: Missing gamepad support.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this. 
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2019-04-03: Misc: Renamed imgui_impl_freeglut.cpp/.h to imgui_impl_glut.cpp/.h.
//  2019-03-25: Misc: Made io.DeltaTime always above zero.
//  2018-11-30: Misc: Setting up io.BackendPlatformName so it can be displayed in the About Window.
//  2018-03-22: Added GLUT Platform binding.

#include "imgui.h"
#include "imgui_impl_glut.h"
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4505) // unreferenced local function has been removed (stb stuff)
#endif

static int g_Time = 0;          // Current time, in milliseconds

bool ImGui_ImplGLUT_Init()
{
    ImGuiIO& io = ImGui::GetIO();

#ifdef FREEGLUT
    io.BackendPlatformName = "imgui_impl_glut (freeglut)";
#else
    io.BackendPlatformName = "imgui_impl_glut";
#endif

    g_Time = 0;

    // Glut has 1 function for characters and one for "special keys". We map the characters in the 0..255 range and the keys above.
    io.KeyMap[ImGuiKey_Tab]             = '\t'; // == 9 == CTRL+I
    io.KeyMap[ImGuiKey_LeftArrow]       = 256 + GLUT_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow]      = 256 + GLUT_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow]         = 256 + GLUT_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow]       = 256 + GLUT_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp]          = 256 + GLUT_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown]        = 256 + GLUT_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home]            = 256 + GLUT_KEY_HOME;
    io.KeyMap[ImGuiKey_End]             = 256 + GLUT_KEY_END;
    io.KeyMap[ImGuiKey_Insert]          = 256 + GLUT_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete]          = 127;
    io.KeyMap[ImGuiKey_Backspace]       = 8;  // == CTRL+H
    io.KeyMap[ImGuiKey_Space]           = ' ';
    io.KeyMap[ImGuiKey_Enter]           = 13; // == CTRL+M
    io.KeyMap[ImGuiKey_Escape]          = 27;
    io.KeyMap[ImGuiKey_Apostrophe]      = 39; // '
    io.KeyMap[ImGuiKey_Comma]           = 44; // ,
    io.KeyMap[ImGuiKey_Minus]           = 45; // -
    io.KeyMap[ImGuiKey_Period]          = 46; // .
    io.KeyMap[ImGuiKey_Slash]           = 47; // /
    io.KeyMap[ImGuiKey_Semicolon]       = 59; // ;
    io.KeyMap[ImGuiKey_Equal]           = 61; // =
    io.KeyMap[ImGuiKey_LeftBracket]     = 91; // [
    io.KeyMap[ImGuiKey_Backslash]       = 92; // \ (this text inhibit multiline comment caused by backlash)
    io.KeyMap[ImGuiKey_RightBracket]    = 93; // ]
    io.KeyMap[ImGuiKey_GraveAccent]     = 96; // `
    io.KeyMap[ImGuiKey_CapsLock]        = 0; // not supported
    io.KeyMap[ImGuiKey_ScrollLock]      = 0; // not supported
    io.KeyMap[ImGuiKey_NumLock]         = 256 + 0x006D; // GLUT_KEY_NUM_LOCK
    io.KeyMap[ImGuiKey_PrintScreen]     = 0; // not supported
    io.KeyMap[ImGuiKey_Pause]           = 0; // not supported
    io.KeyMap[ImGuiKey_KeyPad0]         = '0'; // == 0
    io.KeyMap[ImGuiKey_KeyPad1]         = '1'; // == 1
    io.KeyMap[ImGuiKey_KeyPad2]         = '2'; // == 2
    io.KeyMap[ImGuiKey_KeyPad3]         = '3'; // == 3
    io.KeyMap[ImGuiKey_KeyPad4]         = '4'; // == 4
    io.KeyMap[ImGuiKey_KeyPad5]         = '5'; // == 5
    io.KeyMap[ImGuiKey_KeyPad6]         = '6'; // == 6
    io.KeyMap[ImGuiKey_KeyPad7]         = '7'; // == 7
    io.KeyMap[ImGuiKey_KeyPad8]         = '8'; // == 8
    io.KeyMap[ImGuiKey_KeyPad9]         = '9'; // == 9
    io.KeyMap[ImGuiKey_KeyPadDecimal]   = 46; // == . (US - locale dependent)
    io.KeyMap[ImGuiKey_KeyPadDivide]    = 47; // == /
    io.KeyMap[ImGuiKey_KeyPadMultiply]  = 42;
    io.KeyMap[ImGuiKey_KeyPadSubtract]  = 45; // == -
    io.KeyMap[ImGuiKey_KeyPadAdd]       = 43; //
    io.KeyMap[ImGuiKey_KeyPadEnter]     = 13; // == Enter
    io.KeyMap[ImGuiKey_KeyPadEqual]     = 0; // not supported
    io.KeyMap[ImGuiKey_LeftShift]       = 256 + 0x0070; // GLUT_KEY_SHIFT_L
    io.KeyMap[ImGuiKey_LeftControl]     = 256 + 0x0072; // GLUT_KEY_CTRL_L
    io.KeyMap[ImGuiKey_LeftAlt]         = 256 + 0x0074; // GLUT_KEY_ALT_L
    io.KeyMap[ImGuiKey_LeftSuper]       = 0; // not supported
    io.KeyMap[ImGuiKey_RightShift]      = 256 + 0x0071; // GLUT_KEY_SHIFT_R
    io.KeyMap[ImGuiKey_RightControl]    = 256 + 0x0073; // GLUT_KEY_CTRL_R
    io.KeyMap[ImGuiKey_RightAlt]        = 256 + 0x0075; // GLUT_KEY_ALT_R
    io.KeyMap[ImGuiKey_RightSuper]      = 0; // not supported
    io.KeyMap[ImGuiKey_Menu]            = 0; // not supported
    io.KeyMap[ImGuiKey_0]               = '0';
    io.KeyMap[ImGuiKey_1]               = '1';
    io.KeyMap[ImGuiKey_2]               = '2';
    io.KeyMap[ImGuiKey_3]               = '3';
    io.KeyMap[ImGuiKey_4]               = '4';
    io.KeyMap[ImGuiKey_5]               = '5';
    io.KeyMap[ImGuiKey_6]               = '6';
    io.KeyMap[ImGuiKey_7]               = '7';
    io.KeyMap[ImGuiKey_8]               = '8';
    io.KeyMap[ImGuiKey_9]               = '9';
    io.KeyMap[ImGuiKey_A]               = 'A';
    io.KeyMap[ImGuiKey_B]               = 'B';
    io.KeyMap[ImGuiKey_C]               = 'C';
    io.KeyMap[ImGuiKey_D]               = 'D';
    io.KeyMap[ImGuiKey_E]               = 'E';
    io.KeyMap[ImGuiKey_F]               = 'F';
    io.KeyMap[ImGuiKey_G]               = 'G';
    io.KeyMap[ImGuiKey_H]               = 'H';
    io.KeyMap[ImGuiKey_I]               = 'I';
    io.KeyMap[ImGuiKey_J]               = 'J';
    io.KeyMap[ImGuiKey_K]               = 'K';
    io.KeyMap[ImGuiKey_L]               = 'L';
    io.KeyMap[ImGuiKey_M]               = 'M';
    io.KeyMap[ImGuiKey_N]               = 'N';
    io.KeyMap[ImGuiKey_O]               = 'O';
    io.KeyMap[ImGuiKey_P]               = 'P';
    io.KeyMap[ImGuiKey_Q]               = 'Q';
    io.KeyMap[ImGuiKey_R]               = 'R';
    io.KeyMap[ImGuiKey_S]               = 'S';
    io.KeyMap[ImGuiKey_T]               = 'T';
    io.KeyMap[ImGuiKey_U]               = 'U';
    io.KeyMap[ImGuiKey_V]               = 'V';
    io.KeyMap[ImGuiKey_W]               = 'W';
    io.KeyMap[ImGuiKey_X]               = 'X';
    io.KeyMap[ImGuiKey_Y]               = 'Y';
    io.KeyMap[ImGuiKey_Z]               = 'Z';
    io.KeyMap[ImGuiKey_F1]              = 256 + GLUT_KEY_F1;
    io.KeyMap[ImGuiKey_F2]              = 256 + GLUT_KEY_F2;
    io.KeyMap[ImGuiKey_F3]              = 256 + GLUT_KEY_F3;
    io.KeyMap[ImGuiKey_F4]              = 256 + GLUT_KEY_F4;
    io.KeyMap[ImGuiKey_F5]              = 256 + GLUT_KEY_F5;
    io.KeyMap[ImGuiKey_F6]              = 256 + GLUT_KEY_F6;
    io.KeyMap[ImGuiKey_F7]              = 256 + GLUT_KEY_F7;
    io.KeyMap[ImGuiKey_F8]              = 256 + GLUT_KEY_F8;
    io.KeyMap[ImGuiKey_F9]              = 256 + GLUT_KEY_F9;
    io.KeyMap[ImGuiKey_F10]             = 256 + GLUT_KEY_F10;
    io.KeyMap[ImGuiKey_F11]             = 256 + GLUT_KEY_F11;
    io.KeyMap[ImGuiKey_F12]             = 256 + GLUT_KEY_F12;

    return true;
}

void ImGui_ImplGLUT_InstallFuncs()
{
    glutReshapeFunc(ImGui_ImplGLUT_ReshapeFunc);
    glutMotionFunc(ImGui_ImplGLUT_MotionFunc);
    glutPassiveMotionFunc(ImGui_ImplGLUT_MotionFunc);
    glutMouseFunc(ImGui_ImplGLUT_MouseFunc);
#ifdef __FREEGLUT_EXT_H__
    glutMouseWheelFunc(ImGui_ImplGLUT_MouseWheelFunc);
#endif
    glutKeyboardFunc(ImGui_ImplGLUT_KeyboardFunc);
    glutKeyboardUpFunc(ImGui_ImplGLUT_KeyboardUpFunc);
    glutSpecialFunc(ImGui_ImplGLUT_SpecialFunc);
    glutSpecialUpFunc(ImGui_ImplGLUT_SpecialUpFunc);
}

void ImGui_ImplGLUT_Shutdown()
{
}

void ImGui_ImplGLUT_NewFrame()
{
    // Setup time step
    ImGuiIO& io = ImGui::GetIO();
    int current_time = glutGet(GLUT_ELAPSED_TIME);
    int delta_time_ms = (current_time - g_Time);
    if (delta_time_ms <= 0)
        delta_time_ms = 1;
    io.DeltaTime = delta_time_ms / 1000.0f;
    g_Time = current_time;

    // Start the frame
    ImGui::NewFrame();
}

static void ImGui_ImplGLUT_UpdateKeyboardMods()
{
    ImGuiIO& io = ImGui::GetIO();
    int mods = glutGetModifiers();
    io.KeyCtrl = (mods & GLUT_ACTIVE_CTRL) != 0;
    io.KeyShift = (mods & GLUT_ACTIVE_SHIFT) != 0;
    io.KeyAlt = (mods & GLUT_ACTIVE_ALT) != 0;
}

void ImGui_ImplGLUT_KeyboardFunc(unsigned char c, int x, int y)
{
    // Send character to imgui
    //printf("char_down_func %d '%c'\n", c, c);
    ImGuiIO& io = ImGui::GetIO();
    if (c >= 32)
        io.AddInputCharacter((unsigned int)c);

    // Store letters in KeysDown[] array as both uppercase and lowercase + Handle GLUT translating CTRL+A..CTRL+Z as 1..26.
    // This is a hacky mess but GLUT is unable to distinguish e.g. a TAB key from CTRL+I so this is probably the best we can do here.
    if (c >= 1 && c <= 26)
        io.KeysDown[c] = io.KeysDown[c - 1 + 'a'] = io.KeysDown[c - 1 + 'A'] = true;
    else if (c >= 'a' && c <= 'z')
        io.KeysDown[c] = io.KeysDown[c - 'a' + 'A'] = true;
    else if (c >= 'A' && c <= 'Z')
        io.KeysDown[c] = io.KeysDown[c - 'A' + 'a'] = true;
    else
        io.KeysDown[c] = true;
    ImGui_ImplGLUT_UpdateKeyboardMods();
    (void)x; (void)y; // Unused
}

void ImGui_ImplGLUT_KeyboardUpFunc(unsigned char c, int x, int y)
{
    //printf("char_up_func %d '%c'\n", c, c);
    ImGuiIO& io = ImGui::GetIO();
    if (c >= 1 && c <= 26)
        io.KeysDown[c] = io.KeysDown[c - 1 + 'a'] = io.KeysDown[c - 1 + 'A'] = false;
    else if (c >= 'a' && c <= 'z')
        io.KeysDown[c] = io.KeysDown[c - 'a' + 'A'] = false;
    else if (c >= 'A' && c <= 'Z')
        io.KeysDown[c] = io.KeysDown[c - 'A' + 'a'] = false;
    else
        io.KeysDown[c] = false;
    ImGui_ImplGLUT_UpdateKeyboardMods();
    (void)x; (void)y; // Unused
}

void ImGui_ImplGLUT_SpecialFunc(int key, int x, int y)
{
    //printf("key_down_func %d\n", key);
    ImGuiIO& io = ImGui::GetIO();
    if (key + 256 < IM_ARRAYSIZE(io.KeysDown))
        io.KeysDown[key + 256] = true;
    ImGui_ImplGLUT_UpdateKeyboardMods();
    (void)x; (void)y; // Unused
}

void ImGui_ImplGLUT_SpecialUpFunc(int key, int x, int y)
{
    //printf("key_up_func %d\n", key);
    ImGuiIO& io = ImGui::GetIO();
    if (key + 256 < IM_ARRAYSIZE(io.KeysDown))
        io.KeysDown[key + 256] = false;
    ImGui_ImplGLUT_UpdateKeyboardMods();
    (void)x; (void)y; // Unused
}

void ImGui_ImplGLUT_MouseFunc(int glut_button, int state, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
    int button = -1;
    if (glut_button == GLUT_LEFT_BUTTON) button = 0;
    if (glut_button == GLUT_RIGHT_BUTTON) button = 1;
    if (glut_button == GLUT_MIDDLE_BUTTON) button = 2;
    if (button != -1 && state == GLUT_DOWN)
        io.MouseDown[button] = true;
    if (button != -1 && state == GLUT_UP)
        io.MouseDown[button] = false;
}

#ifdef __FREEGLUT_EXT_H__
void ImGui_ImplGLUT_MouseWheelFunc(int button, int dir, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
    if (dir > 0)
        io.MouseWheel += 1.0;
    else if (dir < 0)
        io.MouseWheel -= 1.0;
    (void)button; // Unused
}
#endif

void ImGui_ImplGLUT_ReshapeFunc(int w, int h)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
}

void ImGui_ImplGLUT_MotionFunc(int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
}
