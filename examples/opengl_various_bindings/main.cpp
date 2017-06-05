//#define IMIMPL_USE_GLEW

// Only one of these is mandatory:
//#define IMIMPL_USE_GLFW3
//#define IMIMPL_USE_GLUT
//#define IMIMPL_USE_SDL2
//#define IMIMPL_USE_WINAPI    //  IMIMPL_USE_GLEW above might be mandatory here


// These are optional definitions that nobody will ever use (all undefined by default):
//#define IMIMPL_SHADER_NONE                          // no shaders at all, and no vertex buffer object as well (minimal implementation).
//#define IMIMPL_NUM_ROUND_ROBIN_VERTEX_BUFFERS     // undefined (=default) => 1. Memory usage vs performance. When defined MUST BE A VALUE >=1.
//#define IMIMPL_SHADER_GL3                         // shader uses openGL 3.3 (glsl #version 330)
//#define IMIMPL_SHADER_GLES                        // shader uses gles (and if IMIMPL_SHADER_GL3 is defined glsl #version 300 es)

#ifdef IMIMPL_USE_GLFW3
#include "ImImpl_Binding_Glfw3.h"
#elif defined(IMIMPL_USE_GLUT)
#include "ImImpl_Binding_Glut.h"
#elif defined(IMIMPL_USE_SDL2)
#include "ImImpl_Binding_SDL2.h"
#elif defined(IMIMPL_USE_WINAPI)
#include "ImImpl_Binding_WinAPI.h"
#endif


#define DEMO_CAN_LOAD_SAVE_STYLES   // This definition needs imguistyleserializer.cpp to be compiled with the project too (no need to include anything)
#ifdef DEMO_CAN_LOAD_SAVE_STYLES
extern bool ImGuiSaveStyle(const char* filename,const ImGuiStyle& style);
extern bool ImGuiLoadStyle(const char* filename,ImGuiStyle& style);
#endif //DEMO_CAN_LOAD_SAVE_STYLES

void InitGL()	// Mandatory
{
    //fprintf(stderr,"InitGL();\n");
    //ImGui::GetIO().FontAllowUserScaling = true;
#ifdef DEMO_CAN_LOAD_SAVE_STYLES
if (!ImGuiLoadStyle("./myimgui.style",ImGui::GetStyle()))   {
    fprintf(stderr,"Warning: \"./myimgui.style\" not present.\n");
}
#endif //DEMO_CAN_LOAD_SAVE_STYLES
}
void ResizeGL(int w,int h)	// Mandatory
{
    //fprintf(stderr,"ResizeGL(%d,%d);\n",w,h);
}
void DrawGL()	// Mandatory
{

        glClearColor(0.8f, 0.6f, 0.6f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        static bool show_test_window = true;
        static bool show_another_window = false;

        // 1. Show a simple window
        {
            // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
            // Me: However I've discovered that when I clamp the FPS to a low value (e.g.10), I have to catch double clicks manually in my binding to make them work.
            // They work, but for some strange reasons only with windows properly set up through ImGui::Begin(...) and ImGui::End(...) (and whose name is NOT 'Debug').
            // [Please remember that double clicking the titlebar of a window minimizes it]
            // No problem with full frame rates.
            static bool open = true;
            ImGui::Begin("Debug ", &open, ImVec2(300,300));  // Try using 10 FPS and replacing the title with "Debug"...

            static float f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            show_test_window ^= ImGui::Button("Test Window");
            show_another_window ^= ImGui::Button("Another Window");

            // Calculate and show framerate
            static float ms_per_frame[120] = { 0 };
            static int ms_per_frame_idx = 0;
            static float ms_per_frame_accum = 0.0f;
            ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
            ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
            ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
            ms_per_frame_idx = (ms_per_frame_idx + 1) % 120;
            const float ms_per_frame_avg = ms_per_frame_accum / 120;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0f / ms_per_frame_avg);

#           ifdef DEMO_CAN_LOAD_SAVE_STYLES
            ImGui::Text("\n");
            ImGui::Text("Please modify the current style in:");
            ImGui::Text("ImGui Test->Window Options->Style Editor");
            static bool loadCurrentStyle = false;
            static bool saveCurrentStyle = false;
            static bool resetCurrentStyle = false;
            loadCurrentStyle = ImGui::Button("Load Saved Style");
            saveCurrentStyle = ImGui::Button("Save Current Style");
            resetCurrentStyle = ImGui::Button("Reset Current Style");
            if (loadCurrentStyle)   {
                if (!ImGuiLoadStyle("./myimgui.style",ImGui::GetStyle()))   {
                    fprintf(stderr,"Warning: \"./myimgui.style\" not present.\n");
                }
            }
            if (saveCurrentStyle)   {
                if (!ImGuiSaveStyle("./myimgui.style",ImGui::GetStyle()))   {
                    fprintf(stderr,"Warning: \"./myimgui.style\" cannot be saved.\n");
                }
            }
            if (resetCurrentStyle)  ImGui::GetStyle() = ImGuiStyle();
#           endif //DEMO_CAN_LOAD_SAVE_STYLES

            ImGui::End();
        }

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window, ImVec2(200,100));
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNewWindowDefaultPos(ImVec2(650, 20));        // Normally user code doesn't need/want to call this, because positions are saved in .ini file. Here we just want to make the demo initial state a bit more friendly!
            ImGui::ShowTestWindow(&show_test_window);
        }

}
void DestroyGL()    // Mandatory
{
}



// Application code
#ifndef IMIMPL_USE_WINAPI
int main(int argc, char** argv)
{
    // Basic
    ImImpl_Main(NULL,argc,argv);

    // Advanced
    /*
    // These lines load an embedded font. [However these files are way too big... inside <imgui.cpp> they used a better format storing bytes at groups of 4, so the files are more concise (1/4?) than mine]
    const unsigned char fntMemory[] =
#   include "./fonts/DejaVuSansCondensedBoldOutlineRGBAbinary18.fnt.inl"
    const unsigned char imgMemory[] =
#   include "./fonts/DejaVuSansCondensedBoldOutlineRGBAbinary18_0.png.inl"

    ImImpl_InitParams gImGuiInitParams(
    -1,-1,NULL,                                                         // optional window width, height, title
    NULL,//"./fonts/DejaVuSansCondensedBoldOutlineRGBAbinary18.fnt",    // optional custom font from file (main custom font)
    NULL,//"./fonts/DejaVuSansCondensedBoldOutlineRGBAbinary18_0.png",  // optional custom font from file (main custom font)
    -1,-1,                                                              // optional white spot in font texture (returned by the console if not set)
    &fntMemory[0],sizeof(fntMemory)/sizeof(fntMemory[0]),               // optional custom font from memory (secondary custom font) WARNING (licensing problem): e.g. embedding a GPL font in your code can make your code GPL as well.
    &imgMemory[0],sizeof(imgMemory)/sizeof(imgMemory[0])                // optional custom font from memory (secondary custom font) WARNING (licensing problem): e.g. embedding a GPL font in your code can make your code GPL as well.
    );
    gImGuiInitParams.gFpsClamp = 10.0f;                                 // Optional Max allowed FPS (default -1 => unclamped). Useful for editors and to save GPU and CPU power.
    ImImpl_Main(&gImGuiInitParams,argc,argv);*/


	return 0;
}
#else //IMIMPL_USE_WINAPI
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int iCmdShow)   // This branch has made my code less concise (I will consider stripping it)
{
    // Basic
    ImImpl_WinMain(NULL,hInstance,hPrevInstance,lpCmdLine,iCmdShow);

    // Advanced
    /*
    // These lines load an embedded font. [However these files are way too big... inside <imgui.cpp> they used a better format storing bytes at groups of 4, so the files are more concise (1/4?) than mine]
    const unsigned char fntMemory[] =
#   include "./fonts/DejaVuSansCondensedBoldOutlineRGBAbinary18.fnt.inl"
    const unsigned char imgMemory[] =
#   include "./fonts/DejaVuSansCondensedBoldOutlineRGBAbinary18_0.png.inl"

    ImImpl_InitParams gImGuiInitParams(
    -1,-1,NULL,                                                         // optional window width, height, title
    NULL,//"./fonts/DejaVuSansCondensedBoldOutlineRGBAbinary18.fnt",    // optional custom font from file (main custom font)
    NULL,//"./fonts/DejaVuSansCondensedBoldOutlineRGBAbinary18_0.png",  // optional custom font from file (main custom font)
    -1,-1,                                                              // optional white spot in font texture (returned by the console if not set)
    &fntMemory[0],sizeof(fntMemory)/sizeof(fntMemory[0]),               // optional custom font from memory (secondary custom font) WARNING (licensing problem): e.g. embedding a GPL font in your code can make your code GPL as well.
    &imgMemory[0],sizeof(imgMemory)/sizeof(imgMemory[0])                // optional custom font from memory (secondary custom font) WARNING (licensing problem): e.g. embedding a GPL font in your code can make your code GPL as well.
    );
    gImGuiInitParams.gFpsClamp = 10.0f;                                 // Optional Max allowed FPS (default -1 => unclamped). Useful for editors and to save GPU and CPU power.
    ImImpl_WinMain(&gImGuiInitParams,hInstance,hPrevInstance,lpCmdLine,iCmdShow);*/

    return 0;
}
#endif //IMIMPL_USE_WINAPI



