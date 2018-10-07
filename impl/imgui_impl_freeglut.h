// dear imgui: Platform Binding for FreeGLUT
// This needs to be used along with a Renderer (e.g. OpenGL2)

// Issues:
//  [ ] Platform: GLUT is unable to distinguish e.g. Backspace from CTRL+H or TAB from CTRL+I
//  [ ] Platform: Missing gamepad support.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

IMGUI_IMPL_API bool     ImGui_ImplFreeGLUT_Init();
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_InstallFuncs();
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_NewFrame();

// You can call ImGui_ImplFreeGLUT_InstallFuncs() to get all those functions installed automatically,
// or call them yourself from your own GLUT handlers. We are using the same weird names as GLUT for consistency..
//---------------------------------------- GLUT name --------------------------------------------- Decent Name ---------
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_ReshapeFunc(int w, int h);                           // ~ ResizeFunc
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_MotionFunc(int x, int y);                            // ~ MouseMoveFunc
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_MouseFunc(int button, int state, int x, int y);      // ~ MouseButtonFunc
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_MouseWheelFunc(int button, int dir, int x, int y);   // ~ MouseWheelFunc
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_KeyboardFunc(unsigned char c, int x, int y);         // ~ CharPressedFunc
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_KeyboardUpFunc(unsigned char c, int x, int y);       // ~ CharReleasedFunc
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_SpecialFunc(int key, int x, int y);                  // ~ KeyPressedFunc
IMGUI_IMPL_API void     ImGui_ImplFreeGLUT_SpecialUpFunc(int key, int x, int y);                // ~ KeyReleasedFunc
