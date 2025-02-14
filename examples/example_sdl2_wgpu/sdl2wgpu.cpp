//------------------------------------------------------------------------------
//  Copyright (c) 2018-2025 Michele Morrone
//  All rights reserved.
//
//  https://michelemorrone.eu - https://brutpitt.com
//
//  X: https://x.com/BrutPitt - GitHub: https://github.com/BrutPitt
//
//  direct mail: brutpitt(at)gmail.com - me(at)michelemorrone.eu
//
//  This software is distributed under the terms of the BSD 2-Clause license
//------------------------------------------------------------------------------
#include "sdl2wgpu.h"
#include <SDL_syswm.h>

WGPUSurface SDL_getWGPUSurface(WGPUInstance instance, SDL_Window* window)
{
    SDL_SysWMinfo sysWMInfo;
    SDL_VERSION(&sysWMInfo.version);
    SDL_GetWindowWMInfo(window, &sysWMInfo);

#if defined(SDL_VIDEO_DRIVER_X11)
    const WGPUChainedStruct chainedStruct { {}, WGPUSType_SurfaceSourceXlibWindow };
    const WGPUSurfaceSourceXlibWindow surfaceSourceDesc { chainedStruct , sysWMInfo.info.x11.display, sysWMInfo.info.x11.window };
    const WGPUSurfaceDescriptor surfaceDescriptor { (WGPUChainedStruct*) &surfaceSourceDesc, nullptr };
    return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);

#elif defined(SDL_VIDEO_DRIVER_WAYLAND)
    const WGPUChainedStruct chainedStruct { {}, WGPUSType_SurfaceSourceWaylandSurface };
    const WGPUSurfaceSourceWaylandSurface surfaceSourceDesc { chainedStruct , sysWMInfo.info.wl.display, sysWMInfo.info.wl.surface };
    const WGPUSurfaceDescriptor surfaceDescriptor { (WGPUChainedStruct*) &surfaceSourceDesc, nullptr };
    return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);

#elif defined(SDL_VIDEO_DRIVER_WINDOWS)
    const WGPUChainedStruct chainedStruct { {}, WGPUSType_SurfaceSourceWindowsHWND };
    const WGPUSurfaceSourceWindowsHWND surfaceSourceDesc { chainedStruct , sysWMInfo.info.win.hinstance, sysWMInfo.info.win.window };
    const WGPUSurfaceDescriptor surfaceDescriptor { (WGPUChainedStruct*) &surfaceSourceDesc, nullptr };
    return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);

#elif defined(SDL_VIDEO_DRIVER_EMSCRIPTEN) //deprecated for DAWN, but yet used from EMSCRIPTEN
    const WGPUChainedStruct chainedStruct { {}, WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector };
    const WGPUSurfaceDescriptorFromCanvasHTMLSelector surfaceSourceDesc = { chainedStruct, "canvas" };
    const WGPUSurfaceDescriptor surfaceDescriptor { (WGPUChainedStruct*) &surfaceSourceDesc, nullptr };
    return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
/*
#elif defined(SDL_VIDEO_DRIVER_EMSCRIPTEN)  // NEW for DAWN but not used from EMSCRIPTEN (yet)
    const WGPUChainedStruct chainedStruct { {}, WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector };
    const WGPUEmscriptenSurfaceSourceCanvasHTMLSelector surfaceSourceDesc = { chainedStruct, "canvas" };
    const WGPUSurfaceDescriptor surfaceDescriptor { (WGPUChainedStruct*) &surfaceSourceDesc, nullptr };
    return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
 */
#else
#error "Unsupported WGPU Backend"
#endif
}

