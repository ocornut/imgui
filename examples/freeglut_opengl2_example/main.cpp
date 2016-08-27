// ImGui - standalone example application for Glut + OpenGL2
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "freeglut.h"

#include "imgui.h"
#include "imgui_impl_glut.h"

#include <iostream>
using namespace std;

unsigned int screenWidth = 1280;
unsigned int screenHeight = 720;
bool show_test_window = true;
bool show_another_window = false;

void drawGUI()
{
    ImGui_ImplGLUT_NewFrame(screenWidth, screenHeight);

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    {
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        if (ImGui::Button("Test Window")) show_test_window ^= 1;
        if (ImGui::Button("Another Window")) show_another_window ^= 1;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
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

    ImGui::Render();
}

void drawScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // add code here to draw scene objects
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_TRIANGLES);
    glVertex3f(-1.0, -1.0, 0.0);
    glVertex3f(1.0, -1.0, 0.0);
    glVertex3f(0.0, 1.0, 0.0);
    glEnd();

    // draw gui, seems unnecessary to push/pop matrix and attributes
    //glPushMatrix();
    //glPushAttrib(GL_ALL_ATTRIB_BITS);
    drawGUI();
    //glPopAttrib();
    //glPopMatrix();

    glutSwapBuffers();
    glutPostRedisplay();
}

bool keyboardEvent(unsigned char nChar, int nX, int nY)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(nChar);

    if (nChar == 27) //Esc-key
        glutLeaveMainLoop();

    return true;
}

void KeyboardSpecial(int key, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(key);
}

bool mouseEvent(int button, int state, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);

    if (state == GLUT_DOWN && (button == GLUT_LEFT_BUTTON))
        io.MouseDown[0] = true;
    else
        io.MouseDown[0] = false;

    if (state == GLUT_DOWN && (button == GLUT_RIGHT_BUTTON))
        io.MouseDown[1] = true;
    else
        io.MouseDown[1] = false;

    return true;
}

void mouseWheel(int button, int dir, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);
    if (dir > 0)
    {
        // Zoom in
        io.MouseWheel = 1.0;
    }
    else if (dir < 0)
    {
        // Zoom out
        io.MouseWheel = -1.0;
    }
}

void reshape(int w, int h)
{
    // update screen width and height for imgui new frames
    screenWidth = w;
    screenHeight = h;

    // Prevent a divide by zero, when window is too short
    // (you cant make a window of zero width).
    if (h == 0)
        h = 1;
    float ratio = 1.0* w / h;

    // Use the Projection Matrix
    glMatrixMode(GL_PROJECTION);

    // Reset Matrix
    glLoadIdentity();

    // Set the viewport to be the entire window
    glViewport(0, 0, w, h);

    // Set the correct perspective.
    gluPerspective(45, ratio, 1, 1000);

    // Get Back to the Modelview
    glMatrixMode(GL_MODELVIEW);
    glTranslatef(0.0, 0.0, -2.0);
}

void keyboardCallback(unsigned char nChar, int x, int y)
{
    if (keyboardEvent(nChar, x, y))
    {
        glutPostRedisplay();
    }
}

void mouseCallback(int button, int state, int x, int y)
{
    if (mouseEvent(button, state, x, y))
    {
        glutPostRedisplay();
    }
}

void mouseDragCallback(int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);

    glutPostRedisplay();
}

void mouseMoveCallback(int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);

    glutPostRedisplay();
}

// initialize ogl and imgui
void init()
{
    glClearColor(0.447f, 0.565f, 0.604f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplGLUT_Init();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_MULTISAMPLE);

    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(200, 200);
    glutCreateWindow("imgui FreeGlut Example");

    // callback
    glutDisplayFunc(drawScene);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardCallback);
    glutSpecialFunc(KeyboardSpecial);
    glutMouseFunc(mouseCallback);
    glutMouseWheelFunc(mouseWheel);
    glutMotionFunc(mouseDragCallback);
    glutPassiveMotionFunc(mouseMoveCallback);

    init();
    glutMainLoop();

    ImGui_ImplGLUT_Shutdown();

    return 0;
}
