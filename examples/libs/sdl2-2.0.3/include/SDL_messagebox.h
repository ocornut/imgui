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

#ifndef _SDL_messagebox_h
#define _SDL_messagebox_h

#include "SDL_stdinc.h"
#include "SDL_video.h"      /* For SDL_Window */

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief SDL_MessageBox flags. If supported will display warning icon, etc.
 */
typedef enum
{
    SDL_MESSAGEBOX_ERROR        = 0x00000010,   /**< error dialog */
    SDL_MESSAGEBOX_WARNING      = 0x00000020,   /**< warning dialog */
    SDL_MESSAGEBOX_INFORMATION  = 0x00000040    /**< informational dialog */
} SDL_MessageBoxFlags;

/**
 * \brief Flags for SDL_MessageBoxButtonData.
 */
typedef enum
{
    SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT = 0x00000001,  /**< Marks the default button when return is hit */
    SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT = 0x00000002   /**< Marks the default button when escape is hit */
} SDL_MessageBoxButtonFlags;

/**
 *  \brief Individual button data.
 */
typedef struct
{
    Uint32 flags;       /**< ::SDL_MessageBoxButtonFlags */
    int buttonid;       /**< User defined button id (value returned via SDL_ShowMessageBox) */
    const char * text;  /**< The UTF-8 button text */
} SDL_MessageBoxButtonData;

/**
 * \brief RGB value used in a message box color scheme
 */
typedef struct
{
    Uint8 r, g, b;
} SDL_MessageBoxColor;

typedef enum
{
    SDL_MESSAGEBOX_COLOR_BACKGROUND,
    SDL_MESSAGEBOX_COLOR_TEXT,
    SDL_MESSAGEBOX_COLOR_BUTTON_BORDER,
    SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND,
    SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED,
    SDL_MESSAGEBOX_COLOR_MAX
} SDL_MessageBoxColorType;

/**
 * \brief A set of colors to use for message box dialogs
 */
typedef struct
{
    SDL_MessageBoxColor colors[SDL_MESSAGEBOX_COLOR_MAX];
} SDL_MessageBoxColorScheme;

/**
 *  \brief MessageBox structure containing title, text, window, etc.
 */
typedef struct
{
    Uint32 flags;                       /**< ::SDL_MessageBoxFlags */
    SDL_Window *window;                 /**< Parent window, can be NULL */
    const char *title;                  /**< UTF-8 title */
    const char *message;                /**< UTF-8 message text */

    int numbuttons;
    const SDL_MessageBoxButtonData *buttons;

    const SDL_MessageBoxColorScheme *colorScheme;   /**< ::SDL_MessageBoxColorScheme, can be NULL to use system settings */
} SDL_MessageBoxData;

/**
 *  \brief Create a modal message box.
 *
 *  \param messageboxdata The SDL_MessageBoxData structure with title, text, etc.
 *  \param buttonid The pointer to which user id of hit button should be copied.
 *
 *  \return -1 on error, otherwise 0 and buttonid contains user id of button
 *          hit or -1 if dialog was closed.
 *
 *  \note This function should be called on the thread that created the parent
 *        window, or on the main thread if the messagebox has no parent.  It will
 *        block execution of that thread until the user clicks a button or
 *        closes the messagebox.
 */
extern DECLSPEC int SDLCALL SDL_ShowMessageBox(const SDL_MessageBoxData *messageboxdata, int *buttonid);

/**
 *  \brief Create a simple modal message box
 *
 *  \param flags    ::SDL_MessageBoxFlags
 *  \param title    UTF-8 title text
 *  \param message  UTF-8 message text
 *  \param window   The parent window, or NULL for no parent
 *
 *  \return 0 on success, -1 on error
 *
 *  \sa SDL_ShowMessageBox
 */
extern DECLSPEC int SDLCALL SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_messagebox_h */

/* vi: set ts=4 sw=4 expandtab: */
