// ImGui - standalone example application for GLFW + OpenGL2, using legacy fixed pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the opengl3_example/ folder**
// See imgui_impl_glfw.cpp for details.

#include "imgui.h"
#include "imgui_impl_orx.h"
#include <stdio.h>

extern "C" 
    {
#include <orx.h>
    }

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#elif defined(__cplusplus)
# define UNUSED(x)
#else
# define UNUSED(x) x
#endif




orxVIEWPORT * gpstMainViewport = orxNULL;
orxCAMERA * gpstMainCamera = orxNULL;

//////////////////////////////////////////////////////////////////////////
void ImGuiOrx_ResizeViewport()
    {
    orxFLOAT scr_w, scr_h;
    orxDisplay_GetScreenSize(&scr_w, &scr_h);

    orxFLOAT vwp_w, vwp_h;
    orxViewport_GetSize(gpstMainViewport, &vwp_w, &vwp_h);

    orxAABOX frustum;
    orxCamera_GetFrustum(gpstMainCamera, &frustum);

    orxVECTOR cam_pos;
    orxCamera_GetPosition(gpstMainCamera, &cam_pos);
    orxCamera_SetFrustum(gpstMainCamera, vwp_w, vwp_h, frustum.vTL.fZ, frustum.vBR.fZ);
    orxCamera_SetPosition(gpstMainCamera, &cam_pos);

    orxDEBUG_PRINT(orxDEBUG_LEVEL_LOG, "Viewport Size : %f, %f", vwp_w, vwp_h);
    }


//////////////////////////////////////////////////////////////////////////
orxSTATUS orxFASTCALL ImGuiOrx_EventHandler(const orxEVENT *_pstEvent) 
    {
    switch (_pstEvent->eType)
        {
        case orxEVENT_TYPE_RENDER:
            {
            /* we render ImGui stuff on the top so we call it at the end of Orx frame rendering cycle */
            if (_pstEvent->eID == orxRENDER_EVENT_STOP)
                ImGui_Orx_Render(NULL, ImGui::GetDrawData());
            }
            break;

        case orxEVENT_TYPE_DISPLAY:
            ImGuiOrx_ResizeViewport();
            break;

        default:
            break;
        }
 
    return orxSTATUS_SUCCESS;
    }

//////////////////////////////////////////////////////////////////////////
orxSTATUS orxFASTCALL ImGuiOrx_Init()
    {
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();

    /* create the gui context */
    ImGui::CreateContext();

    /* initialize the gui port */
    ImGui_Orx_Init();

    /* add events to manage */
    orxEvent_AddHandler(orxEVENT_TYPE_RENDER, ImGuiOrx_EventHandler);
    orxEvent_AddHandler(orxEVENT_TYPE_VIEWPORT, ImGuiOrx_EventHandler);
    orxEvent_AddHandler(orxEVENT_TYPE_DISPLAY, ImGuiOrx_EventHandler);

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them. 
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple. 
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Creates main viewport
    gpstMainViewport = orxViewport_CreateFromConfig("MainViewport");

    // Gets main camera
    gpstMainCamera = orxViewport_GetCamera(gpstMainViewport);
            
    return orxSTATUS_SUCCESS;
    }

//////////////////////////////////////////////////////////////////////////
orxSTATUS orxFASTCALL ImGuiOrx_Run()
    {
    ImGui_Orx_NewFrame();

    ImVec4 clear_color = ImColor(114, 144, 154);

    bool show_demo_window = true;
    bool show_another_window = false;

    orxFLOAT viewport_w, viewport_h;
    orxViewport_GetSize(gpstMainViewport, &viewport_w, &viewport_h);

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"

    ImGui::Text("Hello, world!");
    ImGui::SetNextWindowSize(ImVec2(200, 500), ImGuiSetCond_FirstUseEver);
    static float f = 0.0f;
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    ImGui::ColorEdit3("clear color", (float*)&clear_color);
    if (ImGui::Button("Test Window")) show_demo_window ^= 1;
    if (ImGui::Button("Another Window")) show_another_window ^= 1;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window)
        {
        ImGui::SetNextWindowPos(ImVec2());
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello");
        ImGui::End();
        }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_demo_window)
        {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
        ImGui::ShowTestWindow();
        }

    ImGui::Render();
    return orxSTATUS_SUCCESS;
    }

//////////////////////////////////////////////////////////////////////////
void  orxFASTCALL ImGuiOrx_Exit()
    {
    ImGui_Orx_Shutdown();
    }


/*
#ifndef __orxMSVC__
*/
//////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
    {
    /* Inits and executes orx */
    orx_Execute(argc, argv, ImGuiOrx_Init, ImGuiOrx_Run, ImGuiOrx_Exit);
    return EXIT_SUCCESS;
    }
/*
#else  // __orxMSVC__
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    {
    UNUSED(hInstance);
    UNUSED(hPrevInstance);
    UNUSED(lpCmdLine);
    UNUSED(nCmdShow);

    / * Inits and executes orx * /
    orx_WinExecute(ImGuiOrx_Init, ImGuiOrx_Run, ImGuiOrx_Exit);

    // Done!
    return EXIT_SUCCESS;
    }
#endif // __orxMSVC__
*/


