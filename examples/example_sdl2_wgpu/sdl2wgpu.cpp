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
#include "sdl2wgpu.h"

#include <webgpu/webgpu.h>

#include <SDL.h>
#include <SDL_syswm.h>

#ifdef SDL_VIDEO_DRIVER_COCOA
#include <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>
#endif


WGPUSurface SDL_getWGPUSurface(WGPUInstance instance, SDL_Window* window)
{
    WGPUSurfaceDescriptor surfaceDescriptor {};
    WGPUChainedStruct     chainedStruct     {};
#ifdef __EMSCRIPTEN__
    chainedStruct.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
    WGPUSurfaceDescriptorFromCanvasHTMLSelector surfaceSourceDesc = { chainedStruct, "canvas" };
    surfaceDescriptor.nextInChain = &surfaceSourceDesc.chain;
/*
    // not used in EMSCRIPTEN (yet), but in DAWN EMSCRIPTEN fork emdawnwebgpu: https://dawn.googlesource.com/dawn/+/refs/heads/main/src/emdawnwebgpu/
    chainedStruct.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
    const WGPUEmscriptenSurfaceSourceCanvasHTMLSelector surfaceSourceDesc = { chainedStruct, "canvas" };
    surfaceDescriptor.nextInChain = &surfaceSourceDesc.chain;
*/
#else
    SDL_SysWMinfo sysWMInfo;
    SDL_VERSION(&sysWMInfo.version);
    SDL_GetWindowWMInfo(window, &sysWMInfo);

    #if defined(unix) || defined(__unix__)
        // if not specified SDL_getWGPUSurface prefers always X11, also on Wayland
        // to use Wayland is necessary to specify it in SDL_VIDEODRIVER:
        //    ==> or (inside code)  use 'SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland");' before 'SDL_Init'
        //    ==> or (outside code) export SDL_VIDEODRIVER=wayland environment variable, or
        //                          export SDL_VIDEODRIVER=$XDG_SESSION_TYPE to get current session type
        const char *vidDrv = SDL_GetHint(SDL_HINT_VIDEODRIVER);

        if(vidDrv != nullptr && (tolower(vidDrv[0])=='w' && tolower(vidDrv[1])=='a' && tolower(vidDrv[2])=='y' &&
                                 tolower(vidDrv[3])=='l' && tolower(vidDrv[4])=='a' && tolower(vidDrv[5])=='n' && tolower(vidDrv[6])=='d')) { // wayland
            #if defined(SDL_VIDEO_DRIVER_WAYLAND) // check also if the cmake option DAWN_USE_WAYLAND=ON was set (if you get: "surface not supported")
                chainedStruct.sType = WGPUSType_SurfaceSourceWaylandSurface;
                WGPUSurfaceSourceWaylandSurface surfaceSourceDesc { chainedStruct , sysWMInfo.info.wl.display, sysWMInfo.info.wl.surface };
                surfaceDescriptor.nextInChain = &surfaceSourceDesc.chain;
            #else
                #error "Wayland session, but SDL_VIDEO_DRIVER_WAYLAND missing"
            #endif
        } else {    // Wayland
            #if defined(SDL_VIDEO_DRIVER_X11)
                chainedStruct.sType = WGPUSType_SurfaceSourceXlibWindow;
                WGPUSurfaceDescriptorFromXlibWindow surfaceSourceDesc { chainedStruct , sysWMInfo.info.x11.display, sysWMInfo.info.x11.window };
                surfaceDescriptor.nextInChain = &surfaceSourceDesc.chain;
            #else
                #error "X11 session, but SDL_VIDEO_DRIVER_X11 missing"
            #endif
        }
    #elif defined(SDL_VIDEO_DRIVER_WINDOWS)
        chainedStruct.sType = WGPUSType_SurfaceSourceWindowsHWND;
        WGPUSurfaceSourceWindowsHWND surfaceSourceDesc { chainedStruct , sysWMInfo.info.win.hinstance, sysWMInfo.info.win.window };
        surfaceDescriptor.nextInChain = &surfaceSourceDesc.chain;
    #elif defined(SDL_VIDEO_DRIVER_COCOA)
        id metal_layer = [CAMetalLayer layer];
        NSWindow *ns_window = sysWMInfo.info.cocoa.window;
        [ns_window.contentView setWantsLayer:YES];
        [ns_window.contentView setLayer:metal_layer];

        chainedStruct.sType = WGPUSType_SurfaceSourceMetalLayer;
        WGPUSurfaceSourceMetalLayer surfaceSourceDesc { chainedStruct , metal_layer };
        surfaceDescriptor.nextInChain = &surfaceSourceDesc.chain;
    #else
        #error "Unsupported WGPU Backend"
    #endif
#endif
    return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);

}


