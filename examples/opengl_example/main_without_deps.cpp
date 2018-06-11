// Basic windows/OGL boilerplate code taken from nehe.gamedev.net with IMGUI integration

/*
 *      This Code Was Created By Jeff Molofee 2000
 *      A HUGE Thanks To Fredric Echols For Cleaning Up
 *      And Optimizing This Code, Making It More Flexible!
 *      If You've Found This Code Useful, Please Let Me Know.
 *      Visit My Site At nehe.gamedev.net
 */

#include <windows.h>        // Header File For Windows
#include <gl\gl.h>          // Header File For The OpenGL32 Library
#include "..\..\imgui.h"
#pragma warning(disable:4244 4100 4505 4706 4701 4706)
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
double highDpiScale = 1.0f;
HDC         hDC=NULL;       // Private GDI Device Context
HGLRC       hRC=NULL;       // Permanent Rendering Context
HWND        hWnd=NULL;      // Holds Our Window Handle
HINSTANCE   hInstance;      // Holds The Instance Of The Application

bool    keys[256];          // Array Used For The Keyboard Routine
bool    active=TRUE;        // Window Active Flag Set To TRUE By Default
bool    fullscreen=TRUE;    // Fullscreen Flag Set To Fullscreen Mode By Default

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);   // Declaration For WndProc
static bool mousePressed[2] = { false, false };

void ImImpl_RenderDrawLists(ImDrawData* draw_data)
{
	// We are using the OpenGL fixed pipeline to make the example code simpler to read!
	// A probable faster way to render would be to collate all vertices from all cmd_lists into a single vertex buffer.
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnable(GL_TEXTURE_2D);
	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context

	// Setup orthographic projection matrix
	const float width = ImGui::GetIO().DisplaySize.x / highDpiScale;
	const float height = ImGui::GetIO().DisplaySize.y / highDpiScale;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Render command lists
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const unsigned char* vtx_buffer = (const unsigned char*)&cmd_list->VtxBuffer.front();
		const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();
		glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, pos)));
		glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, col)));

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				glScissor((GLint)(pcmd->ClipRect.x*highDpiScale),
					(GLint)((height - pcmd->ClipRect.w)*highDpiScale),
					(GLint)((pcmd->ClipRect.z - pcmd->ClipRect.x)*highDpiScale),
					(GLint)((pcmd->ClipRect.w - pcmd->ClipRect.y)*highDpiScale));

				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer);
			}
			idx_buffer += pcmd->ElemCount;
		}
	}
#undef OFFSETOF

	// Restore modified state
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindTexture(GL_TEXTURE_2D, 0);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
}

void LoadFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    //ImFont* my_font1 = io.Fonts->AddFontDefault();
    //ImFont* my_font2 = io.Fonts->AddFontFromFileTTF("extra_fonts/Karla-Regular.ttf", 15.0f);
    //ImFont* my_font3 = io.Fonts->AddFontFromFileTTF("extra_fonts/ProggyClean.ttf", 13.0f); my_font3->DisplayOffset.y += 1;
    //ImFont* my_font4 = io.Fonts->AddFontFromFileTTF("extra_fonts/ProggyTiny.ttf", 10.0f); my_font4->DisplayOffset.y += 1;
    //ImFont* my_font5 = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 20.0f, io.Fonts->GetGlyphRangesJapanese());

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)tex_id;
}


void InitImGui()
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    int display_w = (int)(rect.right - rect.left);
    int display_h = (int)(rect.bottom - rect.top);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)display_w, (float)display_h);   // Display size, in pixels. For clamping windows positions.
    io.DeltaTime = 1.0f / 60.0f;                                     // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our time step is variable)
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;                              // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_UP;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';

    io.RenderDrawListsFn = ImImpl_RenderDrawLists;

    LoadFontsTexture();
}
INT64 ticks_per_second = 0;
INT64 last_time = 0;

void UpdateImGui()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup time step
    INT64 current_time;
    QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
    io.DeltaTime = (float)(current_time - last_time) / ticks_per_second;
    last_time = current_time;

    // Setup inputs
    // (we already got mouse position, buttons, wheel from the window message callback)
    BYTE keystate[256];
    GetKeyboardState(keystate);
    for (int i = 0; i < 256; i++)
        io.KeysDown[i] = (keystate[i] & 0x80) != 0;
    io.KeyCtrl = (keystate[VK_CONTROL] & 0x80) != 0;
    io.KeyShift = (keystate[VK_SHIFT] & 0x80) != 0;
    // io.MousePos : filled by WM_MOUSEMOVE event
    // io.MouseDown : filled by WM_*BUTTON* events
    // io.MouseWheel : filled by WM_MOUSEWHEEL events

    // Start the frame
    ImGui::NewFrame();
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)     // Resize And Initialize The GL Window
{
    if (height==0)                                      // Prevent A Divide By Zero By
    {
        height=1;                                       // Making Height Equal One
    }
    ImGuiIO& io = ImGui::GetIO();
    glViewport(0,0,width,height);                       // Reset The Current Viewport

    glMatrixMode(GL_PROJECTION);                        // Select The Projection Matrix
    glLoadIdentity();                                   // Reset The Projection Matrix

    glMatrixMode(GL_MODELVIEW);                         // Select The Modelview Matrix
    glLoadIdentity();                                   // Reset The Modelview Matrix
    io.DisplaySize = ImVec2((float)width, (float)height);                                   // Display size, in pixels. For clamping windows positions.
}

int InitGL(GLvoid)                                      // All Setup For OpenGL Goes Here
{
    InitImGui();
    glShadeModel(GL_SMOOTH);                            // Enable Smooth Shading
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);               // Black Background
    glClearDepth(1.0f);                                 // Depth Buffer Setup
    glEnable(GL_DEPTH_TEST);                            // Enables Depth Testing
    glDepthFunc(GL_LEQUAL);                             // The Type Of Depth Testing To Do
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Really Nice Perspective Calculations
    return TRUE;                                        // Initialization Went OK
}
bool show_test_window = true;
bool show_another_window = false;
ImVec4 clear_col = ImColor(114, 144, 154);
int DrawGLScene(GLvoid)                                 // Here's Where We Do All The Drawing
{
    glClearColor(clear_col.x, clear_col.y, clear_col.z, clear_col.w);
    glClear(GL_COLOR_BUFFER_BIT);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   // Clear Screen And Depth Buffer
    glLoadIdentity();                                   // Reset The Current Modelview Matrix
    UpdateImGui();

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    {
        static float f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&clear_col);
        if (ImGui::Button("Test Window")) show_test_window ^= 1;
        if (ImGui::Button("Another Window")) show_another_window ^= 1;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window, ImVec2(200, 100));
        ImGui::Text("Hello");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_test_window)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
        ImGui::ShowTestWindow(&show_test_window);
    }
    ImGui::Render();
    return TRUE;                                        // Everything Went OK
}

GLvoid KillGLWindow(const char* className)                              // Properly Kill The Window
{
    ImGui::Shutdown();
    if (fullscreen)                                     // Are We In Fullscreen Mode?
    {
        ChangeDisplaySettings(NULL,0);                  // If So Switch Back To The Desktop
        ShowCursor(TRUE);                               // Show Mouse Pointer
    }

    if (hRC)                                            // Do We Have A Rendering Context?
    {
        if (!wglMakeCurrent(NULL,NULL))                 // Are We Able To Release The DC And RC Contexts?
        {
            MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
        }

        if (!wglDeleteContext(hRC))                     // Are We Able To Delete The RC?
        {
            MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
        }
        hRC=NULL;                                       // Set RC To NULL
    }

    if (hDC && !ReleaseDC(hWnd,hDC))                    // Are We Able To Release The DC
    {
        MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
        hDC=NULL;                                       // Set DC To NULL
    }

    if (hWnd && !DestroyWindow(hWnd))                   // Are We Able To Destroy The Window?
    {
        MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
        hWnd=NULL;                                      // Set hWnd To NULL
    }

    if (!UnregisterClass(className, hInstance))         // Are We Able To Unregister Class
    {
        MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
        hInstance=NULL;                                 // Set hInstance To NULL
    }
}

/*  This Code Creates Our OpenGL Window.  Parameters Are:                   *
 *  title           - Title To Appear At The Top Of The Window              *
 *  width           - Width Of The GL Window Or Fullscreen Mode             *
 *  height          - Height Of The GL Window Or Fullscreen Mode            *
 *  bits            - Number Of Bits To Use For Color (8/16/24/32)          *
 *  fullscreenflag  - Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)   */
 
BOOL CreateGLWindow(const char* title, const char* windowClass, int width, int height, int bits, bool fullscreenflag)
{
    GLuint      PixelFormat;            // Holds The Results After Searching For A Match
    WNDCLASS    wc;                     // Windows Class Structure
    DWORD       dwExStyle;              // Window Extended Style
    DWORD       dwStyle;                // Window Style
    RECT        WindowRect;             // Grabs Rectangle Upper Left / Lower Right Values
    WindowRect.left=(long)0;            // Set Left Value To 0
    WindowRect.right=(long)width;       // Set Right Value To Requested Width
    WindowRect.top=(long)0;             // Set Top Value To 0
    WindowRect.bottom=(long)height;     // Set Bottom Value To Requested Height

    fullscreen=fullscreenflag;          // Set The Global Fullscreen Flag

    hInstance           = GetModuleHandle(NULL);                // Grab An Instance For Our Window
    wc.style            = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;   // Redraw On Size, And Own DC For Window.
    wc.lpfnWndProc      = (WNDPROC) WndProc;                    // WndProc Handles Messages
    wc.cbClsExtra       = 0;                                    // No Extra Window Data
    wc.cbWndExtra       = 0;                                    // No Extra Window Data
    wc.hInstance        = hInstance;                            // Set The Instance
    wc.hIcon            = LoadIcon(NULL, IDI_WINLOGO);          // Load The Default Icon
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);          // Load The Arrow Pointer
    wc.hbrBackground    = NULL;                                 // No Background Required For GL
    wc.lpszMenuName     = NULL;                                 // We Don't Want A Menu
    wc.lpszClassName = windowClass;                             // Set The Class Name

    if (!RegisterClass(&wc))                                    // Attempt To Register The Window Class
    {
        MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;                                           // Return FALSE
    }
    
    if (fullscreen)                                             // Attempt Fullscreen Mode?
    {
        DEVMODE dmScreenSettings;                               // Device Mode
        memset(&dmScreenSettings,0,sizeof(dmScreenSettings));   // Makes Sure Memory's Cleared
        dmScreenSettings.dmSize=sizeof(dmScreenSettings);       // Size Of The Devmode Structure
        dmScreenSettings.dmPelsWidth    = width;                // Selected Screen Width
        dmScreenSettings.dmPelsHeight   = height;               // Selected Screen Height
        dmScreenSettings.dmBitsPerPel   = bits;                 // Selected Bits Per Pixel
        dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

        // Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
        if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
        {
            // If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
            if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
            {
                fullscreen=FALSE;       // Windowed Mode Selected.  Fullscreen = FALSE
            }
            else
            {
                // Pop Up A Message Box Letting User Know The Program Is Closing.
                MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
                return FALSE;                                   // Return FALSE
            }
        }
    }

    if (fullscreen)                                             // Are We Still In Fullscreen Mode?
    {
        dwExStyle=WS_EX_APPWINDOW;                              // Window Extended Style
        dwStyle=WS_POPUP;                                       // Windows Style
        ShowCursor(FALSE);                                      // Hide Mouse Pointer
    }
    else
    {
        dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;           // Window Extended Style
        dwStyle=WS_OVERLAPPEDWINDOW;                            // Windows Style
    }

    AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);     // Adjust Window To True Requested Size

    // Create The Window
    if (!(hWnd=CreateWindowEx(  dwExStyle,                          // Extended Style For The Window
                                windowClass,                            // Class Name
                                title,                              // Window Title
                                dwStyle |                           // Defined Window Style
                                WS_CLIPSIBLINGS |                   // Required Window Style
                                WS_CLIPCHILDREN,                    // Required Window Style
                                0, 0,                               // Window Position
                                WindowRect.right-WindowRect.left,   // Calculate Window Width
                                WindowRect.bottom-WindowRect.top,   // Calculate Window Height
                                NULL,                               // No Parent Window
                                NULL,                               // No Menu
                                hInstance,                          // Instance
                                NULL)))                             // Dont Pass Anything To WM_CREATE
    {
        KillGLWindow(windowClass);                              // Reset The Display
        MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;                               // Return FALSE
    }

    static  PIXELFORMATDESCRIPTOR pfd=              // pfd Tells Windows How We Want Things To Be
    {
        sizeof(PIXELFORMATDESCRIPTOR),              // Size Of This Pixel Format Descriptor
        1,                                          // Version Number
        PFD_DRAW_TO_WINDOW |                        // Format Must Support Window
        PFD_SUPPORT_OPENGL |                        // Format Must Support OpenGL
        PFD_DOUBLEBUFFER,                           // Must Support Double Buffering
        PFD_TYPE_RGBA,                              // Request An RGBA Format
        bits,                                       // Select Our Color Depth
        0, 0, 0, 0, 0, 0,                           // Color Bits Ignored
        0,                                          // No Alpha Buffer
        0,                                          // Shift Bit Ignored
        0,                                          // No Accumulation Buffer
        0, 0, 0, 0,                                 // Accumulation Bits Ignored
        16,                                         // 16Bit Z-Buffer (Depth Buffer)  
        0,                                          // No Stencil Buffer
        0,                                          // No Auxiliary Buffer
        PFD_MAIN_PLANE,                             // Main Drawing Layer
        0,                                          // Reserved
        0, 0, 0                                     // Layer Masks Ignored
    };
    
    if (!(hDC=GetDC(hWnd)))                         // Did We Get A Device Context?
    {
        KillGLWindow(windowClass);                              // Reset The Display
        MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;                               // Return FALSE
    }

    if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd))) // Did Windows Find A Matching Pixel Format?
    {
        KillGLWindow(windowClass);                              // Reset The Display
        MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;                               // Return FALSE
    }

    if(!SetPixelFormat(hDC,PixelFormat,&pfd))       // Are We Able To Set The Pixel Format?
    {
        KillGLWindow(windowClass);                              // Reset The Display
        MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;                               // Return FALSE
    }

    if (!(hRC=wglCreateContext(hDC)))               // Are We Able To Get A Rendering Context?
    {
        KillGLWindow(windowClass);                              // Reset The Display
        MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;                               // Return FALSE
    }

    if(!wglMakeCurrent(hDC,hRC))                    // Try To Activate The Rendering Context
    {
        KillGLWindow(windowClass);                              // Reset The Display
        MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;                               // Return FALSE
    }

    ShowWindow(hWnd,SW_SHOW);                       // Show The Window
    SetForegroundWindow(hWnd);                      // Slightly Higher Priority
    SetFocus(hWnd);                                 // Sets Keyboard Focus To The Window
    ReSizeGLScene(width, height);                   // Set Up Our Perspective GL Screen

    if (!InitGL())                                  // Initialize Our Newly Created GL Window
    {
        KillGLWindow(windowClass);                              // Reset The Display
        MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
        return FALSE;                               // Return FALSE
    }

    return TRUE;                                    // Success
}

LRESULT CALLBACK WndProc(   HWND    hWnd,           // Handle For This Window
                            UINT    uMsg,           // Message For This Window
                            WPARAM  wParam,         // Additional Message Information
                            LPARAM  lParam)         // Additional Message Information
{
    ImGuiIO& io = ImGui::GetIO();
    switch (uMsg)                                   // Check For Windows Messages
    {
        case WM_ACTIVATE:                           // Watch For Window Activate Message
        {
            if (!HIWORD(wParam))                    // Check Minimization State
            {
                active=TRUE;                        // Program Is Active
            }
            else
            {
                active=FALSE;                       // Program Is No Longer Active
            }

            return 0;                               // Return To The Message Loop
        }

        case WM_SYSCOMMAND:                         // Intercept System Commands
        {
            switch (wParam)                         // Check System Calls
            {
                case SC_SCREENSAVE:                 // Screensaver Trying To Start?
                case SC_MONITORPOWER:               // Monitor Trying To Enter Powersave?
                return 0;                           // Prevent From Happening
            }
            break;                                  // Exit
        }

        case WM_CLOSE:                              // Did We Receive A Close Message?
        {
            PostQuitMessage(0);                     // Send A Quit Message
            return 0;                               // Jump Back
        }

        case WM_KEYDOWN:                            // Is A Key Being Held Down?
        {
            keys[wParam] = TRUE;                    // If So, Mark It As TRUE
            return 0;                               // Jump Back
        }

        case WM_KEYUP:                              // Has A Key Been Released?
        {
            keys[wParam] = FALSE;                   // If So, Mark It As FALSE
            return 0;                               // Jump Back
        }

        case WM_SIZE:                               // Resize The OpenGL Window
        {
            ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));  // LoWord=Width, HiWord=Height
            return 0;                               // Jump Back
        }
        case WM_ENTERSIZEMOVE:
        {
            SetTimer(hWnd, 1, ImGui::GetIO().DeltaTime*1000, (TIMERPROC)NULL);
            return 0;
        }

        case WM_EXITSIZEMOVE:
        {
            KillTimer(hWnd, 1);
            return 0;
        }
        case WM_TIMER :
        {
            switch (wParam)
            {
            case 1:
            {
                DrawGLScene();
                SwapBuffers(hDC);
            }
            }
        }
        case WM_LBUTTONDOWN:
            io.MouseDown[0] = true;
            return true;
        case WM_LBUTTONUP:
            io.MouseDown[0] = false;
            return true;
        case WM_RBUTTONDOWN:
            io.MouseDown[1] = true;
            return true;
        case WM_RBUTTONUP:
            io.MouseDown[1] = false;
            return true;
        case WM_MOUSEWHEEL:
            io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) / 100.0f;// > 0 ? +0.1f : -0.1f;
            return true;
        case WM_MOUSEMOVE:
            // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
            io.MousePos.x = (signed short)(lParam);
            io.MousePos.y = (signed short)(lParam >> 16);
            io.MousePos.x /= highDpiScale;
            io.MousePos.y /= highDpiScale;
            return true;
        case WM_CHAR:
            // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
            if (wParam > 0 && wParam < 0x10000)
                io.AddInputCharacter((unsigned short)wParam);
            return true;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

    }

    // Pass All Unhandled Messages To DefWindowProc
    return DefWindowProc(hWnd,uMsg,wParam,lParam);
}
// Get the horizontal and vertical screen sizes in pixel
void GetDesktopResolution(int& horizontal, int& vertical)
{
    RECT desktop;
    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates
    // (horizontal, vertical)
    horizontal = desktop.right;
    vertical = desktop.bottom;
}

void* getUser32Function(const char* functionName)
{
    HMODULE module = GetModuleHandleA("user32.dll");
    assert(module != 0);
    return (void*)GetProcAddress(module, functionName);
}

#ifndef MONITOR_DPI_TYPE
enum Monitor_DPI_Type
{
    MDT_Effective_DPI = 0,
    MDT_Angular_DPI = 1,
    MDT_Raw_DPI = 2,
    MDT_Default = MDT_Effective_DPI
};

enum Process_DPI_Awareness
{
    Process_DPI_Unaware = 0,
    Process_System_DPI_Aware = 1,
    Process_Per_Monitor_DPI_Aware = 2
};
#endif

typedef BOOL(WINAPI* SetProcessDPIAwareFunc)();
typedef BOOL(WINAPI* SetProcessDPIAwarenessFunc) (Process_DPI_Awareness);
typedef HRESULT(WINAPI* GetDPIForMonitorFunc) (HMONITOR, Monitor_DPI_Type, UINT*, UINT*);

static SetProcessDPIAwareFunc     setProcessDPIAware = nullptr;
static SetProcessDPIAwarenessFunc setProcessDPIAwareness = nullptr;
static GetDPIForMonitorFunc       getDPIForMonitor = nullptr;

//==============================================================================
static void setDPIAwareness()
{
    if (setProcessDPIAwareness == nullptr)
    {
        HMODULE shcoreModule = GetModuleHandleA("SHCore.dll");

        if (shcoreModule != 0)
        {
            setProcessDPIAwareness = (SetProcessDPIAwarenessFunc)GetProcAddress(shcoreModule, "SetProcessDpiAwareness");
            getDPIForMonitor = (GetDPIForMonitorFunc)GetProcAddress(shcoreModule, "GetDpiForMonitor");

            if (setProcessDPIAwareness != nullptr && getDPIForMonitor != nullptr
                //                     && SUCCEEDED (setProcessDPIAwareness (Process_Per_Monitor_DPI_Aware)))
                && SUCCEEDED(setProcessDPIAwareness(Process_System_DPI_Aware))) // (keep using this mode temporarily..)
                return;
        }

        if (setProcessDPIAware == nullptr)
        {
            setProcessDPIAware = (SetProcessDPIAwareFunc)getUser32Function("SetProcessDPIAware");

            if (setProcessDPIAware != nullptr)
                setProcessDPIAware();
        }
    }
}

static double getGlobalDPI()
{
    setDPIAwareness();

    HDC dc = GetDC(0);
    const double dpi = (GetDeviceCaps(dc, LOGPIXELSX)
        + GetDeviceCaps(dc, LOGPIXELSY)) / 2.0;
    ReleaseDC(0, dc);
    return dpi;
}

double getDefaultMasterScale()
{
    return getGlobalDPI() / 96.0;
}

int WINAPI WinMain( HINSTANCE   hInstance,          // Instance
                    HINSTANCE   hPrevInstance,      // Previous Instance
                    LPSTR       lpCmdLine,          // Command Line Parameters
                    int         nCmdShow)           // Window Show State
{
    setDPIAwareness();
    highDpiScale = getDefaultMasterScale();
    const char* window_name = "rrGui";
    const char* class_name = "rrGui";
    MSG     msg;                                    // Windows Message Structure
    BOOL    done=FALSE;                             // Bool Variable To Exit Loop
    if (!QueryPerformanceFrequency((LARGE_INTEGER *)&ticks_per_second))
        return 1;
    if (!QueryPerformanceCounter((LARGE_INTEGER *)&last_time))
        return 1;
    int width = 1024 * highDpiScale;
    int height = 768 * highDpiScale;
	fullscreen=FALSE;                           // Windowed Mode

    // Create Our OpenGL Window
    if (!CreateGLWindow(window_name, class_name, width, height, 32, fullscreen))
    {
        return 0;                                   // Quit If Window Was Not Created
    }

    while(!done)                                    // Loop That Runs While done=FALSE
    {
        if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))   // Is There A Message Waiting?
        {
            if (msg.message==WM_QUIT)               // Have We Received A Quit Message?
            {
                done=TRUE;                          // If So done=TRUE
            }
            else                                    // If Not, Deal With Window Messages
            {
                TranslateMessage(&msg);             // Translate The Message
                DispatchMessage(&msg);              // Dispatch The Message
            }
        }
        else                                        // If There Are No Messages
        {
            // Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
            if (active)                             // Program Active?
            {
                if (keys[VK_ESCAPE])                // Was ESC Pressed?
                {
                    int res = MessageBox(NULL, "Do you really want to quit the application?", 
                        "Quit?", MB_YESNO | MB_ICONEXCLAMATION);
                    if (res == IDYES)
                    {
                        done = TRUE;				// ESC Signalled A Quit
                    }
                }
                else                                // Not Time To Quit, Update Screen
                {
                    DrawGLScene();                  // Draw The Scene
                    SwapBuffers(hDC);               // Swap Buffers (Double Buffering)
                }
            }
        }
    }

    // Shutdown
    KillGLWindow(class_name);						// Kill The Window
    return (msg.wParam);                            // Exit The Program
}
