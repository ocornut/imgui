// ImGui - standalone example application for Glfw + OpenGL 2, using fixed pipeline

#include <imgui.h>
#include "imgui_impl_glfw.h"
#include <stdio.h>
#include <GLFW/glfw3.h>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

float IntAsFloat(void *ints, int index)
{
    return (float)(((int *)ints)[index]);
}

const char *WeaponDropTypeStr(void *strs, int index)
{
    return ((const char **)strs)[index];
}

#define ARRAY_ELEMENTS(array)    (sizeof(array) / sizeof(array[0]))

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(1);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui OpenGL2 example", NULL, NULL);
    glfwMakeContextCurrent(window);

    // Setup ImGui binding
    ImGui_ImplGlfw_Init(window, true);
    //ImGuiIO& io = ImGui::GetIO();
    //ImFont* my_font0 = io.Fonts->AddFontDefault();
    //ImFont* my_font1 = io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //ImFont* my_font2 = io.Fonts->AddFontFromFileTTF("../../extra_fonts/Karla-Regular.ttf", 16.0f);
    //ImFont* my_font3 = io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f); my_font3->DisplayOffset.y += 1;
    //ImFont* my_font4 = io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f); my_font4->DisplayOffset.y += 1;
    //ImFont* my_font5 = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, io.Fonts->GetGlyphRangesJapanese());

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);
    srand(0);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfw_NewFrame();

        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        {
            static float f = 0.0f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

        ImGui::Begin("Histogram labels");
        static const char *dropTypeStr[] =
        {
            "WEAPONDROPTYPE_MG",
            "WEAPONDROPTYPE_CANNON",
            "WEAPONDROPTYPE_ARTILLERY",
            "WEAPONDROPTYPE_LASER",
            "WEAPONDROPTYPE_FLAME",
        };

        static int dropCounts[ARRAY_ELEMENTS(dropTypeStr)] = {};
        static bool customLabels = false;

        ImGui::Checkbox("Custom histogram labels", &customLabels);

        if (customLabels)
        {
            ImGui::PlotHistogram("Type histogram", IntAsFloat, WeaponDropTypeStr, dropCounts, dropTypeStr, ARRAY_ELEMENTS(dropCounts), 0, NULL, 0.0f, FLT_MAX, ImVec2(0.0f, 200.0f));
        }
        else
        {
            ImGui::PlotHistogram("Type histogram", IntAsFloat, NULL, dropCounts, NULL, ARRAY_ELEMENTS(dropCounts), 0, NULL, 0.0f, FLT_MAX, ImVec2(0.0f, 200.0f));
        }

        static int numToDrop = 1;
        ImGui::SliderInt("Drops to generate", &numToDrop, 1, 100000);

        if (ImGui::Button("Drop"))
        {
            for (int i = 0; i < numToDrop; ++i)
            {
                ++dropCounts[rand() % ARRAY_ELEMENTS(dropCounts)];
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear statistics"))
        {
            memset(dropCounts, 0, sizeof(dropCounts));
        }

        ImGui::End();

        // Rendering
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return 0;
}
