#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui EnterKeepActive Test", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // ⭐⭐⭐ Enable the problematic flag!
    io.ConfigInputTextEnterKeepActive = true;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    static char buf[128] = "edit me";

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Test Window");

        ImGui::Text("Press Enter inside the InputText. It will stay active.");
        ImGui::Text("Watch IsItemDeactivatedAfterEdit value below.");

        ImGui::InputText("Input", buf, sizeof(buf));

        bool active = ImGui::IsItemActive();
        bool deactivated_after_edit = ImGui::IsItemDeactivatedAfterEdit();

        ImGui::Text("IsItemActive = %d", active);
        ImGui::Text("IsItemDeactivatedAfterEdit = %d", deactivated_after_edit);

        ImGui::Separator();
        ImGui::TextWrapped(
            "Expected: Enter does NOT deactivate the input.\n"
            "Observed: IsItemDeactivatedAfterEdit never becomes true.\n"
            "This reproduces issue #9001.");

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
