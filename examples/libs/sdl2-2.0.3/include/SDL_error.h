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
 *  \file SDL_error.h
 *
 *  Simple error message routines for SDL.
 */

#ifndef _SDL_error_h
#define _SDL_error_h

#include "SDL_stdinc.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Public functions */
/* SDL_SetError() unconditionally returns -1. */
extern DECLSPEC int SDLCALL SDL_SetError(const char *fmt, ...);
extern DECLSPEC const char *SDLCALL SDL_GetError(void);
extern DECLSPEC void SDLCALL SDL_ClearError(void);

/**
 *  \name Internal error functions
 *
 *  \internal
 *  Private error reporting function - used internally.
 */
/* @{ */
#define SDL_OutOfMemory()   SDL_Error(SDL_ENOMEM)
#define SDL_Unsupported()   SDL_Error(SDL_UNSUPPORTED)
#define SDL_InvalidParamError(param)    SDL_SetError("Parameter '%s' is invalid", (param))
typedef enum
{
    SDL_ENOMEM,
    SDL_EFREAD,
    SDL_EFWRITE,
    SDL_EFSEEK,
    SDL_UNSUPPORTED,
    SDL_LASTERROR
} SDL_errorcode;
/* SDL_Error() unconditionally returns -1. */
extern DECLSPEC int SDLCALL SDL_Error(SDL_errorcode code);
/* @} *//* Internal error functions */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_error_h */

/* vi: set ts=4 sw=4 expandtab: */
