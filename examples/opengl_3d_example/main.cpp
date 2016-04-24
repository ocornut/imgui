// ImGui - standalone example application for Glfw + OpenGL 2, using fixed pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
/*
    This is a adaptation of example: opengl_example, here we show how create a simple 3d world in background of ImGui.
    - Johnathan Fercher
*/

#include <imgui.h>
#include "imgui_impl_glfw.h"
#include <stdio.h>
#include <GL/glew.h>        // to use glutPerspective
#include <GLFW/glfw3.h>
#include "math.h"           // to use sin, cos ainda M_PI

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

void drawCube(float orientation[3], float position[3], float scale[3], float tone){
    // vertices 
    GLfloat vertices[] =
    {
        -1, -1, -1,   -1, -1,  1,   -1,  1,  1,   -1,  1, -1,
        1, -1, -1,    1, -1,  1,    1,  1,  1,    1,  1, -1,
        -1, -1, -1,   -1, -1,  1,    1, -1,  1,    1, -1, -1,
        -1,  1, -1,   -1,  1,  1,    1,  1,  1,    1,  1, -1,
        -1, -1, -1,   -1,  1, -1,    1,  1, -1,    1, -1, -1,
        -1, -1,  1,   -1,  1,  1,    1,  1,  1,    1, -1,  1
    };

    // Define color gray
    GLfloat colors[72];
    for(int i = 0 ; i < 72 ; i++){
        colors[i] = tone;
    }

    glPushMatrix();
        glRotatef(orientation[0], 1, 0, 0);
        glRotatef(orientation[1], 0, 1, 0);
        glRotatef(orientation[2], 0, 0, 1);
        glTranslatef(position[0], position[1], position[2]);
        glScalef(scale[0], scale[1], scale[2]);
        
        //We have a color array and a vertex array 
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, vertices);
        glColorPointer(3, GL_FLOAT, 0, colors);

        //Send data : 24 vertices 
        glDrawArrays(GL_QUADS, 0, 24);

        //Cleanup states 
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
}

void drawSphere(double r, int lats, int longs, int x, int y, int z){
    glPushMatrix();
        glTranslatef(x, y+0.15, z);
        glScalef(0.47, 0.47, 0.47);

        int i, j;

        for(i = 0; i <= lats; i++){
            double lat0 = M_PI * (-0.5 + (double) (i - 1) / lats);
            double z0  = sin(lat0);
            double zr0 =  cos(lat0);

            double lat1 = M_PI * (-0.5 + (double) i / lats);
            double z1 = sin(lat1);
            double zr1 = cos(lat1);

            glColor3f(1, 0.4, 0.1);
            glBegin(GL_QUAD_STRIP);

            for(j = 0; j <= longs; j++) {
                double lng = 2 * M_PI * (double) (j - 1) / longs;
                double x = cos(lng);
                double y = sin(lng);

                glNormal3f(x * zr0, y * zr0, z0);
                glVertex3f(x * zr0, y * zr0, z0);
                glNormal3f(x * zr1, y * zr1, z1);
                glVertex3f(x * zr1, y * zr1, z1);
            }
            glEnd();
        }
    glPopMatrix();

}

int main(int, char**)
{
    float rotateCube = 0; // Apply rotation on cube
    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui OpenGL2 3D example", NULL, NULL);
    glfwMakeContextCurrent(window);

    // Setup ImGui binding
    ImGui_ImplGlfw_Init(window, true);

    ImVec4 clear_color = ImColor(114, 144, 154);

    glEnable(GL_DEPTH_TEST); // Depth Testing
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        ImGui_ImplGlfw_NewFrame();

        if (ImGui::BeginMainMenuBar()){
            if (ImGui::BeginMenu("Example")){
                ImGui::MenuItem("Sub", "OK");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Rendering
        int display_w, display_h;
        glfwGetWindowSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        /*Here we add some code to add the 3d world*/{
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();

            gluPerspective(60, (double)display_w / (double)display_h, 0.1, 3000);

            glMatrixMode(GL_MODELVIEW);

            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glLoadIdentity();

            // Positions the "camera"
            // Both respectively X, Y and Z
            int orientation[3] = { 60, 0, 0 };
            int position[3] = { 0, -20, -15 };

            glRotatef(orientation[0], 1, 0, 0);
            glRotatef(orientation[1], 0, 1, 0);
            glRotatef(orientation[2], 0, 0, 1);
            glTranslatef(position[0], position[1], position[2]);

            // Draw objects
            float orientationFloor[3] = { 0, 0, 0 }; // Radians
            float positionFloor[3] = { 0, 0, 0 };
            float scaleFloor[3] = { 15, 0.05, 13 };
            drawCube(orientationFloor, positionFloor, scaleFloor, 0.1);

            drawSphere(1, 10, 10, 3, 0, 0);

            float orientationCube[3] = { 0, rotateCube, 0 }; // Radians
            float positionCube[3] = { 0, 1, 0 };
            float scaleCube[3] = { 1, 1 ,1 };
            drawCube(orientationCube, positionCube, scaleCube, 0.2);
            rotateCube += 0.2;
        }

        ImGui::Render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return 0;
}
