// dear imgui: "null" example application
// (compile and link imgui, create context, run headless with NO INPUTS, NO GRAPHICS OUTPUT)
// This is useful to test building, but you cannot interact with anything here!
#include "imgui.h"
#include <stdio.h>

// For imgui_impl_null: use relative filename + embed implementation directly by including the .cpp file.
// This is to simplify casual building of this example from all sorts of test scripts.
#include "../../backends/imgui_impl_null.h"
#include "../../backends/imgui_impl_null.cpp"

int main(int, char**)
{
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplNullPlatform_Init();
    ImGui_ImplNullRender_Init();

    for (int n = 0; n < 20; n++)
    {
        printf("NewFrame() %d\n", n);
        ImGui_ImplNullPlatform_NewFrame();
        ImGui_ImplNullRender_NewFrame();
        ImGui::NewFrame();

        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::ShowDemoWindow(nullptr);

        ImGui::Render();
    }

    printf("DestroyContext()\n");
    ImGui_ImplNullRender_Shutdown();
    ImGui_ImplNullPlatform_Shutdown();
    ImGui::DestroyContext();
    return 0;
}
