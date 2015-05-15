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
 *  \file SDL_test_font.h
 *
 *  Include file for SDL test framework.
 *
 *  This code is a part of the SDL2_test library, not the main SDL library.
 */

#ifndef _SDL_test_font_h
#define _SDL_test_font_h

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Function prototypes */

#define FONT_CHARACTER_SIZE  8

/**
 *  \brief Draw a string in the currently set font.
 *
 *  \param renderer The renderer to draw on.
 *  \param x The X coordinate of the upper left corner of the character.
 *  \param y The Y coordinate of the upper left corner of the character.
 *  \param c The character to draw.
 *
 *  \returns Returns 0 on success, -1 on failure.
 */
int SDLTest_DrawCharacter( SDL_Renderer *renderer, int x, int y, char c );

/**
 *  \brief Draw a string in the currently set font.
 *
 *  \param renderer The renderer to draw on.
 *  \param x The X coordinate of the upper left corner of the string.
 *  \param y The Y coordinate of the upper left corner of the string.
 *  \param s The string to draw.
 *
 *  \returns Returns 0 on success, -1 on failure.
 */
int SDLTest_DrawString( SDL_Renderer * renderer, int x, int y, const char *s );


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_test_font_h */

/* vi: set ts=4 sw=4 expandtab: */
