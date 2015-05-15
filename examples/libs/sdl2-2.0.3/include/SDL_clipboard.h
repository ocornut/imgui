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
 * \file SDL_clipboard.h
 *
 * Include file for SDL clipboard handling
 */

#ifndef _SDL_clipboard_h
#define _SDL_clipboard_h

#include "SDL_stdinc.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Function prototypes */

/**
 * \brief Put UTF-8 text into the clipboard
 *
 * \sa SDL_GetClipboardText()
 */
extern DECLSPEC int SDLCALL SDL_SetClipboardText(const char *text);

/**
 * \brief Get UTF-8 text from the clipboard, which must be freed with SDL_free()
 *
 * \sa SDL_SetClipboardText()
 */
extern DECLSPEC char * SDLCALL SDL_GetClipboardText(void);

/**
 * \brief Returns a flag indicating whether the clipboard exists and contains a text string that is non-empty
 *
 * \sa SDL_GetClipboardText()
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasClipboardText(void);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_clipboard_h */

/* vi: set ts=4 sw=4 expandtab: */
