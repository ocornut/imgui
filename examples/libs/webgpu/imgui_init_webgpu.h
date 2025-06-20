#pragma once

#include <stdio.h>
#include <stdlib.h>

#ifdef IMGUI_EXAMPLE_GLFW_WGPU
    #include <GLFW/glfw3.h>
#elif IMGUI_EXAMPLE_SDL2_WGPU
    #define SDL_MAIN_HANDLED
    #include <SDL.h>
    #elif IMGUI_EXAMPLE_SDL3_WGPU
    #include <SDL3/SDL.h>
#endif

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5.h>

    #if defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
        #include <emscripten/html5_webgpu.h>
    #endif
    #include <webgpu/webgpu.h>
    #include <webgpu/webgpu_cpp.h>
    #include "../emscripten/emscripten_mainloop_stub.h"
#else
    #if defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
        #include <webgpu/webgpu_glfw.h>
    #else
        #include <webgpu/wgpu.h>
    #endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

    #ifdef IMGUI_EXAMPLE_GLFW_WGPU
        WGPUSurface GLFW_getWGPUSurface(WGPUInstance instance, void* window);
    #elif IMGUI_EXAMPLE_SDL2_WGPU
        WGPUSurface SDL2_getWGPUSurface(WGPUInstance instance, SDL_Window* window);
    #elif IMGUI_EXAMPLE_SDL3_WGPU
        WGPUSurface SDL3_getWGPUSurface(WGPUInstance instance, SDL_Window* window);
    #endif

#ifdef __cplusplus
}
#endif

extern WGPUInstance             wgpu_instance;
extern WGPUDevice               wgpu_device;
extern WGPUSurface              wgpu_surface;
extern WGPUQueue                wgpu_queue;
extern WGPUTextureFormat        wgpu_preferred_fmt;
extern WGPUSurfaceConfiguration wgpu_surface_configuration;
extern int                      wgpu_surface_width;
extern int                      wgpu_surface_height;

