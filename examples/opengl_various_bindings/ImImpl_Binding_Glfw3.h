#ifndef IMIMPL_BINDING_H
#define IMIMPL_BINDING_H

#ifdef _WIN32
#pragma warning (disable: 4996)         // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#include <Windows.h>
#include <Imm.h>
#endif
#include <imgui.h>


// glew (optional) & glfw
#ifdef IMIMPL_USE_GLEW
//#define GLEW_STATIC		// Optional, depending on which glew lib you want to use
#ifdef __APPLE__   // or __MACOSX__ ?
#   include <OpenGL/glew.h>     // guessing...
#else //__APPLE
#   include <GL/glew.h>
#endif //__APPLE
#else //IMIMPL_USE_GLEW
#define GL_GLEXT_PROTOTYPES
#endif //IMIMPL_USE_GLEW
//#define GLFW_STATIC
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include "ImImpl_RenderDrawLists.h"




static GLFWwindow* window;
//static bool mousePressed[2] = { false, false };
static ImVec2 mousePosScale(1.0f, 1.0f);


// NB: ImGui already provide OS clipboard support for Windows so this isn't needed if you are using Windows only.
static const char* ImImpl_GetClipboardTextFn()
{
    return glfwGetClipboardString(window);
}

static void ImImpl_SetClipboardTextFn(const char* text)
{
    glfwSetClipboardString(window, text);
}

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

// GLFW callbacks to get events
static void glfw_error_callback(int error, const char* description)	{
    fputs(description, stderr);
}
static bool gImGuiAppIsIconified = false;
static void glfw_window_iconify_callback(GLFWwindow* window,int iconified)    {
    gImGuiAppIsIconified = iconified == GL_TRUE;
}
static void glfw_framebuffer_size_callback(GLFWwindow* window,int fb_w,int fb_h)  {
    int w, h;glfwGetWindowSize(window, &w, &h);
    mousePosScale.x = (float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
    mousePosScale.y = (float)fb_h / h;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);
}
static void glfw_window_size_callback(GLFWwindow* window,int w,int h)  {
    int fb_w, fb_h;glfwGetFramebufferSize(window, &fb_w, &fb_h);
    mousePosScale.x = (float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
    mousePosScale.y = (float)fb_h / h;
    ResizeGL(w,h);
}
static bool gImGuiBindingMouseDblClicked[5]={false,false,false,false,false};
static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)	{
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < 5) {
        io.MouseDown[button] = (action == GLFW_PRESS);
        // Manual double click handling:
        static double dblClickTimes[6]={-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX};  // seconds
        if (action == GLFW_PRESS)   {
            double time = glfwGetTime();
            double& oldTime = dblClickTimes[button];
            bool& mouseDoubleClicked = gImGuiBindingMouseDblClicked[button];
            if (time - oldTime < io.MouseDoubleClickTime) {
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
    io.KeyCtrl = (mods & GLFW_MOD_CONTROL);
    io.KeyShift = (mods & GLFW_MOD_SHIFT);

}
static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)	{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel = (yoffset != 0.0f) ? yoffset > 0.0f ? 1 : - 1 : 0;           // Mouse wheel: -1,0,+1
}
static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)	{
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)   io.KeysDown[key] = true;
    if (action == GLFW_RELEASE) io.KeysDown[key] = false;
    io.KeyCtrl = (mods & GLFW_MOD_CONTROL);
    io.KeyShift = (mods & GLFW_MOD_SHIFT);
}
static void glfw_char_callback(GLFWwindow* window, unsigned int c)	{
    if (c > 0 && c < 0x10000)
        ImGui::GetIO().AddInputCharacter((unsigned short)c);
}
static void glfw_mouse_enter_leave_callback(GLFWwindow* window, int entered)	{
    if (entered==GL_FALSE) {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos.x=io.MousePos.y=-1.f;
    }
}
static void glfw_mouse_move_callback(GLFWwindow* window, double x,double y)	{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x * mousePosScale.x, (float)y * mousePosScale.y);      // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
}
static void InitImGui(const ImImpl_InitParams* pOptionalInitParams=NULL)	{
    int w, h;
    int fb_w, fb_h;
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    mousePosScale.x = (float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
    mousePosScale.y = (float)fb_h / h;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.
    io.DeltaTime = 1.0f/60.0f;                          // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
    io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;             // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.RenderDrawListsFn = ImImpl_RenderDrawLists;
    io.SetClipboardTextFn = ImImpl_SetClipboardTextFn;
    io.GetClipboardTextFn = ImImpl_GetClipboardTextFn;
#ifdef _WIN32
    io.ImeSetInputScreenPosFn = ImImpl_ImeSetInputScreenPosFn;
#endif

    // 3 common init steps
    InitImGuiFontTexture(pOptionalInitParams);
    InitImGuiProgram();
    InitImGuiBuffer();
}

static bool InitBinding(const ImImpl_InitParams* pOptionalInitParams=NULL,int argc=0, char** argv=NULL)	{
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())    {
        fprintf(stderr, "Could not call glfwInit(...) successfully.\n");
        return false;
    }

//-ENABLE-OPENGLES COMPATIBILITY PROFILES----------------------------------
#ifndef IMIMPL_SHADER_NONE
#ifdef IMIMPL_SHADER_GLES
#   ifndef IMIMPL_SHADER_GL3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);          // 1 => ES1.1   2 => ES2.0  3 => ES3.0
#   else //IMIMPL_SHADER_GL3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
#   endif //MIMPL_SHADER_GL3
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#endif // IMIMPL_SHADER_GLES
#endif //IMIMPL_SHADER_NONE
//--------------------------------------------------------------------------
    //glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

    if (pOptionalInitParams && pOptionalInitParams->gWindowTitle[0]!='\0')  window = glfwCreateWindow(pOptionalInitParams ? pOptionalInitParams->gWindowSize.x : 1270, pOptionalInitParams ? pOptionalInitParams->gWindowSize.y : 720,(const char*) &pOptionalInitParams->gWindowTitle[0], NULL, NULL);
    else		window = glfwCreateWindow(pOptionalInitParams ? pOptionalInitParams->gWindowSize.x : 1270, pOptionalInitParams ? pOptionalInitParams->gWindowSize.y : 720, "ImGui Glfw3 OpenGL example", NULL, NULL);
    if (!window)    {
        fprintf(stderr, "Could not call glfwCreateWindow(...) successfully.\n");
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    glfwSetScrollCallback(window, glfw_scroll_callback);
    glfwSetCharCallback(window, glfw_char_callback);
    glfwSetCursorPosCallback(window, glfw_mouse_move_callback);
    glfwSetCursorEnterCallback(window, glfw_mouse_enter_leave_callback);

    glfwSetWindowSizeCallback(window, glfw_window_size_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
    glfwSetWindowIconifyCallback(window, glfw_window_iconify_callback);

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

	return true;
}

// Application code
int ImImpl_Main(const ImImpl_InitParams* pOptionalInitParams=NULL,int argc=0, char** argv=NULL)
{
    if (!InitBinding(pOptionalInitParams,argc,argv)) return -1;
    InitImGui(pOptionalInitParams);
    ImGuiIO& io = ImGui::GetIO();        
    
    InitGL();
 	ResizeGL(io.DisplaySize.x,io.DisplaySize.y);
	
    static double time = 0.0f;

    gImGuiInverseFPSClamp = pOptionalInitParams ? ((pOptionalInitParams->gFpsClamp!=0) ? (1.0f/pOptionalInitParams->gFpsClamp) : 1.0f) : -1.0f;
	
    while (!glfwWindowShouldClose(window))
    {
        if (!gImGuiPaused)	for (size_t i = 0; i < 5; i++) gImGuiBindingMouseDblClicked[i] = false;   // We manually set it (otherwise it won't work with low frame rates)
        if (gImGuiAppIsIconified) {
            //fprintf(stderr,"glfwWaitEvents() Start %1.4f\n",glfwGetTime());
            glfwWaitEvents();
            //fprintf(stderr,"glfwWaitEvents() End %1.4f\n",glfwGetTime());
        }
        else glfwPollEvents();
  
    	// Setup timestep    	
    	const double current_time =  glfwGetTime();
        io.DeltaTime = (float)(current_time - time);
	    time = current_time;      
                
        // If needed we must wait (inverseFPSClamp-io.DeltaTime) seconds (=> honestly I shouldn't add the * 2.0f factor at the end, but ImGui tells me the wrong FPS otherwise... why? <=)
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

        glfwSwapBuffers(window);
    }

    DestroyGL();
    ImGui::Shutdown();
    DestroyImGuiFontTexture();
    DestroyImGuiProgram();
    DestroyImGuiBuffer();
    glfwTerminate();
    return 0;
}

#endif //#ifndef IMIMPL_BINDING_H

