#include "OsgImGuiHandler.hpp"

#include <osg/Camera>
#include <osgUtil/GLObjectsVisitor>
#include <osgUtil/SceneView>
#include <osgUtil/UpdateVisitor>
#include <osgViewer/ViewerEventHandlers>

#include "imgui.h"
#include "imgui_impl_opengl3.h"

struct OsgImGuiHandler::ImGuiNewFrameCallback : public osg::Camera::DrawCallback
{
    ImGuiNewFrameCallback(OsgImGuiHandler& handler)
        : handler_(handler)
    {
    }

    void operator()(osg::RenderInfo& renderInfo) const override
    {
        handler_.newFrame(renderInfo);
    }

private:
    OsgImGuiHandler& handler_;
};

struct OsgImGuiHandler::ImGuiRenderCallback : public osg::Camera::DrawCallback
{
    ImGuiRenderCallback(OsgImGuiHandler& handler)
        : handler_(handler)
    {
    }

    void operator()(osg::RenderInfo& renderInfo) const override
    {
        handler_.render(renderInfo);
    }

private:
    OsgImGuiHandler& handler_;
};

OsgImGuiHandler::OsgImGuiHandler()
    : time_(0.0f), mousePressed_{false}, mouseWheel_(0.0f), initialized_(false)
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    init();
}

/**
 * Imporant Note: Dear ImGui expects the control Keys indices not to be
 * greater thant 511. It actually uses an array of 512 elements. However,
 * OSG has indices greater than that. So here I do a conversion for special
 * keys between ImGui and OSG.
 */
static int ConvertFromOSGKey(int key)
{
    using KEY = osgGA::GUIEventAdapter::KeySymbol;

    switch (key)
    {
        case KEY::KEY_Tab:
            return ImGuiKey_Tab;
        case KEY::KEY_Left:
            return ImGuiKey_LeftArrow;
        case KEY::KEY_Right:
            return ImGuiKey_RightArrow;
        case KEY::KEY_Up:
            return ImGuiKey_UpArrow;
        case KEY::KEY_Down:
            return ImGuiKey_DownArrow;
        case KEY::KEY_Page_Up:
            return ImGuiKey_PageUp;
        case KEY::KEY_Page_Down:
            return ImGuiKey_PageDown;
        case KEY::KEY_Home:
            return ImGuiKey_Home;
        case KEY::KEY_End:
            return ImGuiKey_End;
        case KEY::KEY_Delete:
            return ImGuiKey_Delete;
        case KEY::KEY_BackSpace:
            return ImGuiKey_Backspace;
        case KEY::KEY_Return:
            return ImGuiKey_Enter;
        case KEY::KEY_Escape:
            return ImGuiKey_Escape;
        default: // Not found
            return -1;
    }
}

void OsgImGuiHandler::init()
{
    ImGuiIO& io = ImGui::GetIO();

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_Tab] = ImGuiKey_Tab;
    io.KeyMap[ImGuiKey_LeftArrow] = ImGuiKey_LeftArrow;
    io.KeyMap[ImGuiKey_RightArrow] = ImGuiKey_RightArrow;
    io.KeyMap[ImGuiKey_UpArrow] = ImGuiKey_UpArrow;
    io.KeyMap[ImGuiKey_DownArrow] = ImGuiKey_DownArrow;
    io.KeyMap[ImGuiKey_PageUp] = ImGuiKey_PageUp;
    io.KeyMap[ImGuiKey_PageDown] = ImGuiKey_PageDown;
    io.KeyMap[ImGuiKey_Home] = ImGuiKey_Home;
    io.KeyMap[ImGuiKey_End] = ImGuiKey_End;
    io.KeyMap[ImGuiKey_Delete] = ImGuiKey_Delete;
    io.KeyMap[ImGuiKey_Backspace] = ImGuiKey_Backspace;
    io.KeyMap[ImGuiKey_Enter] = ImGuiKey_Enter;
    io.KeyMap[ImGuiKey_Escape] = ImGuiKey_Escape;
    io.KeyMap[ImGuiKey_A] = osgGA::GUIEventAdapter::KeySymbol::KEY_A;
    io.KeyMap[ImGuiKey_C] = osgGA::GUIEventAdapter::KeySymbol::KEY_C;
    io.KeyMap[ImGuiKey_V] = osgGA::GUIEventAdapter::KeySymbol::KEY_V;
    io.KeyMap[ImGuiKey_X] = osgGA::GUIEventAdapter::KeySymbol::KEY_X;
    io.KeyMap[ImGuiKey_Y] = osgGA::GUIEventAdapter::KeySymbol::KEY_Y;
    io.KeyMap[ImGuiKey_Z] = osgGA::GUIEventAdapter::KeySymbol::KEY_Z;

    ImGui_ImplOpenGL3_Init();

    io.RenderDrawListsFn = ImGui_ImplOpenGL3_RenderDrawData;
}

void OsgImGuiHandler::setCameraCallbacks(osg::Camera* camera)
{
    camera->setPreDrawCallback(new ImGuiNewFrameCallback(*this));
    camera->setPostDrawCallback(new ImGuiRenderCallback(*this));
}

void OsgImGuiHandler::newFrame(osg::RenderInfo& theRenderInfo)
{
    ImGui_ImplOpenGL3_NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    osg::Viewport* aViewport = theRenderInfo.getCurrentCamera()->getViewport();
    io.DisplaySize = ImVec2(aViewport->width(), aViewport->height());

    double aCurrentTime = theRenderInfo.getView()->getFrameStamp()->getSimulationTime();
    io.DeltaTime = time_ > 0.0 ? (float)(aCurrentTime - time_ + 0.0001) : (float)(1.0f / 60.0f);
    time_ = aCurrentTime;

    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = mousePressed_[i];
    }

    io.MouseWheel = mouseWheel_;
    mouseWheel_ = 0.0f;

    ImGui::NewFrame();
}

void OsgImGuiHandler::render(osg::RenderInfo& /*theRenderInfo*/)
{
    drawUi();
    ImGui::Render();
}

bool OsgImGuiHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (!initialized_)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (view)
        {
            setCameraCallbacks(view->getCamera());
            initialized_ = true;
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    const bool wantCapureMouse = io.WantCaptureMouse;
    const bool wantCapureKeyboard = io.WantCaptureKeyboard;

    switch (ea.getEventType())
    {
        case osgGA::GUIEventAdapter::KEYDOWN:
        case osgGA::GUIEventAdapter::KEYUP:
        {
            const bool isKeyDown = ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN;
            const int c = ea.getKey();
            const int special_key = ConvertFromOSGKey(c);
            if (special_key > 0)
            {
                assert((special_key >= 0 && special_key < 512) && "ImGui KeysMap is an array of 512");

                io.KeysDown[special_key] = isKeyDown;

                io.KeyCtrl = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL;
                io.KeyShift = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT;
                io.KeyAlt = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT;
                io.KeySuper = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SUPER;
            }
            else if (isKeyDown && c > 0 && c < 0xFF)
            {
                io.AddInputCharacter((unsigned short)c);
            }
            return wantCapureKeyboard;
        }
        case (osgGA::GUIEventAdapter::RELEASE):
        case (osgGA::GUIEventAdapter::PUSH):
        {
            io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
            mousePressed_[0] = ea.getButtonMask() & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
            mousePressed_[1] = ea.getButtonMask() & osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
            mousePressed_[2] = ea.getButtonMask() & osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
            return wantCapureMouse;
        }
        case (osgGA::GUIEventAdapter::DRAG):
        case (osgGA::GUIEventAdapter::MOVE):
        {
            io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
            return wantCapureMouse;
        }
        case (osgGA::GUIEventAdapter::SCROLL):
        {
            mouseWheel_ = ea.getScrollingDeltaY();
            return wantCapureMouse;
        }
        default:
        {
            return false;
        }
    }

    return false;
}
