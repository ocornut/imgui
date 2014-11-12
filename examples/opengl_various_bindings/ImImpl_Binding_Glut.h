#ifndef IMIMPL_BINDING_H
#define IMIMPL_BINDING_H

#ifdef _WIN32
#pragma warning (disable: 4996)         // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#include <Windows.h>
#include <Imm.h>
#endif
#include <imgui.h>


// glew (optional) & glut
#ifdef IMIMPL_USE_GLEW
#ifdef __APPLE__   // or __MACOSX__ ?
#   include <OpenGL/glew.h>             // guessing...
#else //__APPLE
#   include <GL/glew.h>
#endif //__APPLE
#else //IMIMPL_USE_GLEW
#define GL_GLEXT_PROTOTYPES
#endif //IMIMPL_USE_GLEW
#ifdef __APPLE__   // or __MACOSX__ ?
#   include <OpenGL/glut.h>             // guessing...
#else //__APPLE
#   include <GL/glut.h>
#endif //__APPLE
#ifdef __FREEGLUT_STD_H__
#ifdef __APPLE__   // or __MACOSX__ ?
#   include <OpenGL/freeglut_ext.h>     // guessing...
#else //__APPLE
#   include <GL/freeglut_ext.h>
#endif //__APPLE
#endif //__FREEGLUT_STD_H__

#include "ImImpl_RenderDrawLists.h"



static ImVec2 mousePosScale(1.0f, 1.0f);

// NB: ImGui already provide OS clipboard support for Windows so this isn't needed if you are using Windows only.
#ifndef _WIN32
// If somebody implements these, in InitImGui(...) these callbacks MUST be set (they're currently detached).
// The default fallback on non-Windows OS is a "private" clipboard.
static const char* ImImpl_GetClipboardTextFn()
{
    //fprintf(stderr,"ImImpl_GetClipboardTextFn()\n");
    static char* txt = "copy and paste not implemented in the glut backend!";
    //return SDL_GetClipboardText();
    return txt;
}
static void ImImpl_SetClipboardTextFn(const char* text)
{
    //fprintf(stderr,"ImImpl_SetClipboardTextFn()\n");
    //SDL_SetClipboardText(text);
}
#endif //_WIN32

// TODO: once we can extract the HWND from glut...
/*
#ifdef _WIN32
// Notify OS Input Method Editor of text input position (e.g. when using Japanese/Chinese inputs, otherwise this isn't needed)
static void ImImpl_ImeSetInputScreenPosFn(int x, int y)
{
    HWND hwnd = glfwGetWin32Window(window);
    if (HIMC himc = ImmGetContext(hwnd))
    {
        COMPOSITIONFORM cf;
        cf.ptCurrentPos.x = x;
        cf.ptCurrentPos.y = y;
        cf.dwStyle = CFS_FORCE_POSITION;
        ImmSetCompositionWindow(himc, &cf);
    }
}
#endif
*/

static void GlutReshapeFunc(int w,int h)    {
    int fb_w, fb_h;
    fb_w = w;
    fb_h = h;
    //glfwGetFramebufferSize(window, &fb_w, &fb_h);
    mousePosScale.x = 1.f;//(float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
    mousePosScale.y = 1.f;//(float)fb_h / h;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.

    ResizeGL(w,h);
}
static void GlutEntryFunc(int a)   {
    //fprintf(stderr,"GlutEntryFunc %d\n",a);
    if (a==0){
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos.x = io.MousePos.y = -1;
    }
}
static bool gImGuiAppIconized = false;  // stays always false (I'm not able to detect when the user minimizes a window)
/*
// This method gets never called on my system:
static void GlutVisibilityFunc(int a)   {
    fprintf(stderr,"GlutVisibilityFunc %d\n",a);
    gImGuiAppIconized = a == GL_TRUE;
}
// This gets called, but does not provide the info I'm looking for
static void GlutWindowStatusFunc(int a)   {
    fprintf(stderr,"GlutWindowStatusFunc %d\n",a);
    //gImGuiAppIconized = a == GL_TRUE;
}*/
static inline void GlutSpecialUpDown(int key,int x,int y,bool down)   {

    ImGuiIO& io = ImGui::GetIO();

    const int mods = glutGetModifiers();
    io.KeyCtrl = (mods&GLUT_ACTIVE_CTRL) != 0;
    io.KeyShift = (mods&GLUT_ACTIVE_SHIFT) != 0;
    io.MousePos.x = x;io.MousePos.y = y;

    if (key>=0 && key<512) io.KeysDown[key] = down;
}
static void GlutSpecial(int key,int x,int y)   {
    GlutSpecialUpDown(key,x,y,true);
}
static void GlutSpecialUp(int key,int x,int y)   {
    GlutSpecialUpDown(key,x,y,false);
}
static inline void GlutKeyboardUpDown(unsigned char key,int x,int y,bool down)   {

    ImGuiIO& io = ImGui::GetIO();

    const int mods = glutGetModifiers();
    io.KeyCtrl = (mods&GLUT_ACTIVE_CTRL) != 0;
    io.KeyShift = (mods&GLUT_ACTIVE_SHIFT) != 0;
    io.MousePos.x = x;io.MousePos.y = y;

    if (    key!=GLUT_KEY_LEFT && key!=GLUT_KEY_RIGHT &&
            key!=GLUT_KEY_UP && key!=GLUT_KEY_DOWN &&
            key!=GLUT_KEY_HOME && key!=GLUT_KEY_END
        )   {
        io.KeysDown[key] = down;
    }

    if (down) io.AddInputCharacter(key);
}
static void GlutKeyboard(unsigned char key,int x,int y)   {
    GlutKeyboardUpDown(key,x,y,true);
}
static void GlutKeyboardUp(unsigned char key,int x,int y)   {
    GlutKeyboardUpDown(key,x,y,false);
}
static bool gImGuiBindingMouseDblClicked[5]={false,false,false,false,false};
static void GlutMouse(int b,int s,int x,int y)  {
    //fprintf(stderr,"GlutMouse(%d,%d,%d,%d);\n",b,s,x,y);
    ImGuiIO& io = ImGui::GetIO();
    const int mods = glutGetModifiers();
    io.KeyCtrl = (mods&GLUT_ACTIVE_CTRL) != 0;
    io.KeyShift = (mods&GLUT_ACTIVE_SHIFT) != 0;
    io.MousePos.x = x;io.MousePos.y = y;
    if (b>=0 && b<5)    {
        io.MouseDown[b] = (s==0);
#       ifndef _WIN32
        if (s==0)   io.MouseWheel = b==3 ? 1 : b==4 ? -1 : 0;
#       endif //_WIN32
        // Manual double click handling:
        static double dblClickTimes[6]={-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX};  // seconds
        if (s == 0)   {
            double time = glutGet(GLUT_ELAPSED_TIME);
            double& oldTime = dblClickTimes[b];
            bool& mouseDoubleClicked = gImGuiBindingMouseDblClicked[b];
            if (time - oldTime < io.MouseDoubleClickTime*1000.f) {
                mouseDoubleClicked = true;
                oldTime = -FLT_MAX;
                //fprintf(stderr,"Double Clicked button %d\n",button);
            }
            else {
                //fprintf(stderr,"Not Double Clicked button %d (%1.4f < %1.4f)\n",button,(float)time-(float)oldTime,io.MouseDoubleClickTime);
                mouseDoubleClicked = false;
                oldTime = time;
            }
        }
    }
}
#ifdef _WIN32
static void GlutMouseWheel(int b,int s,int x,int y)  {
    //fprintf(stderr,"GlutMouseWheel(%d,%d,%d,%d);\n",b,s,x,y);
    ImGuiIO& io = ImGui::GetIO();
    const int mods = glutGetModifiers();
    io.KeyCtrl = (mods&GLUT_ACTIVE_CTRL) != 0;
    io.KeyShift = (mods&GLUT_ACTIVE_SHIFT) != 0;
    io.MousePos.x = x;io.MousePos.y = y;
    // NEVER TESTED !!!!!!!!
    if (s==0)   io.MouseWheel = b==0 ? 1 : b==1 ? -1 : 0;
}
#endif //_WIN32

static void GlutMotion(int x,int y)  {
    ImGuiIO& io = ImGui::GetIO();
    //const int mods = glutGetModifiers();
    //io.KeyCtrl = (mods&GLUT_ACTIVE_CTRL) != 0;
    //io.KeyShift = (mods&GLUT_ACTIVE_SHIFT) != 0;
    io.MousePos.x = x;io.MousePos.y = y;
}
static void GlutPassiveMotion(int x,int y)  {
    ImGuiIO& io = ImGui::GetIO();
    //const int mods = glutGetModifiers();
    //io.KeyCtrl = (mods&GLUT_ACTIVE_CTRL) != 0;
    //io.KeyShift = (mods&GLUT_ACTIVE_SHIFT) != 0;
    io.MousePos.x = x;io.MousePos.y = y;
}
static void GlutDrawGL()    {
    ImGuiIO& io = ImGui::GetIO();    
    if (gImGuiAppIconized) WaitFor(1000);

    // Setup timestep
    static double time = 0.0f;
    const double current_time =  (double) glutGet(GLUT_ELAPSED_TIME);
    io.DeltaTime = (float) ((current_time - time)*0.001);
    time = current_time;
    if (io.DeltaTime<=0) io.DeltaTime = (1.0f/60.0f);

    // If needed we must wait (gImGuiInverseFPSClamp-io.DeltaTime) seconds (=> honestly I shouldn't add the * 2.0f factor at the end, but ImGui tells me the wrong FPS otherwise... why? <=)
    if (gImGuiInverseFPSClamp>0 && io.DeltaTime < gImGuiInverseFPSClamp)  WaitFor((unsigned int) ((gImGuiInverseFPSClamp-io.DeltaTime)*1000.f * 2.0f) );

    if (!gImGuiPaused)	{
        // Start the frame
        ImGui::NewFrame();
        for (size_t i = 0; i < 5; i++) {
            io.MouseDoubleClicked[i]=gImGuiBindingMouseDblClicked[i];   // We manually set it (otherwise it won't work with low frame rates)
        }
    }

    DrawGL();

    if (!gImGuiPaused)	{
        // Rendering
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        //glClearColor(0.8f, 0.6f, 0.6f, 1.0f);
        //glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
    }

    glutSwapBuffers();
    if (!gImGuiPaused)	for (size_t i = 0; i < 5; i++) gImGuiBindingMouseDblClicked[i] = false;   // We manually set it (otherwise it won't work with low frame rates)
}
static void GlutIdle(void)  {
   glutPostRedisplay(); // TODO: Well, we could sleep a bit here if we detect that the window is minimized...
}
static void GlutFakeDrawGL() {
   glutDisplayFunc(GlutDrawGL);
}


static void InitImGui(const ImImpl_InitParams* pOptionalInitParams=NULL)    {
    //fprintf(stderr,"InitImGui(%d,%d);\n",w,h);
    //int w, h;
    int fb_w, fb_h;
    fb_w = glutGet(GLUT_WINDOW_WIDTH);
    fb_h = glutGet(GLUT_WINDOW_HEIGHT);
    //glfwGetFramebufferSize(window, &fb_w, &fb_h);
    mousePosScale.x = 1.f;//(float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
    mousePosScale.y = 1.f;//(float)fb_h / h;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.
    io.DeltaTime = 1.0f/60.0f;                          // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
    io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels

    // Set up ImGui
    io.KeyMap[ImGuiKey_Tab] = 9;    // tab (ascii)

    io.KeyMap[ImGuiKey_LeftArrow] =     GLUT_KEY_LEFT;    // Left
    io.KeyMap[ImGuiKey_RightArrow] =    GLUT_KEY_RIGHT;   // Right
    io.KeyMap[ImGuiKey_UpArrow] =       GLUT_KEY_UP;      // Up
    io.KeyMap[ImGuiKey_DownArrow] =     GLUT_KEY_DOWN;    // Down
    io.KeyMap[ImGuiKey_Home] =          GLUT_KEY_HOME;    // Home
    io.KeyMap[ImGuiKey_End] =           GLUT_KEY_END;     // End

    io.KeyMap[ImGuiKey_Delete] =    127;      // Delete  (ascii) (0x006F)
    io.KeyMap[ImGuiKey_Backspace] = 8;        // Backspace  (ascii)
    io.KeyMap[ImGuiKey_Enter] = 13;           // Enter  (ascii)
    io.KeyMap[ImGuiKey_Escape] = 27;          // Escape  (ascii)
    io.KeyMap[ImGuiKey_A] = 1;
    io.KeyMap[ImGuiKey_C] = 3;
    io.KeyMap[ImGuiKey_V] = 22;
    io.KeyMap[ImGuiKey_X] = 24;
    io.KeyMap[ImGuiKey_Y] = 25;
    io.KeyMap[ImGuiKey_Z] = 26;

    io.RenderDrawListsFn = ImImpl_RenderDrawLists;
#ifndef _WIN32
    //io.SetClipboardTextFn = ImImpl_SetClipboardTextFn;
    //io.GetClipboardTextFn = ImImpl_GetClipboardTextFn;
#endif
#ifdef _WIN32
    //io.ImeSetInputScreenPosFn = ImImpl_ImeSetInputScreenPosFn;
#endif

    // 3 common init steps
    InitImGuiFontTexture(pOptionalInitParams);
    InitImGuiProgram();
    InitImGuiBuffer();
}

static bool InitBinding(const ImImpl_InitParams* pOptionalInitParams=NULL,int argc=0, char** argv=NULL)	{

//-ENABLE-OPENGLES COMPATIBILITY PROFILES----------------------------------
/* // Don't know how to make this work... for sure,
 * // "custom" freeglut compiled with -legl -lglesv2 can do it,
 * // but otherwie it seems that the required profile definition is missing ATM.
 * // Moreover, I'm not sure that the glutInitContextMethods (defined in freeglut_ext.h)
 * // can be called at this point...
*/
/*
#ifndef IMIMPL_SHADER_NONE
#ifdef IMIMPL_SHADER_GLES
#   ifndef IMIMPL_SHADER_GL3
    glutInitContextVersion( 2, 0);
#   else //IMIMPL_SHADER_GL3
    glutInitContextVersion( 3, 0);
#   endif //MIMPL_SHADER_GL3
    //glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);                          // GLFW3 can do it!
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);   // SDL2 can do it too!

    // What about freeglut?
    glutInitContextProfile( GLUT_ES_PROFILE ); //GLUT_CORE_PROFILE GLUT_COMPATIBILITY_PROFILE
    //glutInitContextVersion( int majorVersion, int minorVersion );
    //glutInitContextFlags( int flags );  //GLUT_DEBUG  GLUT_FORWARD_COMPATIBLE
    //glutInitContextProfile( int profile ); //GLUT_CORE_PROFILE GLUT_COMPATIBILITY_PROFILE
#endif // IMIMPL_SHADER_GLES
#endif //IMIMPL_SHADER_NONE
*/
//--------------------------------------------------------------------------


    glutInitDisplayMode(GLUT_RGB | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE);// | GLUT_MULTISAMPLE | GLUT_SRGB); // The latter is defined in freeglut_ext.h
    glutInitWindowSize(pOptionalInitParams ? pOptionalInitParams->gWindowSize.x : 1270, pOptionalInitParams ? pOptionalInitParams->gWindowSize.y : 720);
    //int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
    //glutInitWindowPosition(5 * screenWidth/ 12, 0);
    glutInit(&argc, argv);
    if (!glutCreateWindow((pOptionalInitParams && pOptionalInitParams->gWindowTitle[0]!='\0') ? (const char*) &pOptionalInitParams->gWindowTitle[0] : "ImGui Glut OpenGL example"))
    {
        fprintf(stderr, "Could not call glutCreateWindow(...) successfully.\n");
        return false;
    }

    //OpenGL info
    {
        printf("GL Vendor: %s\n", glGetString( GL_VENDOR ));
        printf("GL Renderer : %s\n", glGetString( GL_RENDERER ));
        printf("GL Version (string) : %s\n",  glGetString( GL_VERSION ));
        printf("GLSL Version : %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ));
        //printf("GL Extensions:\n%s\n",(char *) glGetString(GL_EXTENSIONS));
    }

#ifdef IMIMPL_USE_GLEW
    GLenum err = glewInit();
    if( GLEW_OK != err )
    {
        fprintf(stderr, "Error initializing GLEW: %s\n",
                glewGetErrorString(err) );
        return false;
    }
#endif //IMIMPL_USE_GLEW


    // Set up glut callbacks
    glutIdleFunc(GlutIdle);
    glutReshapeFunc(GlutReshapeFunc);
    //glutDisplayFunc(GlutDrawGL);

    //glutInitContextFunc(InitGL);   // freeglut3 only: call order:  ResizeGL() - InitGL() - DrawGL()
    //InitGL();                      // call order:                  InitGL() - DrawGL() - ResizeGL()

    //But I prefer call order: InitGL() - ResizeGL() - DrawGL():
    // this can be achieved by skipping the first DrawGL call this way:
    glutDisplayFunc(GlutFakeDrawGL);

    glutKeyboardFunc(GlutKeyboard);
    glutKeyboardUpFunc(GlutKeyboardUp);
    glutSpecialFunc(GlutSpecial);
    glutSpecialUpFunc(GlutSpecialUp);
    glutMouseFunc(GlutMouse);               // This handles mouse wheel as well on non-Windows systems
    glutMotionFunc(GlutMotion);
    glutPassiveMotionFunc(GlutPassiveMotion);

    glutEntryFunc(GlutEntryFunc);

    // Apparently, there's no way to detect when the user minimizes a window using glut (at least on my system these callbacks don't provide this info)
    //glutVisibilityFunc(GlutVisibilityFunc);       // never called
    //glutWindowStatusFunc(GlutWindowStatusFunc);   // called on resizing too

#ifdef _WIN32
    glutMouseWheelFunc(GlutMouseWheel);
#endif //_WIN32

	return true;
}

// Application code
int ImImpl_Main(const ImImpl_InitParams* pOptionalInitParams=NULL,int argc=0, char** argv=NULL)
{
    if (!InitBinding(pOptionalInitParams,argc,argv)) return -1;	
    InitImGui(pOptionalInitParams);
    ImGuiIO& io = ImGui::GetIO();        

    gImGuiInverseFPSClamp = pOptionalInitParams ? ((pOptionalInitParams->gFpsClamp!=0) ? (1.0f/pOptionalInitParams->gFpsClamp) : 1.0f) : -1.0f;
    
    InitGL();
    ResizeGL((int)io.DisplaySize.x,(int)io.DisplaySize.y);
	


#ifdef __FREEGLUT_EXT_H__
    glutSetOption ( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION ) ;
#endif //__FREEGLUT_STD_H__

    glutMainLoop();     // GLUT has its own main loop, which calls display();

    ImGui::Shutdown();
    DestroyGL();
    DestroyImGuiFontTexture();
    DestroyImGuiProgram();
    DestroyImGuiBuffer();

    return 0;
}

#endif //#ifndef IMIMPL_BINDING_H

