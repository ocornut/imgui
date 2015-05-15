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
 *  \file SDL_version.h
 *
 *  This header defines the current SDL version.
 */

#ifndef _SDL_version_h
#define _SDL_version_h

#include "SDL_stdinc.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief Information the version of SDL in use.
 *
 *  Represents the library's version as three levels: major revision
 *  (increments with massive changes, additions, and enhancements),
 *  minor revision (increments with backwards-compatible changes to the
 *  major revision), and patchlevel (increments with fixes to the minor
 *  revision).
 *
 *  \sa SDL_VERSION
 *  \sa SDL_GetVersion
 */
typedef struct SDL_version
{
    Uint8 major;        /**< major version */
    Uint8 minor;        /**< minor version */
    Uint8 patch;        /**< update version */
} SDL_version;

/* Printable format: "%d.%d.%d", MAJOR, MINOR, PATCHLEVEL
*/
#define SDL_MAJOR_VERSION   2
#define SDL_MINOR_VERSION   0
#define SDL_PATCHLEVEL      3

/**
 *  \brief Macro to determine SDL version program was compiled against.
 *
 *  This macro fills in a SDL_version structure with the version of the
 *  library you compiled against. This is determined by what header the
 *  compiler uses. Note that if you dynamically linked the library, you might
 *  have a slightly newer or older version at runtime. That version can be
 *  determined with SDL_GetVersion(), which, unlike SDL_VERSION(),
 *  is not a macro.
 *
 *  \param x A pointer to a SDL_version struct to initialize.
 *
 *  \sa SDL_version
 *  \sa SDL_GetVersion
 */
#define SDL_VERSION(x)                          \
{                                   \
    (x)->major = SDL_MAJOR_VERSION;                 \
    (x)->minor = SDL_MINOR_VERSION;                 \
    (x)->patch = SDL_PATCHLEVEL;                    \
}

/**
 *  This macro turns the version numbers into a numeric value:
 *  \verbatim
    (1,2,3) -> (1203)
    \endverbatim
 *
 *  This assumes that there will never be more than 100 patchlevels.
 */
#define SDL_VERSIONNUM(X, Y, Z)                     \
    ((X)*1000 + (Y)*100 + (Z))

/**
 *  This is the version number macro for the current SDL version.
 */
#define SDL_COMPILEDVERSION \
    SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL)

/**
 *  This macro will evaluate to true if compiled with SDL at least X.Y.Z.
 */
#define SDL_VERSION_ATLEAST(X, Y, Z) \
    (SDL_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))

/**
 *  \brief Get the version of SDL that is linked against your program.
 *
 *  If you are linking to SDL dynamically, then it is possible that the
 *  current version will be different than the version you compiled against.
 *  This function returns the current version, while SDL_VERSION() is a
 *  macro that tells you what version you compiled with.
 *
 *  \code
 *  SDL_version compiled;
 *  SDL_version linked;
 *
 *  SDL_VERSION(&compiled);
 *  SDL_GetVersion(&linked);
 *  printf("We compiled against SDL version %d.%d.%d ...\n",
 *         compiled.major, compiled.minor, compiled.patch);
 *  printf("But we linked against SDL version %d.%d.%d.\n",
 *         linked.major, linked.minor, linked.patch);
 *  \endcode
 *
 *  This function may be called safely at any time, even before SDL_Init().
 *
 *  \sa SDL_VERSION
 */
extern DECLSPEC void SDLCALL SDL_GetVersion(SDL_version * ver);

/**
 *  \brief Get the code revision of SDL that is linked against your program.
 *
 *  Returns an arbitrary string (a hash value) uniquely identifying the
 *  exact revision of the SDL library in use, and is only useful in comparing
 *  against other revisions. It is NOT an incrementing number.
 */
extern DECLSPEC const char *SDLCALL SDL_GetRevision(void);

/**
 *  \brief Get the revision number of SDL that is linked against your program.
 *
 *  Returns a number uniquely identifying the exact revision of the SDL
 *  library in use. It is an incrementing number based on commits to
 *  hg.libsdl.org.
 */
extern DECLSPEC int SDLCALL SDL_GetRevisionNumber(void);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_version_h */

/* vi: set ts=4 sw=4 expandtab: */
