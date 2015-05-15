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
 *  \file SDL_gesture.h
 *
 *  Include file for SDL gesture event handling.
 */

#ifndef _SDL_gesture_h
#define _SDL_gesture_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"

#include "SDL_touch.h"


#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef Sint64 SDL_GestureID;

/* Function prototypes */

/**
 *  \brief Begin Recording a gesture on the specified touch, or all touches (-1)
 *
 *
 */
extern DECLSPEC int SDLCALL SDL_RecordGesture(SDL_TouchID touchId);


/**
 *  \brief Save all currently loaded Dollar Gesture templates
 *
 *
 */
extern DECLSPEC int SDLCALL SDL_SaveAllDollarTemplates(SDL_RWops *dst);

/**
 *  \brief Save a currently loaded Dollar Gesture template
 *
 *
 */
extern DECLSPEC int SDLCALL SDL_SaveDollarTemplate(SDL_GestureID gestureId,SDL_RWops *dst);


/**
 *  \brief Load Dollar Gesture templates from a file
 *
 *
 */
extern DECLSPEC int SDLCALL SDL_LoadDollarTemplates(SDL_TouchID touchId, SDL_RWops *src);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_gesture_h */

/* vi: set ts=4 sw=4 expandtab: */
