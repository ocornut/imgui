// ImGui GLFW bindings
// https://github.com/ocornut/imgui

struct GLFWwindow;

bool        ImGui_ImplGlfw_Init(GLFWwindow* window, bool install_callbacks);
void        ImGui_ImplGlfw_Shutdown();
void        ImGui_ImplGlfw_LoadFontsTexture();
void        ImGui_ImplGlfw_NewFrame();

// GLFW callbacks (installed by default if you enable 'install_callbacks' during initialization)
// Provide here if you want to chain callbacks
void        ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void        ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void        ImGui_ImplGlFw_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void        ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c);
