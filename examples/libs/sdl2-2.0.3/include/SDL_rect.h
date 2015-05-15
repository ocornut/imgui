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
 *  \file SDL_rect.h
 *
 *  Header file for SDL_rect definition and management functions.
 */

#ifndef _SDL_rect_h
#define _SDL_rect_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_pixels.h"
#include "SDL_rwops.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief  The structure that defines a point
 *
 *  \sa SDL_EnclosePoints
 */
typedef struct SDL_Point
{
    int x;
    int y;
} SDL_Point;

/**
 *  \brief A rectangle, with the origin at the upper left.
 *
 *  \sa SDL_RectEmpty
 *  \sa SDL_RectEquals
 *  \sa SDL_HasIntersection
 *  \sa SDL_IntersectRect
 *  \sa SDL_UnionRect
 *  \sa SDL_EnclosePoints
 */
typedef struct SDL_Rect
{
    int x, y;
    int w, h;
} SDL_Rect;

/**
 *  \brief Returns true if the rectangle has no area.
 */
SDL_FORCE_INLINE SDL_bool SDL_RectEmpty(const SDL_Rect *r)
{
    return ((!r) || (r->w <= 0) || (r->h <= 0)) ? SDL_TRUE : SDL_FALSE;
}

/**
 *  \brief Returns true if the two rectangles are equal.
 */
SDL_FORCE_INLINE SDL_bool SDL_RectEquals(const SDL_Rect *a, const SDL_Rect *b)
{
    return (a && b && (a->x == b->x) && (a->y == b->y) &&
            (a->w == b->w) && (a->h == b->h)) ? SDL_TRUE : SDL_FALSE;
}

/**
 *  \brief Determine whether two rectangles intersect.
 *
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasIntersection(const SDL_Rect * A,
                                                     const SDL_Rect * B);

/**
 *  \brief Calculate the intersection of two rectangles.
 *
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectRect(const SDL_Rect * A,
                                                   const SDL_Rect * B,
                                                   SDL_Rect * result);

/**
 *  \brief Calculate the union of two rectangles.
 */
extern DECLSPEC void SDLCALL SDL_UnionRect(const SDL_Rect * A,
                                           const SDL_Rect * B,
                                           SDL_Rect * result);

/**
 *  \brief Calculate a minimal rectangle enclosing a set of points
 *
 *  \return SDL_TRUE if any points were within the clipping rect
 */
extern DECLSPEC SDL_bool SDLCALL SDL_EnclosePoints(const SDL_Point * points,
                                                   int count,
                                                   const SDL_Rect * clip,
                                                   SDL_Rect * result);

/**
 *  \brief Calculate the intersection of a rectangle and line segment.
 *
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectRectAndLine(const SDL_Rect *
                                                          rect, int *X1,
                                                          int *Y1, int *X2,
                                                          int *Y2);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_rect_h */

/* vi: set ts=4 sw=4 expandtab: */
