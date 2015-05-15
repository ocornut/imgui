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

#ifndef _SDL_main_h
#define _SDL_main_h

#include "SDL_stdinc.h"

/**
 *  \file SDL_main.h
 *
 *  Redefine main() on some platforms so that it is called by SDL.
 */

#ifndef SDL_MAIN_HANDLED
#if defined(__WIN32__)
/* On Windows SDL provides WinMain(), which parses the command line and passes
   the arguments to your main function.

   If you provide your own WinMain(), you may define SDL_MAIN_HANDLED
 */
#define SDL_MAIN_AVAILABLE

#elif defined(__WINRT__)
/* On WinRT, SDL provides a main function that initializes CoreApplication,
   creating an instance of IFrameworkView in the process.

   Please note that #include'ing SDL_main.h is not enough to get a main()
   function working.  In non-XAML apps, the file,
   src/main/winrt/SDL_WinRT_main_NonXAML.cpp, or a copy of it, must be compiled
   into the app itself.  In XAML apps, the function, SDL_WinRTRunApp must be
   called, with a pointer to the Direct3D-hosted XAML control passed in.
*/
#define SDL_MAIN_NEEDED

#elif defined(__IPHONEOS__)
/* On iOS SDL provides a main function that creates an application delegate
   and starts the iOS application run loop.

   See src/video/uikit/SDL_uikitappdelegate.m for more details.
 */
#define SDL_MAIN_NEEDED

#elif defined(__ANDROID__)
/* On Android SDL provides a Java class in SDLActivity.java that is the
   main activity entry point.

   See README-android.txt for more details on extending that class.
 */
#define SDL_MAIN_NEEDED

#endif
#endif /* SDL_MAIN_HANDLED */

#ifdef __cplusplus
#define C_LINKAGE   "C"
#else
#define C_LINKAGE
#endif /* __cplusplus */

/**
 *  \file SDL_main.h
 *
 *  The application's main() function must be called with C linkage,
 *  and should be declared like this:
 *  \code
 *  #ifdef __cplusplus
 *  extern "C"
 *  #endif
 *  int main(int argc, char *argv[])
 *  {
 *  }
 *  \endcode
 */

#if defined(SDL_MAIN_NEEDED) || defined(SDL_MAIN_AVAILABLE)
#define main    SDL_main
#endif

/**
 *  The prototype for the application's main() function
 */
extern C_LINKAGE int SDL_main(int argc, char *argv[]);


#include "begin_code.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  This is called by the real SDL main function to let the rest of the
 *  library know that initialization was done properly.
 *
 *  Calling this yourself without knowing what you're doing can cause
 *  crashes and hard to diagnose problems with your application.
 */
extern DECLSPEC void SDLCALL SDL_SetMainReady(void);

#ifdef __WIN32__

/**
 *  This can be called to set the application class at startup
 */
extern DECLSPEC int SDLCALL SDL_RegisterApp(char *name, Uint32 style,
                                            void *hInst);
extern DECLSPEC void SDLCALL SDL_UnregisterApp(void);

#endif /* __WIN32__ */


#ifdef __WINRT__

/**
 *  \brief Initializes and launches an SDL/WinRT application.
 *
 *  \param mainFunction The SDL app's C-style main().
 *  \param xamlBackgroundPanel An optional, XAML-based, background panel.
 *     For Non-XAML apps, this value must be set to NULL.  For XAML apps,
 *     pass in a pointer to a SwapChainBackgroundPanel, casted to an
 *     IInspectable (via reinterpret_cast).
 *  \ret 0 on success, -1 on failure.  On failure, use SDL_GetError to retrieve more
 *      information on the failure.
 */
extern DECLSPEC int SDLCALL SDL_WinRTRunApp(int (*mainFunction)(int, char **), void * xamlBackgroundPanel);

#endif /* __WINRT__ */


#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_main_h */

/* vi: set ts=4 sw=4 expandtab: */
