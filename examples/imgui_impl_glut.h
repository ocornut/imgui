// dear imgui: Platform Binding for GLUT/FreeGLUT
// This needs to be used along with a Renderer (e.g. OpenGL2)

// !!! GLUT/FreeGLUT IS OBSOLETE SOFTWARE. Using GLUT is not recommended unless you really miss the 90's. !!!
// !!! If someone or something is teaching you GLUT in 2020, you are being abused. Please show some resistance. !!!
// !!! Nowadays, prefer using GLFW or SDL instead!

// Issues:
//  [ ] Platform: GLUT is unable to distinguish e.g. Backspace from CTRL+H or TAB from CTRL+I
//  [ ] Platform: Missing mouse cursor shape/visibility support.
//  [ ] Platform: Missing clipboard support (not supported by Glut).
//  [ ] Platform: Missing gamepad support.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

IMGUI_IMPL_API bool     ImGui_ImplGLUT_Init();
IMGUI_IMPL_API void     ImGui_ImplGLUT_InstallFuncs();
IMGUI_IMPL_API void     ImGui_ImplGLUT_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplGLUT_NewFrame();

// You can call ImGui_ImplGLUT_InstallFuncs() to get all those functions installed automatically,
// or call them yourself from your own GLUT handlers. We are using the same weird names as GLUT for consistency..
//---------------------------------------- GLUT name --------------------------------------------- Decent Name ---------
IMGUI_IMPL_API void     ImGui_ImplGLUT_ReshapeFunc(int w, int h);                           // ~ ResizeFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_MotionFunc(int x, int y);                            // ~ MouseMoveFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_MouseFunc(int button, int state, int x, int y);      // ~ MouseButtonFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_MouseWheelFunc(int button, int dir, int x, int y);   // ~ MouseWheelFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_KeyboardFunc(unsigned char c, int x, int y);         // ~ CharPressedFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_KeyboardUpFunc(unsigned char c, int x, int y);       // ~ CharReleasedFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_SpecialFunc(int key, int x, int y);                  // ~ KeyPressedFunc
IMGUI_IMPL_API void     ImGui_ImplGLUT_SpecialUpFunc(int key, int x, int y);                // ~ KeyReleasedFunc
