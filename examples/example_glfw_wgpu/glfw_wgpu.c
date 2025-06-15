#include <webgpu/wgpu.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

WGPUSurface GLFW_getWGPUSurface(WGPUInstance instance, GLFWwindow* window)
{
    WGPUSurfaceDescriptor surfaceDescriptor = {};

#if defined(GLFW_EXPOSE_NATIVE_COCOA)
    {
        id metal_layer = NULL;
        NSWindow *ns_window = glfwGetCocoaWindow(window);
        [ns_window.contentView setWantsLayer:YES];
        metal_layer = [CAMetalLayer layer];
        [ns_window.contentView setLayer:metal_layer];

        WGPUSurfaceSourceMetalLayer surfaceMetal = {};
        surfaceMetal.chain   = (const WGPUChainedStruct){ .sType = WGPUSType_SurfaceSourceMetalLayer, };
        surfaceMetal.layer = metal_layer;

        surfaceDescriptor.nextInChain = (const WGPUChainedStruct *) &surfaceMetal;
    }
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND) && defined(GLFW_EXPOSE_NATIVE_X11)
    if (glfwGetPlatform() == GLFW_PLATFORM_X11) {
        Display *x11_display = glfwGetX11Display();
        Window x11_window = glfwGetX11Window(window);

        WGPUSurfaceSourceXlibWindow surfaceXlib = {};
        surfaceXlib.chain   = (const WGPUChainedStruct) { .sType = WGPUSType_SurfaceSourceXlibWindow, };
        surfaceXlib.display = x11_display;
        surfaceXlib.window  = x11_window;

        surfaceDescriptor.nextInChain = (const WGPUChainedStruct *) &surfaceXlib;
    }

    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
        struct wl_display *wayland_display = glfwGetWaylandDisplay();
        struct wl_surface *wayland_surface = glfwGetWaylandWindow(window);

        WGPUSurfaceSourceWaylandSurface surfaceWayland = {};
        surfaceWayland.chain   = (const WGPUChainedStruct) { .sType = WGPUSType_SurfaceSourceWaylandSurface, };
        surfaceWayland.display = wayland_display;
        surfaceWayland.surface = wayland_surface;

        surfaceDescriptor.nextInChain = (const WGPUChainedStruct *) &surfaceWayland;
    }
#elif defined(GLFW_EXPOSE_NATIVE_WIN32)
    {
        HWND hwnd = glfwGetWin32Window(window);
        HINSTANCE hinstance = GetModuleHandle(NULL);

        WGPUSurfaceSourceWindowsHWND surfaceHWND = {};
        surfaceHWND.chain     = (const WGPUChainedStruct) { .sType = WGPUSType_SurfaceSourceWindowsHWND, };
        surfaceHWND.hinstance = hinstance;
        surfaceHWND.hwnd      = hwnd;

        surfaceDescriptor.nextInChain = (const WGPUChainedStruct *) &surfaceHWND;
    }
#else
#error "Unsupported GLFW native platform"
#endif
    return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
}


