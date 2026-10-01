// Dear ImGui: standalone example application for Xlib + OpenGL

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_xlib.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

// Main code
int main(int, char**)
{
    // Setup Xlib
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        printf("Error: Could not open display\n");
        return -1;
    }

    int screen_id = DefaultScreen(display);

    static int visual_attribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
    	GLX_DEPTH_SIZE, 24,
    	GLX_STENCIL_SIZE, 8,
        None
    };

    int num_fbc = 0;
    GLXFBConfig *fbc = glXChooseFBConfig(display, screen_id, visual_attribs, &num_fbc);
    if (!fbc) {
        printf("Error: glXChooseFBConfig() failed\n");
        return -1;
    }

    XVisualInfo *visual = glXGetVisualFromFBConfig(display, fbc[0]);
    if (visual == 0) {
        printf("Error: Could not create correct visual window\n");
        XCloseDisplay(display);
        return -1;
    }

    // Create window
    XSetWindowAttributes window_attribs;
    window_attribs.border_pixel = 0;
    window_attribs.background_pixel = None;
    window_attribs.colormap = XCreateColormap(display, RootWindow(display, screen_id), visual->visual, AllocNone);
    Window window = XCreateWindow(display, RootWindow(display, screen_id), 0, 0, 1280, 720, 0, visual->depth, InputOutput, visual->visual, CWBackPixel | CWColormap | CWBorderPixel, &window_attribs);

    // Handle window closing
    Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);

    XFree(visual);

    // Create GLX context
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = NULL;
    glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC) glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    int context_attribs[] = {
        GLX_CONTEXT_FLAGS_ARB        , 0,
        GLX_CONTEXT_PROFILE_MASK_ARB , GLX_CONTEXT_ES2_PROFILE_BIT_EXT,
        GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        None
    };
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    int context_attribs[] = {
        GLX_CONTEXT_FLAGS_ARB        , 0,
        GLX_CONTEXT_PROFILE_MASK_ARB , GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        None
    };
#endif

    GLXContext glx_context = glXCreateContextAttribsARB(display, fbc[0], 0, True, context_attribs);

    if (glx_context == 0) {
        printf("Error: Could not create GLX context\n");
        XCloseDisplay(display);
        return -1;
    }

    XMapWindow(display, window);
    glXMakeCurrent(display, window, glx_context);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplXlib_Init(display, window);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        while (XPending(display))
        {
            XEvent event;
            XNextEvent(display, &event);
            ImGui_ImplXlib_ProcessEvent(&event);
            
            if (event.type == ClientMessage && event.xclient.window == window && (Atom)event.xclient.data.l[0] == wmDeleteMessage)
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplXlib_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glXSwapBuffers(display, window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplXlib_Shutdown();
    ImGui::DestroyContext();

    glXDestroyContext(display, glx_context);
    XDestroyWindow(display, window);
    XFreeColormap(display, window_attribs.colormap);
    XCloseDisplay(display);

    return 0;
}
