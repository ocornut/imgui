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
 *  \file SDL_bits.h
 *
 *  Functions for fiddling with bits and bitmasks.
 */

#ifndef _SDL_bits_h
#define _SDL_bits_h

#include "SDL_stdinc.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \file SDL_bits.h
 */

/**
 *  Get the index of the most significant bit. Result is undefined when called
 *  with 0. This operation can also be stated as "count leading zeroes" and
 *  "log base 2".
 *
 *  \return Index of the most significant bit, or -1 if the value is 0.
 */
SDL_FORCE_INLINE int
SDL_MostSignificantBitIndex32(Uint32 x)
{
#if defined(__GNUC__) && __GNUC__ >= 4
    /* Count Leading Zeroes builtin in GCC.
     * http://gcc.gnu.org/onlinedocs/gcc-4.3.4/gcc/Other-Builtins.html
     */
    if (x == 0) {
        return -1;
    }
    return 31 - __builtin_clz(x);
#else
    /* Based off of Bit Twiddling Hacks by Sean Eron Anderson
     * <seander@cs.stanford.edu>, released in the public domain.
     * http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
     */
    const Uint32 b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
    const int    S[] = {1, 2, 4, 8, 16};

    int msbIndex = 0;
    int i;

    if (x == 0) {
        return -1;
    }

    for (i = 4; i >= 0; i--)
    {
        if (x & b[i])
        {
            x >>= S[i];
            msbIndex |= S[i];
        }
    }

    return msbIndex;
#endif
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_bits_h */

/* vi: set ts=4 sw=4 expandtab: */
