//------------------------------------------------------------------------------
//  Copyright (c) 2025 Michele Morrone
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
#include <webgpu/webgpu.h>

#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_syswm.h>

#ifdef SDL_VIDEO_DRIVER_COCOA
#include <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

WGPUSurface SDL2_getWGPUSurface(WGPUInstance instance, SDL_Window* window)
{
    WGPUSurfaceDescriptor surfaceDescriptor = {};
    WGPUChainedStruct     chainedStruct     = {};
    SDL_SysWMinfo sysWMInfo;
    SDL_VERSION(&sysWMInfo.version);
    SDL_GetWindowWMInfo(window, &sysWMInfo);

#if defined(SDL_VIDEO_DRIVER_WAYLAND) || defined(SDL_VIDEO_DRIVER_X11)
    const char *vidDrv = SDL_GetHint(SDL_HINT_VIDEODRIVER);
    if(!vidDrv) return NULL;

    if(tolower(vidDrv[0])=='w' && tolower(vidDrv[1])=='a' && tolower(vidDrv[2])=='y' &&
       tolower(vidDrv[3])=='l' && tolower(vidDrv[4])=='a' && tolower(vidDrv[5])=='n' && tolower(vidDrv[6])=='d') { // wayland

        chainedStruct.sType = WGPUSType_SurfaceSourceWaylandSurface;

        WGPUSurfaceSourceWaylandSurface surfaceWayland = {};
        surfaceWayland.chain   = chainedStruct;
        surfaceWayland.display = sysWMInfo.info.wl.display;
        surfaceWayland.surface = sysWMInfo.info.wl.surface;

        surfaceDescriptor.nextInChain = &surfaceWayland.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);

    } else {    // x11

        chainedStruct.sType = WGPUSType_SurfaceSourceXlibWindow;

        WGPUSurfaceSourceXlibWindow surfaceXlib = {};
        surfaceXlib.chain   = chainedStruct;
        surfaceXlib.display = sysWMInfo.info.x11.display;
        surfaceXlib.window  = sysWMInfo.info.x11.window;

        surfaceDescriptor.nextInChain = &surfaceXlib.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#elif defined(SDL_VIDEO_DRIVER_WINDOWS)
    {
        chainedStruct.sType = WGPUSType_SurfaceSourceWindowsHWND;

        WGPUSurfaceSourceWindowsHWND surfaceHWND = {};
        surfaceHWND.chain     = chainedStruct;
        surfaceHWND.hinstance = sysWMInfo.info.win.hinstance;
        surfaceHWND.hwnd      = sysWMInfo.info.win.window;

        surfaceDescriptor.nextInChain = &surfaceHWND.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#elif defined(SDL_VIDEO_DRIVER_COCOA)
    {
        id metal_layer = [CAMetalLayer layer];
        NSWindow *ns_window = sysWMInfo.info.cocoa.window;
        [ns_window.contentView setWantsLayer:YES];
        [ns_window.contentView setLayer:metal_layer];

        chainedStruct.sType = WGPUSType_SurfaceSourceMetalLayer;

        WGPUSurfaceSourceMetalLayer surfaceMetal = {};
        surfaceMetal.chain = chainedStruct;
        surfaceMetal.layer = metal_layer;

        surfaceDescriptor.nextInChain = &surfaceMetal.chain;
        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
}
#else
    #error "Unsupported SDL2/WebGPU Backend"
#endif
}


