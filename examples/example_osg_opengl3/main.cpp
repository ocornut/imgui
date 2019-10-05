#include <iostream>

#include "GL/glew.h"

#include <osgViewer/Viewer>
#include <osgViewer/config/SingleWindow>

#include <imgui.h>

#include "OsgImGuiHandler.hpp"

class GlewInitOperation : public osg::Operation
{
public:
    GlewInitOperation()
        : osg::Operation("GlewInitCallback", false)
    {
    }

    void operator()(osg::Object* object) override
    {
        osg::GraphicsContext* context = dynamic_cast<osg::GraphicsContext*>(object);
        if (!context)
            return;

        if (glewInit() != GLEW_OK)
        {
            std::cout << "glewInit() failed\n";
        }
    }
};

class ImGuiDemo : public OsgImGuiHandler
{
protected:
    void drawUi() override
    {
        // ImGui code goes here...
        ImGui::ShowDemoWindow();
    }
};

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;

    viewer.apply(new osgViewer::SingleWindow(100, 100, 640, 480));
    viewer.setRealizeOperation(new GlewInitOperation);
    viewer.addEventHandler(new ImGuiDemo);

    return viewer.run();
}
