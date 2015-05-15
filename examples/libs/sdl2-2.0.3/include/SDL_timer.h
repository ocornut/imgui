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

#ifndef _SDL_timer_h
#define _SDL_timer_h

/**
 *  \file SDL_timer.h
 *
 *  Header for the SDL time management routines.
 */

#include "SDL_stdinc.h"
#include "SDL_error.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Get the number of milliseconds since the SDL library initialization.
 *
 * \note This value wraps if the program runs for more than ~49 days.
 */
extern DECLSPEC Uint32 SDLCALL SDL_GetTicks(void);

/**
 * \brief Compare SDL ticks values, and return true if A has passed B
 *
 * e.g. if you want to wait 100 ms, you could do this:
 *  Uint32 timeout = SDL_GetTicks() + 100;
 *  while (!SDL_TICKS_PASSED(SDL_GetTicks(), timeout)) {
 *      ... do work until timeout has elapsed
 *  }
 */
#define SDL_TICKS_PASSED(A, B)  ((Sint32)((B) - (A)) <= 0)

/**
 * \brief Get the current value of the high resolution counter
 */
extern DECLSPEC Uint64 SDLCALL SDL_GetPerformanceCounter(void);

/**
 * \brief Get the count per second of the high resolution counter
 */
extern DECLSPEC Uint64 SDLCALL SDL_GetPerformanceFrequency(void);

/**
 * \brief Wait a specified number of milliseconds before returning.
 */
extern DECLSPEC void SDLCALL SDL_Delay(Uint32 ms);

/**
 *  Function prototype for the timer callback function.
 *
 *  The callback function is passed the current timer interval and returns
 *  the next timer interval.  If the returned value is the same as the one
 *  passed in, the periodic alarm continues, otherwise a new alarm is
 *  scheduled.  If the callback returns 0, the periodic alarm is cancelled.
 */
typedef Uint32 (SDLCALL * SDL_TimerCallback) (Uint32 interval, void *param);

/**
 * Definition of the timer ID type.
 */
typedef int SDL_TimerID;

/**
 * \brief Add a new timer to the pool of timers already running.
 *
 * \return A timer ID, or NULL when an error occurs.
 */
extern DECLSPEC SDL_TimerID SDLCALL SDL_AddTimer(Uint32 interval,
                                                 SDL_TimerCallback callback,
                                                 void *param);

/**
 * \brief Remove a timer knowing its ID.
 *
 * \return A boolean value indicating success or failure.
 *
 * \warning It is not safe to remove a timer multiple times.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_RemoveTimer(SDL_TimerID id);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_timer_h */

/* vi: set ts=4 sw=4 expandtab: */
