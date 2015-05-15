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
 *  \file SDL_keyboard.h
 *
 *  Include file for SDL keyboard event handling
 */

#ifndef _SDL_keyboard_h
#define _SDL_keyboard_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_keycode.h"
#include "SDL_video.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief The SDL keysym structure, used in key events.
 *
 *  \note  If you are looking for translated character input, see the ::SDL_TEXTINPUT event.
 */
typedef struct SDL_Keysym
{
    SDL_Scancode scancode;      /**< SDL physical key code - see ::SDL_Scancode for details */
    SDL_Keycode sym;            /**< SDL virtual key code - see ::SDL_Keycode for details */
    Uint16 mod;                 /**< current key modifiers */
    Uint32 unused;
} SDL_Keysym;

/* Function prototypes */

/**
 *  \brief Get the window which currently has keyboard focus.
 */
extern DECLSPEC SDL_Window * SDLCALL SDL_GetKeyboardFocus(void);

/**
 *  \brief Get a snapshot of the current state of the keyboard.
 *
 *  \param numkeys if non-NULL, receives the length of the returned array.
 *
 *  \return An array of key states. Indexes into this array are obtained by using ::SDL_Scancode values.
 *
 *  \b Example:
 *  \code
 *  const Uint8 *state = SDL_GetKeyboardState(NULL);
 *  if ( state[SDL_SCANCODE_RETURN] )   {
 *      printf("<RETURN> is pressed.\n");
 *  }
 *  \endcode
 */
extern DECLSPEC const Uint8 *SDLCALL SDL_GetKeyboardState(int *numkeys);

/**
 *  \brief Get the current key modifier state for the keyboard.
 */
extern DECLSPEC SDL_Keymod SDLCALL SDL_GetModState(void);

/**
 *  \brief Set the current key modifier state for the keyboard.
 *
 *  \note This does not change the keyboard state, only the key modifier flags.
 */
extern DECLSPEC void SDLCALL SDL_SetModState(SDL_Keymod modstate);

/**
 *  \brief Get the key code corresponding to the given scancode according
 *         to the current keyboard layout.
 *
 *  See ::SDL_Keycode for details.
 *
 *  \sa SDL_GetKeyName()
 */
extern DECLSPEC SDL_Keycode SDLCALL SDL_GetKeyFromScancode(SDL_Scancode scancode);

/**
 *  \brief Get the scancode corresponding to the given key code according to the
 *         current keyboard layout.
 *
 *  See ::SDL_Scancode for details.
 *
 *  \sa SDL_GetScancodeName()
 */
extern DECLSPEC SDL_Scancode SDLCALL SDL_GetScancodeFromKey(SDL_Keycode key);

/**
 *  \brief Get a human-readable name for a scancode.
 *
 *  \return A pointer to the name for the scancode.
 *          If the scancode doesn't have a name, this function returns
 *          an empty string ("").
 *
 *  \sa SDL_Scancode
 */
extern DECLSPEC const char *SDLCALL SDL_GetScancodeName(SDL_Scancode scancode);

/**
 *  \brief Get a scancode from a human-readable name
 *
 *  \return scancode, or SDL_SCANCODE_UNKNOWN if the name wasn't recognized
 *
 *  \sa SDL_Scancode
 */
extern DECLSPEC SDL_Scancode SDLCALL SDL_GetScancodeFromName(const char *name);

/**
 *  \brief Get a human-readable name for a key.
 *
 *  \return A pointer to a UTF-8 string that stays valid at least until the next
 *          call to this function. If you need it around any longer, you must
 *          copy it.  If the key doesn't have a name, this function returns an
 *          empty string ("").
 *
 *  \sa SDL_Key
 */
extern DECLSPEC const char *SDLCALL SDL_GetKeyName(SDL_Keycode key);

/**
 *  \brief Get a key code from a human-readable name
 *
 *  \return key code, or SDLK_UNKNOWN if the name wasn't recognized
 *
 *  \sa SDL_Keycode
 */
extern DECLSPEC SDL_Keycode SDLCALL SDL_GetKeyFromName(const char *name);

/**
 *  \brief Start accepting Unicode text input events.
 *         This function will show the on-screen keyboard if supported.
 *
 *  \sa SDL_StopTextInput()
 *  \sa SDL_SetTextInputRect()
 *  \sa SDL_HasScreenKeyboardSupport()
 */
extern DECLSPEC void SDLCALL SDL_StartTextInput(void);

/**
 *  \brief Return whether or not Unicode text input events are enabled.
 *
 *  \sa SDL_StartTextInput()
 *  \sa SDL_StopTextInput()
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IsTextInputActive(void);

/**
 *  \brief Stop receiving any text input events.
 *         This function will hide the on-screen keyboard if supported.
 *
 *  \sa SDL_StartTextInput()
 *  \sa SDL_HasScreenKeyboardSupport()
 */
extern DECLSPEC void SDLCALL SDL_StopTextInput(void);

/**
 *  \brief Set the rectangle used to type Unicode text inputs.
 *         This is used as a hint for IME and on-screen keyboard placement.
 *
 *  \sa SDL_StartTextInput()
 */
extern DECLSPEC void SDLCALL SDL_SetTextInputRect(SDL_Rect *rect);

/**
 *  \brief Returns whether the platform has some screen keyboard support.
 *
 *  \return SDL_TRUE if some keyboard support is available else SDL_FALSE.
 *
 *  \note Not all screen keyboard functions are supported on all platforms.
 *
 *  \sa SDL_IsScreenKeyboardShown()
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasScreenKeyboardSupport(void);

/**
 *  \brief Returns whether the screen keyboard is shown for given window.
 *
 *  \param window The window for which screen keyboard should be queried.
 *
 *  \return SDL_TRUE if screen keyboard is shown else SDL_FALSE.
 *
 *  \sa SDL_HasScreenKeyboardSupport()
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IsScreenKeyboardShown(SDL_Window *window);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_keyboard_h */

/* vi: set ts=4 sw=4 expandtab: */
