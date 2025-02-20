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
#pragma once
#include <webgpu/webgpu.h>
#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif
WGPUSurface SDL_getWGPUSurface(WGPUInstance instance, SDL_Window* window);
#ifdef __cplusplus
}
#endif

