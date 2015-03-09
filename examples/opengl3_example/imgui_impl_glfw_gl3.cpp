// ImGui GLFW binding with OpenGL3 + shaders
// https://github.com/ocornut/imgui

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"

// GL3W/GLFW
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#ifdef _MSC_VER
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

static GLFWwindow*  g_window = NULL;
static bool         g_mouse_pressed[3] = { false, false, false };
static float        g_mouse_wheel = 0.0f;
static double       g_time = 0.0f;
static bool         g_font_texture_loaded = false;

// Handles for OpenGL3 rendering
static int          g_shader_handle = 0, g_vert_handle = 0, g_frag_handle = 0;
static int          g_texture_location = 0, g_proj_mtx_location = 0;
static int          g_position_location = 0, g_uv_location = 0, g_colour_location = 0;
static size_t       g_vbo_max_size = 20000;
static unsigned int g_vbo_handle = 0, g_vao_handle = 0;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
static void ImGui_ImplGlfwGL3_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
    if (cmd_lists_count == 0)
        return;

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    const float ortho_projection[4][4] =
    {
        { 2.0f/width,	0.0f,			0.0f,		0.0f },
        { 0.0f,			2.0f/-height,	0.0f,		0.0f },
        { 0.0f,			0.0f,			-1.0f,		0.0f },
        { -1.0f,		1.0f,			0.0f,		1.0f },
    };
    glUseProgram(g_shader_handle);
    glUniform1i(g_texture_location, 0);
    glUniformMatrix4fv(g_proj_mtx_location, 1, GL_FALSE, &ortho_projection[0][0]);

    // Grow our buffer according to what we need
    size_t total_vtx_count = 0;
    for (int n = 0; n < cmd_lists_count; n++)
        total_vtx_count += cmd_lists[n]->vtx_buffer.size();
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo_handle);
    size_t neededBufferSize = total_vtx_count * sizeof(ImDrawVert);
    if (neededBufferSize > g_vbo_max_size)
    {
        g_vbo_max_size = neededBufferSize + 5000;  // Grow buffer
        glBufferData(GL_ARRAY_BUFFER, g_vbo_max_size, NULL, GL_STREAM_DRAW);
    }

    // Copy and convert all vertices into a single contiguous buffer
    unsigned char* buffer_data = (unsigned char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if (!buffer_data)
        return;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        memcpy(buffer_data, &cmd_list->vtx_buffer[0], cmd_list->vtx_buffer.size() * sizeof(ImDrawVert));
        buffer_data += cmd_list->vtx_buffer.size() * sizeof(ImDrawVert);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(g_vao_handle);

    int cmd_offset = 0;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        int vtx_offset = cmd_offset;
        const ImDrawCmd* pcmd_end = cmd_list->commands.end();
        for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
        {
            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->texture_id);
            glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
            glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
            vtx_offset += pcmd->vtx_count;
        }
        cmd_offset = vtx_offset;
    }

    // Restore modified state
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static const char* ImGui_ImplGlfwGL3_GetClipboardText()
{
    return glfwGetClipboardString(g_window);
}

static void ImGui_ImplGlfwGL3_SetClipboardText(const char* text)
{
    glfwSetClipboardString(g_window, text);
}

void ImGui_ImplGlfwGL3_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS && button >= 0 && button < 3)
        g_mouse_pressed[button] = true;
}

void ImGui_ImplGlfwGL3_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_mouse_wheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
}

void ImGui_ImplGlfwGL3_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;
    io.KeyCtrl = (mods & GLFW_MOD_CONTROL) != 0;
    io.KeyShift = (mods & GLFW_MOD_SHIFT) != 0;
}

void ImGui_ImplGlfwGL3_CharCallback(GLFWwindow* window, unsigned int c)
{
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

void ImGui_ImplGlfwGL3_LoadFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)tex_id;

    g_font_texture_loaded = true;
}

static void InitGL()
{
    const GLchar *vertex_shader =
        "#version 330\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "	Frag_UV = UV;\n"
        "	Frag_Color = Color;\n"
        "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader =
        "#version 330\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
        "}\n";

    g_shader_handle = glCreateProgram();
    g_vert_handle = glCreateShader(GL_VERTEX_SHADER);
    g_frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(g_vert_handle, 1, &vertex_shader, 0);
    glShaderSource(g_frag_handle, 1, &fragment_shader, 0);
    glCompileShader(g_vert_handle);
    glCompileShader(g_frag_handle);
    glAttachShader(g_shader_handle, g_vert_handle);
    glAttachShader(g_shader_handle, g_frag_handle);
    glLinkProgram(g_shader_handle);

    g_texture_location = glGetUniformLocation(g_shader_handle, "Texture");
    g_proj_mtx_location = glGetUniformLocation(g_shader_handle, "ProjMtx");
    g_position_location = glGetAttribLocation(g_shader_handle, "Position");
    g_uv_location = glGetAttribLocation(g_shader_handle, "UV");
    g_colour_location = glGetAttribLocation(g_shader_handle, "Color");

    glGenBuffers(1, &g_vbo_handle);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo_handle);
    glBufferData(GL_ARRAY_BUFFER, g_vbo_max_size, NULL, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &g_vao_handle);
    glBindVertexArray(g_vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo_handle);
    glEnableVertexAttribArray(g_position_location);
    glEnableVertexAttribArray(g_uv_location);
    glEnableVertexAttribArray(g_colour_location);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    glVertexAttribPointer(g_position_location, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(g_uv_location, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(g_colour_location, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool    ImGui_ImplGlfwGL3_Init(GLFWwindow* window, bool install_callbacks)
{
    InitGL();

    g_window = window;

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                 // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.RenderDrawListsFn = ImGui_ImplGlfwGL3_RenderDrawLists;
    io.SetClipboardTextFn = ImGui_ImplGlfwGL3_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGlfwGL3_GetClipboardText;
#ifdef _MSC_VER
    io.ImeWindowHandle = glfwGetWin32Window(g_window);
#endif

    if (install_callbacks)
    {
        glfwSetMouseButtonCallback(window, ImGui_ImplGlfwGL3_MouseButtonCallback);
        glfwSetScrollCallback(window, ImGui_ImplGlfwGL3_ScrollCallback);
        glfwSetKeyCallback(window, ImGui_ImplGlfwGL3_KeyCallback);
        glfwSetCharCallback(window, ImGui_ImplGlfwGL3_CharCallback);
    }

    return true;
}

void ImGui_ImplGlfwGL3_Shutdown()
{
    if (g_vao_handle) glDeleteVertexArrays(1, &g_vao_handle);
    if (g_vbo_handle) glDeleteBuffers(1, &g_vbo_handle);
    g_vao_handle = 0;
    g_vbo_handle = 0;

    glDetachShader(g_shader_handle, g_vert_handle);
    glDeleteShader(g_vert_handle);
    g_vert_handle = 0;

    glDetachShader(g_shader_handle, g_frag_handle);
    glDeleteShader(g_frag_handle);
    g_frag_handle = 0;

    glDeleteProgram(g_shader_handle);
    g_shader_handle = 0;

    GLuint tex_id = (GLuint)ImGui::GetIO().Fonts->TexID;
    if (tex_id)
    {
        glDeleteTextures(1, &tex_id);
        ImGui::GetIO().Fonts->TexID = 0;
    }
    ImGui::Shutdown();
}

void ImGui_ImplGlfwGL3_NewFrame()
{
    if (!g_font_texture_loaded)
        ImGui_ImplGlfwGL3_LoadFontsTexture();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(g_window, &w, &h);
    glfwGetFramebufferSize(g_window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)display_w, (float)display_h);

    // Setup time step
    double current_time =  glfwGetTime();
    io.DeltaTime = g_time > 0.0 ? (float)(current_time - g_time) : (float)(1.0f/60.0f);
    g_time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    double mouse_x, mouse_y;
    glfwGetCursorPos(g_window, &mouse_x, &mouse_y);
    mouse_x *= (float)display_w / w;                                                                    // Convert mouse coordinates to pixels
    mouse_y *= (float)display_h / h;
    io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);                                               // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)

    io.MouseDown[0] = g_mouse_pressed[0] || glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_LEFT) != 0;     // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
    io.MouseDown[1] = g_mouse_pressed[1] || glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_RIGHT) != 0;
    io.MouseDown[2] = g_mouse_pressed[2] || glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_MIDDLE) != 0;
    g_mouse_pressed[0] = false;
    g_mouse_pressed[1] = false;
    g_mouse_pressed[2] = false;

    io.MouseWheel = g_mouse_wheel;
    g_mouse_wheel = 0.0f;

    // Start the frame
    ImGui::NewFrame();
}
