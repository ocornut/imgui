// ImGui GLFW binding with OpenGL3 + shaders
// https://github.com/ocornut/imgui

struct GLFWwindow;

bool        ImGui_ImplGlfwGL3_Init(GLFWwindow* window, bool install_callbacks);
void        ImGui_ImplGlfwGL3_Shutdown();
void        ImGui_ImplGlfwGL3_LoadFontsTexture();
void        ImGui_ImplGlfwGL3_NewFrame();

// GLFW callbacks (installed by default if you enable 'install_callbacks' during initialization)
// Provide here if you want to chain callbacks
void        ImGui_ImplGlfwGL3_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void        ImGui_ImplGlfwGL3_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void        ImGui_ImplGlFwGL3_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void        ImGui_ImplGlfwGL3_CharCallback(GLFWwindow* window, unsigned int c);
