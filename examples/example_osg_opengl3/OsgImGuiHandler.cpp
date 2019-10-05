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

//////////////////////////////////////////////////////////////////////////////
// Imporant Note: Dear ImGui expects the control Keys indices not to be	    //
// greater thant 511. It actually uses an array of 512 elements. However,   //
// OSG has indices greater than that. So here I do a conversion for special //
// keys between ImGui and OSG.						                        //
//////////////////////////////////////////////////////////////////////////////

/**
 * Special keys that are usually greater than 512 in OSGga
 **/
enum ConvertedKey : int
{
    ConvertedKey_Tab = 257,
    ConvertedKey_Left,
    ConvertedKey_Right,
    ConvertedKey_Up,
    ConvertedKey_Down,
    ConvertedKey_PageUp,
    ConvertedKey_PageDown,
    ConvertedKey_Home,
    ConvertedKey_End,
    ConvertedKey_Delete,
    ConvertedKey_BackSpace,
    ConvertedKey_Enter,
    ConvertedKey_Escape,
    // Modifiers
    ConvertedKey_LeftControl,
    ConvertedKey_RightControl,
    ConvertedKey_LeftShift,
    ConvertedKey_RightShift,
    ConvertedKey_LeftAlt,
    ConvertedKey_RightAlt,
    ConvertedKey_LeftSuper,
    ConvertedKey_RightSuper
};

/**
 * Check for a special key and return the converted code (range [257, 511]) if
 * so. Otherwise returns -1
 */
static int ConvertFromOSGKey(int key)
{
    using KEY = osgGA::GUIEventAdapter::KeySymbol;

    switch (key)
    {
        default: // Not found
            return -1;
        case KEY::KEY_Tab:
            return ConvertedKey_Tab;
        case KEY::KEY_Left:
            return ConvertedKey_Left;
        case KEY::KEY_Right:
            return ConvertedKey_Right;
        case KEY::KEY_Up:
            return ConvertedKey_Up;
        case KEY::KEY_Down:
            return ConvertedKey_Down;
        case KEY::KEY_Page_Up:
            return ConvertedKey_PageUp;
        case KEY::KEY_Page_Down:
            return ConvertedKey_PageDown;
        case KEY::KEY_Home:
            return ConvertedKey_Home;
        case KEY::KEY_End:
            return ConvertedKey_End;
        case KEY::KEY_Delete:
            return ConvertedKey_Delete;
        case KEY::KEY_BackSpace:
            return ConvertedKey_BackSpace;
        case KEY::KEY_Return:
            return ConvertedKey_Enter;
        case KEY::KEY_Escape:
            return ConvertedKey_Escape;
        case KEY::KEY_Control_L:
            return ConvertedKey_LeftControl;
        case KEY::KEY_Control_R:
            return ConvertedKey_RightControl;
        case KEY::KEY_Shift_L:
            return ConvertedKey_LeftShift;
        case KEY::KEY_Shift_R:
            return ConvertedKey_RightShift;
        case KEY::KEY_Alt_L:
            return ConvertedKey_LeftAlt;
        case KEY::KEY_Alt_R:
            return ConvertedKey_RightAlt;
        case KEY::KEY_Super_L:
            return ConvertedKey_LeftSuper;
        case KEY::KEY_Super_R:
            return ConvertedKey_RightSuper;
    }
    assert(false && "Switch has a default case");
    return -1;
}

void OsgImGuiHandler::init()
{
    ImGuiIO& io = ImGui::GetIO();

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_Tab] = ConvertedKey_Tab;
    io.KeyMap[ImGuiKey_LeftArrow] = ConvertedKey_Left;
    io.KeyMap[ImGuiKey_RightArrow] = ConvertedKey_Right;
    io.KeyMap[ImGuiKey_UpArrow] = ConvertedKey_Up;
    io.KeyMap[ImGuiKey_DownArrow] = ConvertedKey_Down;
    io.KeyMap[ImGuiKey_PageUp] = ConvertedKey_PageUp;
    io.KeyMap[ImGuiKey_PageDown] = ConvertedKey_PageDown;
    io.KeyMap[ImGuiKey_Home] = ConvertedKey_Home;
    io.KeyMap[ImGuiKey_End] = ConvertedKey_End;
    io.KeyMap[ImGuiKey_Delete] = ConvertedKey_Delete;
    io.KeyMap[ImGuiKey_Backspace] = ConvertedKey_BackSpace;
    io.KeyMap[ImGuiKey_Enter] = ConvertedKey_Enter;
    io.KeyMap[ImGuiKey_Escape] = ConvertedKey_Escape;
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
        {
            const int c = ea.getKey();
            const int special_key = ConvertFromOSGKey(c);
            if (special_key > 0)
            {
                assert(special_key < 512 && "ImGui KeysDown is an array of 512");
                assert(special_key > 256 && "ASCII stop at 127, but we use the range [257, 511]");

                io.KeysDown[special_key] = true;

                io.KeyCtrl = io.KeysDown[ConvertedKey_LeftControl] || io.KeysDown[ConvertedKey_RightControl];
                io.KeyShift = io.KeysDown[ConvertedKey_LeftShift] || io.KeysDown[ConvertedKey_RightShift];
                io.KeyAlt = io.KeysDown[ConvertedKey_LeftAlt] || io.KeysDown[ConvertedKey_RightAlt];
                io.KeySuper = io.KeysDown[ConvertedKey_LeftSuper] || io.KeysDown[ConvertedKey_RightSuper];
            }
            else if (c > 0 && c < 0x10000)
            {
                io.AddInputCharacter((unsigned short)c);
            }
            return wantCapureKeyboard;
        }
        case osgGA::GUIEventAdapter::KEYUP:
        {
            const int c = ea.getKey();
            const int special_key = ConvertFromOSGKey(c);
            if (special_key > 0)
            {
                assert(special_key < 512 && "ImGui KeysMap is an array of 512");
                assert(special_key > 256 && "ASCII stop at 127, but we use the range [257, 511]");

                io.KeysDown[special_key] = false;

                io.KeyCtrl = io.KeysDown[ConvertedKey_LeftControl] || io.KeysDown[ConvertedKey_RightControl];
                io.KeyShift = io.KeysDown[ConvertedKey_LeftShift] || io.KeysDown[ConvertedKey_RightShift];
                io.KeyAlt = io.KeysDown[ConvertedKey_LeftAlt] || io.KeysDown[ConvertedKey_RightAlt];
                io.KeySuper = io.KeysDown[ConvertedKey_LeftSuper] || io.KeysDown[ConvertedKey_RightSuper];
            }
            return wantCapureKeyboard;
        }
        case (osgGA::GUIEventAdapter::PUSH):
        {
            io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
            mousePressed_[0] = true;
            return wantCapureMouse;
        }
        case (osgGA::GUIEventAdapter::DRAG):
        case (osgGA::GUIEventAdapter::MOVE):
        {
            io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
            return wantCapureMouse;
        }
        case (osgGA::GUIEventAdapter::RELEASE):
        {
            mousePressed_[0] = false;
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
