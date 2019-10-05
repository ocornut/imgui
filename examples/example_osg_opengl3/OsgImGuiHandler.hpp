#pragma once

#include <osgViewer/ViewerEventHandlers>

namespace osg {
class Camera;
}

class OsgImGuiHandler : public osgGA::GUIEventHandler
{
public:
    OsgImGuiHandler();

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;

protected:
    // Put your ImGui code inside this function
    virtual void drawUi() = 0;

private:
    void init();

    void setCameraCallbacks(osg::Camera* camera);

    void newFrame(osg::RenderInfo& theRenderInfo);

    void render(osg::RenderInfo& theRenderInfo);

private:
    struct ImGuiNewFrameCallback;
    struct ImGuiRenderCallback;

    double time_;
    bool mousePressed_[3];
    float mouseWheel_;
    bool initialized_;
};
