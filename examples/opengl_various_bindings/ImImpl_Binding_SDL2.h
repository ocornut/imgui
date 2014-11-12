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
#if (!defined(IMIMPL_SHADER_GLES) || defined (IMIMPL_SHADER_NONE))
#include <SDL2/SDL_opengl.h>
#else //IMIMPL_SHADER_GLES
#include <SDL2/SDL_opengles2.h>
#endif //IMIMPL_SHADER_GLES
#endif //IMIMPL_USE_GLEW
#include <SDL2/SDL.h>

#define IMIMPL_SDL2_USE_STL   // Different handling of SDL_KEYDOWN/SDL_KEYUP events
#ifdef IMIMPL_SDL2_USE_STL
#include <map>              // we could work around this dependency if needed
static std::map<int,int> specialKeyMap;
#endif //IMIMPL_SDL2_USE_STL

#include "ImImpl_RenderDrawLists.h"

static SDL_Window* window = NULL;
static ImVec2 mousePosScale(1.0f, 1.0f);


// NB: ImGui already provide OS clipboard support for Windows so this isn't needed if you are using Windows only.
static const char* ImImpl_GetClipboardTextFn()
{
    return SDL_GetClipboardText();
}

static void ImImpl_SetClipboardTextFn(const char* text)
{
    SDL_SetClipboardText(text);
}
// TODO: once we can find out how to get a HWND from SDL2
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


static void InitImGui(const ImImpl_InitParams* pOptionalInitParams=NULL)	{
    //int w, h;
    int fb_w, fb_h;
    SDL_GetWindowSize(window, &fb_w, &fb_h);
    //glfwGetFramebufferSize(window, &fb_w, &fb_h);
    mousePosScale.x = 1.f;//(float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
    mousePosScale.y = 1.f;//(float)fb_h / h;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.
    io.DeltaTime = 1.0f/60.0f;                          // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
    io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels

#   ifdef IMIMPL_SDL2_USE_STL
    specialKeyMap.clear();const int numKeyMaps = 512;
    int cnt = 1;
    io.KeyMap[ImGuiKey_Tab] =           specialKeyMap[SDLK_TAB]              = numKeyMaps - cnt++;             // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] =     specialKeyMap[SDLK_LEFT]             = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_RightArrow] =    specialKeyMap[SDLK_RIGHT]            = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_UpArrow] =       specialKeyMap[SDLK_UP]               = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_DownArrow] =     specialKeyMap[SDLK_DOWN]             = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_Home] =          specialKeyMap[SDLK_HOME]             = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_End] =           specialKeyMap[SDLK_END]              = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_Delete] =        specialKeyMap[SDLK_DELETE]           = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_Backspace] =     specialKeyMap[SDLK_BACKSPACE]        = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_Enter] =         specialKeyMap[SDLK_RETURN]           = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_Escape] =        specialKeyMap[SDLK_ESCAPE]           = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_A] =             specialKeyMap[SDLK_a]                = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_C] =             specialKeyMap[SDLK_c]                = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_V] =             specialKeyMap[SDLK_v]                = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_X] =             specialKeyMap[SDLK_x]                = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_Y] =             specialKeyMap[SDLK_y]                = numKeyMaps - cnt++;
    io.KeyMap[ImGuiKey_Z] =             specialKeyMap[SDLK_z]                = numKeyMaps - cnt++;
#   else //IMIMPL_SDL2_USE_STL
    // thanks to mattiasljungstrom:
    io.KeyMap[ImGuiKey_Tab] = SDL_GetScancodeFromKey( SDLK_TAB );
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_GetScancodeFromKey( SDLK_LEFT );
    io.KeyMap[ImGuiKey_RightArrow] = SDL_GetScancodeFromKey( SDLK_RIGHT );
    io.KeyMap[ImGuiKey_UpArrow] = SDL_GetScancodeFromKey( SDLK_UP );
    io.KeyMap[ImGuiKey_DownArrow] = SDL_GetScancodeFromKey( SDLK_DOWN );
    io.KeyMap[ImGuiKey_Home] = SDL_GetScancodeFromKey( SDLK_HOME );
    io.KeyMap[ImGuiKey_End] = SDL_GetScancodeFromKey( SDLK_END );
    io.KeyMap[ImGuiKey_Delete] = SDL_GetScancodeFromKey( SDLK_DELETE );
    io.KeyMap[ImGuiKey_Backspace] = SDL_GetScancodeFromKey( SDLK_BACKSPACE );
    io.KeyMap[ImGuiKey_Enter] = SDL_GetScancodeFromKey( SDLK_RETURN );
    io.KeyMap[ImGuiKey_Escape] = SDL_GetScancodeFromKey( SDLK_ESCAPE );
    io.KeyMap[ImGuiKey_A] = SDLK_a;
    io.KeyMap[ImGuiKey_C] = SDLK_c;
    io.KeyMap[ImGuiKey_V] = SDLK_v;
    io.KeyMap[ImGuiKey_X] = SDLK_x;
    io.KeyMap[ImGuiKey_Y] = SDLK_y;
    io.KeyMap[ImGuiKey_Z] = SDLK_z;
#   endif //IMIMPL_SDL2_USE_STL

    io.RenderDrawListsFn = ImImpl_RenderDrawLists;
    io.SetClipboardTextFn = ImImpl_SetClipboardTextFn;
    io.GetClipboardTextFn = ImImpl_GetClipboardTextFn;
#ifdef _MSC_VER
    //io.ImeSetInputScreenPosFn = ImImpl_ImeSetInputScreenPosFn;
#endif

    // 3 common init steps
    InitImGuiFontTexture(pOptionalInitParams);
    InitImGuiProgram();
    InitImGuiBuffer();
}

static bool InitBinding(const ImImpl_InitParams* pOptionalInitParams=NULL,int argc=0, char** argv=NULL)	{

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)  {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    // Init OpenGL
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

//-ENABLE-OPENGLES COMPATIBILITY PROFILES----------------------------------
#ifndef IMIMPL_SHADER_NONE
#ifdef IMIMPL_SHADER_GLES
#   ifndef IMIMPL_SHADER_GL3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);          // 1 => ES1.1   2 => ES2.0  3 => ES3.0
#   else //IMIMPL_SHADER_GL3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
#   endif //MIMPL_SHADER_GL3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
#endif // IMIMPL_SHADER_GLES
#endif //IMIMPL_SHADER_NONE
//--------------------------------------------------------------------------

    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,);
    //SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE,1);

    window = SDL_CreateWindow((pOptionalInitParams && pOptionalInitParams->gWindowTitle[0]!='\0') ? (const char*) &pOptionalInitParams->gWindowTitle[0] : "ImGui SDL2 OpenGL example",
                          SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED,
                          pOptionalInitParams ? pOptionalInitParams->gWindowSize.x : 1270,
                          pOptionalInitParams ? pOptionalInitParams->gWindowSize.y : 720,
                          SDL_WINDOW_RESIZABLE |
                          SDL_WINDOW_OPENGL
                          );
    if (!window)
    {
        fprintf(stderr, "Could not call SDL_CreateWindow(...) successfully.\n");
        return false;
    }

    SDL_GL_CreateContext(window);

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
    ResizeGL((int) io.DisplaySize.x,(int) io.DisplaySize.y);
	
    gImGuiInverseFPSClamp = pOptionalInitParams ? ((pOptionalInitParams->gFpsClamp!=0) ? (1.0f/pOptionalInitParams->gFpsClamp) : 1.0f) : -1.0f;
	
    static bool gImGuiBindingMouseDblClicked[5]={false,false,false,false,false};

    int done = 0;
    SDL_Event event;
    while (!done)
    {
        if (!gImGuiPaused) for (size_t i = 0; i < 5; i++) gImGuiBindingMouseDblClicked[i] = false;   // We manually set it (otherwise it won't work with low frame rates)
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    {
                    int width = event.window.data1;
                    int height = event.window.data2;
                    int fb_w, fb_h;
                    fb_w = width;fb_h = height;
                    mousePosScale.x = 1.f;//(float)fb_w / w;              // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
                    mousePosScale.y = 1.f;//(float)fb_h / h;
                    io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);    // Display size, in pixels. For clamping windows positions.

                    ResizeGL(width,height);
                    }
                    break;
                }
            }
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                SDL_Keymod mod;
                //mod = event.key.keysym.mod; // event.key.keysym.mod == 0 always!
                mod = SDL_GetModState();
                io.KeyCtrl = (mod & (KMOD_LCTRL|KMOD_RCTRL)) != 0;
                io.KeyShift = (mod & (KMOD_LSHIFT|KMOD_RSHIFT)) != 0;
                // SDLK_UNKNOWN enums...
                //io.KeysDown[event.key.keysym.sym] = event.key.type == SDL_KEYDOWN;
#               ifdef IMIMPL_SDL2_USE_STL
                std::map<int,int>::const_iterator it = specialKeyMap.find(event.key.keysym.sym);
                if ( it!=specialKeyMap.end() ) {
                    io.KeysDown[it->second] = (event.key.type == SDL_KEYDOWN);
                }
                // Must I set the other io.KeysDown[...] here too ?
#               else  //IMIMPL_SDL2_USE_STL
                // thanks to mattiasljungstrom:
                const SDL_Scancode key = event.key.keysym.scancode;
                if (key>=0 && key<512)  io.KeysDown[key] = (event.key.type == SDL_KEYDOWN);
#               endif //IMIMPL_SDL2_USE_STL

            }
                break;
            //case SDL_TEXTEDITING:   break;
            case SDL_TEXTINPUT:
            {
                size_t i=0;char c;
                while ((c = event.text.text[i])!='\0')   {
                    if (c > 0 && c < 0x10000)   ImGui::GetIO().AddInputCharacter((unsigned short)c);
                    if (++i==SDL_TEXTINPUTEVENT_TEXT_SIZE) break;
                }

            }
                break;
            case SDL_MOUSEBUTTONDOWN:        /**< Mouse button pressed */
            case SDL_MOUSEBUTTONUP: {
                SDL_Keymod mod = SDL_GetModState();
                io.KeyCtrl = (mod & (KMOD_LCTRL|KMOD_RCTRL)) != 0;
                io.KeyShift = (mod & (KMOD_LSHIFT|KMOD_RSHIFT)) != 0;
                if (event.button.button>0 && event.button.button<6) {
                    io.MouseDown[event.button.button-1] = (event.button.type == SDL_MOUSEBUTTONDOWN);
                    if (event.button.clicks==2 && event.button.type == SDL_MOUSEBUTTONDOWN) gImGuiBindingMouseDblClicked[event.button.button-1] = true;
                    //else gImGuiBindingMouseDblClicked[event.button.button-1] = false;
                }
                //fprintf(stderr,"mousePressed[%d] = %s\n",event.button.button-1,(event.button.type == SDL_MOUSEBUTTONDOWN)?"true":"false");
                }
                break;
            break;
            case SDL_MOUSEWHEEL:
                // positive away from the user and negative toward the user
                io.MouseWheel = (event.wheel.y != 0) ? event.wheel.y > 0 ? 1 : - 1 : 0;           // Mouse wheel: -1,0,+1
                //fprintf(stderr,"io.MouseWheel = %d (%d,%d)\n",io.MouseWheel,event.wheel.x,event.wheel.y); // set correctly, but why it does not seem to work ?
                break;
            case SDL_MOUSEMOTION:
                io.MousePos = ImVec2((float)event.motion.x * mousePosScale.x, (float)event.motion.y * mousePosScale.y);
                //fprintf(stderr,"io.MousePos (%1.2f,%1.2f)\n",io.MousePos.x,io.MousePos.y);
                break;
            case SDL_QUIT:
                done = 1;
                break;
            default:
                break;
            }
        }

        // Setup io.DeltaTime
        static Uint32  time = SDL_GetTicks();
        const Uint32  current_time =  SDL_GetTicks();
        io.DeltaTime = (float)(0.001*(double)(current_time - time));
        if (io.DeltaTime<=0) io.DeltaTime=1.0f/60.0f;
        time = current_time;

        // If needed we must wait (gImGuiInverseFPSClamp-io.DeltaTime) seconds (=> honestly I shouldn't add the * 2.0f factor at the end, but ImGui tells me the wrong FPS otherwise... why? <=)
        if (gImGuiInverseFPSClamp>0 && io.DeltaTime < gImGuiInverseFPSClamp)  WaitFor((unsigned int) ((gImGuiInverseFPSClamp-io.DeltaTime)*1000.f * 2.0f) );

        if (!gImGuiPaused) {
            ImGui::NewFrame();
            for (size_t i = 0; i < 5; i++) {
                io.MouseDoubleClicked[i]=gImGuiBindingMouseDblClicked[i];   // We manually set it (otherwise it won't work with low frame rates)
            }
        }


        DrawGL();


        if (!gImGuiPaused)	{
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            //glClearColor(0.8f, 0.6f, 0.6f, 1.0f);
            //glClear(GL_COLOR_BUFFER_BIT);

            ImGui::Render();
        }

        SDL_GL_SwapWindow(window);

    }

    DestroyGL();
    ImGui::Shutdown();
    DestroyImGuiFontTexture();
    DestroyImGuiProgram();
    DestroyImGuiBuffer();
    SDL_Quit();
    return 0;
}

#endif //#ifndef IMIMPL_BINDING_H

