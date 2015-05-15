/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 *  \file SDL_cpuinfo.h
 *
 *  CPU feature detection for SDL.
 */

#ifndef _SDL_cpuinfo_h
#define _SDL_cpuinfo_h

#include "SDL_stdinc.h"

/* Need to do this here because intrin.h has C++ code in it */
/* Visual Studio 2005 has a bug where intrin.h conflicts with winnt.h */
#if defined(_MSC_VER) && (_MSC_VER >= 1500) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#ifndef _WIN64
#define __MMX__
#define __3dNOW__
#endif
#define __SSE__
#define __SSE2__
#elif defined(__MINGW64_VERSION_MAJOR)
#include <intrin.h>
#else
#ifdef __ALTIVEC__
#if HAVE_ALTIVEC_H && !defined(__APPLE_ALTIVEC__)
#include <altivec.h>
#undef pixel
#endif
#endif
#ifdef __MMX__
#include <mmintrin.h>
#endif
#ifdef __3dNOW__
#include <mm3dnow.h>
#endif
#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#endif

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* This is a guess for the cacheline size used for padding.
 * Most x86 processors have a 64 byte cache line.
 * The 64-bit PowerPC processors have a 128 byte cache line.
 * We'll use the larger value to be generally safe.
 */
#define SDL_CACHELINE_SIZE  128

/**
 *  This function returns the number of CPU cores available.
 */
extern DECLSPEC int SDLCALL SDL_GetCPUCount(void);

/**
 *  This function returns the L1 cache line size of the CPU
 *
 *  This is useful for determining multi-threaded structure padding
 *  or SIMD prefetch sizes.
 */
extern DECLSPEC int SDLCALL SDL_GetCPUCacheLineSize(void);

/**
 *  This function returns true if the CPU has the RDTSC instruction.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasRDTSC(void);

/**
 *  This function returns true if the CPU has AltiVec features.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasAltiVec(void);

/**
 *  This function returns true if the CPU has MMX features.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasMMX(void);

/**
 *  This function returns true if the CPU has 3DNow! features.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_Has3DNow(void);

/**
 *  This function returns true if the CPU has SSE features.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE(void);

/**
 *  This function returns true if the CPU has SSE2 features.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE2(void);

/**
 *  This function returns true if the CPU has SSE3 features.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE3(void);

/**
 *  This function returns true if the CPU has SSE4.1 features.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE41(void);

/**
 *  This function returns true if the CPU has SSE4.2 features.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE42(void);

/**
 *  This function returns true if the CPU has AVX features.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasAVX(void);

/**
 *  This function returns the amount of RAM configured in the system, in MB.
 */
extern DECLSPEC int SDLCALL SDL_GetSystemRAM(void);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_cpuinfo_h */

/* vi: set ts=4 sw=4 expandtab: */
