// ImGui Platform Binding for: FreeGLUT
// This needs to be used along with a Renderer (e.g. OpenGL2)

// Issues:
//  [ ] Platform: GLUT is unable to distinguish e.g. Backspace from CTRL+H or TAB from CTRL+I

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2018-03-22: Added FreeGLUT Platform binding.

#include "imgui.h"
#include "imgui_impl_freeglut.h"
#include <GL/freeglut.h>

#ifdef _MSC_VER
#pragma warning (disable: 4505) // unreferenced local function has been removed (stb stuff)
#endif

static int g_Time = 0;          // Current time, in milliseconds

bool ImGui_ImplFreeGLUT_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    g_Time = 0;

    // Glut has 1 function for characters and one for "special keys". We map the characters in the 0..255 range and the keys above.
    io.KeyMap[ImGuiKey_Tab]         = '\t'; // == 9 == CTRL+I
    io.KeyMap[ImGuiKey_LeftArrow]   = 256 + GLUT_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow]  = 256 + GLUT_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow]     = 256 + GLUT_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow]   = 256 + GLUT_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp]      = 256 + GLUT_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown]    = 256 + GLUT_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home]        = 256 + GLUT_KEY_HOME;
    io.KeyMap[ImGuiKey_End]         = 256 + GLUT_KEY_END;
    io.KeyMap[ImGuiKey_Insert]      = 256 + GLUT_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete]      = 127;
    io.KeyMap[ImGuiKey_Backspace]   = 8;  // == CTRL+H
    io.KeyMap[ImGuiKey_Space]       = ' ';
    io.KeyMap[ImGuiKey_Enter]       = 13; // == CTRL+M
    io.KeyMap[ImGuiKey_Escape]      = 27;
    io.KeyMap[ImGuiKey_A]           = 'A';
    io.KeyMap[ImGuiKey_C]           = 'C';
    io.KeyMap[ImGuiKey_V]           = 'V';
    io.KeyMap[ImGuiKey_X]           = 'X';
    io.KeyMap[ImGuiKey_Y]           = 'Y';
    io.KeyMap[ImGuiKey_Z]           = 'Z';

    return true;
}

void ImGui_ImplFreeGLUT_InstallFuncs()
{
    glutReshapeFunc(ImGui_ImplFreeGLUT_ReshapeFunc);
    glutMotionFunc(ImGui_ImplFreeGLUT_MotionFunc);
    glutPassiveMotionFunc(ImGui_ImplFreeGLUT_MotionFunc);
    glutMouseFunc(ImGui_ImplFreeGLUT_MouseFunc);
    glutMouseWheelFunc(ImGui_ImplFreeGLUT_MouseWheelFunc);
    glutKeyboardFunc(ImGui_ImplFreeGLUT_KeyboardFunc);
    glutKeyboardUpFunc(ImGui_ImplFreeGLUT_KeyboardUpFunc);
    glutSpecialFunc(ImGui_ImplFreeGLUT_SpecialFunc);
    glutSpecialUpFunc(ImGui_ImplFreeGLUT_SpecialUpFunc);
}

void ImGui_ImplFreeGLUT_Shutdown()
{
}

void ImGui_ImplFreeGLUT_NewFrame()
{
    // Setup time step
    ImGuiIO& io = ImGui::GetIO();
    int current_time = glutGet(GLUT_ELAPSED_TIME);
    io.DeltaTime = (current_time - g_Time) / 1000.0f;
    g_Time = current_time;

    // Start the frame
    ImGui::NewFrame();
}

static void ImGui_ImplFreeGLUT_UpdateKeyboardMods()
{
    ImGuiIO& io = ImGui::GetIO();
    int mods = glutGetModifiers();
    io.KeyCtrl = (mods & GLUT_ACTIVE_CTRL) != 0;
    io.KeyShift = (mods & GLUT_ACTIVE_SHIFT) != 0;
    io.KeyAlt = (mods & GLUT_ACTIVE_ALT) != 0;
}

void ImGui_ImplFreeGLUT_KeyboardFunc(unsigned char c, int x, int y)
{
    // Send character to imgui
    //printf("char_down_func %d '%c'\n", c, c);
    ImGuiIO& io = ImGui::GetIO();
    if (c >= 32)
        io.AddInputCharacter(c);

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
    ImGui_ImplFreeGLUT_UpdateKeyboardMods();
    (void)x; (void)y; // Unused
}

void ImGui_ImplFreeGLUT_KeyboardUpFunc(unsigned char c, int x, int y)
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
    ImGui_ImplFreeGLUT_UpdateKeyboardMods();
    (void)x; (void)y; // Unused
}

void ImGui_ImplFreeGLUT_SpecialFunc(int key, int x, int y)
{
    //printf("key_down_func %d\n", key);
    ImGuiIO& io = ImGui::GetIO();
    if (key + 256 < IM_ARRAYSIZE(io.KeysDown))
        io.KeysDown[key + 256] = true;
    ImGui_ImplFreeGLUT_UpdateKeyboardMods();
    (void)x; (void)y; // Unused
}

void ImGui_ImplFreeGLUT_SpecialUpFunc(int key, int x, int y)
{
    //printf("key_up_func %d\n", key);
    ImGuiIO& io = ImGui::GetIO();
    if (key + 256 < IM_ARRAYSIZE(io.KeysDown))
        io.KeysDown[key + 256] = false;
    ImGui_ImplFreeGLUT_UpdateKeyboardMods();
    (void)x; (void)y; // Unused
}

void ImGui_ImplFreeGLUT_MouseFunc(int glut_button, int state, int x, int y)
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

void ImGui_ImplFreeGLUT_MouseWheelFunc(int button, int dir, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
    if (dir > 0)
        io.MouseWheel += 1.0;
    else if (dir < 0)
        io.MouseWheel -= 1.0;
    (void)button; // Unused
}

void ImGui_ImplFreeGLUT_ReshapeFunc(int w, int h)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
}

void ImGui_ImplFreeGLUT_MotionFunc(int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
}
