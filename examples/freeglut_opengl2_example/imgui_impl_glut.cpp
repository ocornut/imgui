// ImGui Glut binding with OpenGL
// You can copy and use unmodified imgui_impl_* files in your project.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// See main.cpp for an example of using this.
// https://github.com/ocornut/imgui


#include "imgui.h"
#include "imgui_impl_glut.h"

#include "freeglut.h"

#include <iostream>

// Data
static int          oldTimeSinceStart = 0;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplGLUT_RenderDrawLists(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // We are using the OpenGL fixed pipeline to make the example code simpler to read!
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
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

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
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
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
            }
            idx_buffer += pcmd->ElemCount;
        }
    }
    #undef OFFSETOF

    // Restore modified state
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

static const char* ImGui_ImplGLUT_GetClipboardText()
{
    std::cerr << "ImGui_ImplGLUT_GetClipboardText not implemented\n";
    return "";
}

static void ImGui_ImplGLUT_SetClipboardText(const char* text)
{
}

bool ImGui_ImplGLUT_CreateDeviceObjects()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

void ImGui_ImplGLUT_InvalidateDeviceObjects()
{
    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }
}

bool ImGui_ImplGLUT_Init()
{
    ImGuiIO& io = ImGui::GetIO();

    io.RenderDrawListsFn = ImGui_ImplGLUT_RenderDrawLists;   // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = ImGui_ImplGLUT_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGLUT_GetClipboardText;

    io.KeyMap[ImGuiKey_Tab]        = 9;                     // tab
    io.KeyMap[ImGuiKey_LeftArrow]  = GLUT_KEY_LEFT;         // Left
    io.KeyMap[ImGuiKey_RightArrow] = GLUT_KEY_RIGHT;        // Right
    io.KeyMap[ImGuiKey_UpArrow]    = GLUT_KEY_UP;           // Up
    io.KeyMap[ImGuiKey_DownArrow]  = GLUT_KEY_DOWN;         // Down
    io.KeyMap[ImGuiKey_PageUp]     = GLUT_KEY_PAGE_UP;      // PageUp
    io.KeyMap[ImGuiKey_PageDown]   = GLUT_KEY_PAGE_DOWN;    // PageDown
    io.KeyMap[ImGuiKey_Home]       = GLUT_KEY_HOME;         // Home
    io.KeyMap[ImGuiKey_End]        = GLUT_KEY_END;          // End
    io.KeyMap[ImGuiKey_Delete]     = 127;                   // Delete
    io.KeyMap[ImGuiKey_Backspace]  = 8;                     // Backspace
    io.KeyMap[ImGuiKey_Enter]      = 13;                    // Enter
    io.KeyMap[ImGuiKey_Escape]     = 27;                    // Escape
    io.KeyMap[11]                  = 1;                     // ctrl-A
    io.KeyMap[12]                  = 3;                     // ctrl-C
    io.KeyMap[13]                  = 22;                    // ctrl-V
    io.KeyMap[14]                  = 24;                    // ctrl-X
    io.KeyMap[15]                  = 25;                    // ctrl-Y
    io.KeyMap[16]                  = 26;                    // ctrl-Z

    return true;
}

void ImGui_ImplGLUT_Shutdown()
{
    ImGui_ImplGLUT_InvalidateDeviceObjects();
    ImGui::Shutdown();
}

void ImGui_ImplGLUT_NewFrame(int w, int h)
{
    if (!g_FontTexture)
        ImGui_ImplGLUT_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2((float)w, (float)h);

    // Setup time step
    int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (timeSinceStart - oldTimeSinceStart)/1000.0f;

    io.DeltaTime = deltaTime > 0.0 ? deltaTime : (float)(1.0f / 60.0f);
    oldTimeSinceStart = timeSinceStart;

    // Start the frame
    ImGui::NewFrame();
}