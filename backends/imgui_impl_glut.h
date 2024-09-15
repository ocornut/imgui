// dear imgui: Platform Backend for GLUT/FreeGLUT
// This needs to be used along with a Renderer (e.g. OpenGL2)

// !!! GLUT/FreeGLUT IS OBSOLETE PREHISTORIC SOFTWARE. Using GLUT is not recommended unless you really miss the 90's. !!!
// !!! If someone or something is teaching you GLUT today, you are being abused. Please show some resistance. !!!
// !!! Nowadays, prefer using GLFW or SDL instead!

// Implemented features:
//  [X] Platform: Partial keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy GLUT values will also be supported unless IMGUI_DISABLE_OBSOLETE_KEYIO is set]
// Issues:
//  [ ] Platform: GLUT is unable to distinguish e.g. Backspace from CTRL+H or TAB from CTRL+I
//  [ ] Platform: Missing horizontal mouse wheel support.
//  [ ] Platform: Missing mouse cursor shape/visibility support.
//  [ ] Platform: Missing clipboard support (not supported by Glut).
//  [ ] Platform: Missing gamepad support.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#pragma once
#ifndef IMGUI_DISABLE
#include "imgui.h"      // IMGUI_IMPL_API

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool     ImGui_ImplGLUT_Init();
IMGUI_IMPL_API void     ImGui_ImplGLUT_InstallFuncs();
IMGUI_IMPL_API void     ImGui_ImplGLUT_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplGLUT_NewFrame();

// You can call ImGui_ImplGLUT_InstallFuncs() to get all those functions installed automatically,
// or call them yourself from your own GLUT handlers. We are using the same weird names as GLUT for consistency..
//------------------------------------ GLUT name ---------------------------------------------- Decent Name ---------
IMGUI_IMPL_API void     ImGui_ImplGLUT_ReshapeFunc(int w, int h);                           // ~ ResizeFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_MotionFunc(int x, int y);                            // ~ MouseMoveFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_MouseFunc(int button, int state, int x, int y);      // ~ MouseButtonFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_MouseWheelFunc(int button, int dir, int x, int y);   // ~ MouseWheelFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_KeyboardFunc(unsigned char c, int x, int y);         // ~ CharPressedFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_KeyboardUpFunc(unsigned char c, int x, int y);       // ~ CharReleasedFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_SpecialFunc(int key, int x, int y);                  // ~ KeyPressedFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_SpecialUpFunc(int key, int x, int y);                // ~ KeyReleasedFunc

#endif // #ifndef IMGUI_DISABLE
