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
 *  \file SDL_syswm.h
 *
 *  Include file for SDL custom system window manager hooks.
 */

#ifndef _SDL_syswm_h
#define _SDL_syswm_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_version.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \file SDL_syswm.h
 *
 *  Your application has access to a special type of event ::SDL_SYSWMEVENT,
 *  which contains window-manager specific information and arrives whenever
 *  an unhandled window event occurs.  This event is ignored by default, but
 *  you can enable it with SDL_EventState().
 */
#ifdef SDL_PROTOTYPES_ONLY
struct SDL_SysWMinfo;
#else

#if defined(SDL_VIDEO_DRIVER_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined(SDL_VIDEO_DRIVER_WINRT)
#include <Inspectable.h>
#endif

/* This is the structure for custom window manager events */
#if defined(SDL_VIDEO_DRIVER_X11)
#if defined(__APPLE__) && defined(__MACH__)
/* conflicts with Quickdraw.h */
#define Cursor X11Cursor
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#if defined(__APPLE__) && defined(__MACH__)
/* matches the re-define above */
#undef Cursor
#endif

#endif /* defined(SDL_VIDEO_DRIVER_X11) */

#if defined(SDL_VIDEO_DRIVER_DIRECTFB)
#include <directfb.h>
#endif

#if defined(SDL_VIDEO_DRIVER_COCOA)
#ifdef __OBJC__
#include <Cocoa/Cocoa.h>
#else
typedef struct _NSWindow NSWindow;
#endif
#endif

#if defined(SDL_VIDEO_DRIVER_UIKIT)
#ifdef __OBJC__
#include <UIKit/UIKit.h>
#else
typedef struct _UIWindow UIWindow;
typedef struct _UIViewController UIViewController;
#endif
#endif

#if defined(SDL_VIDEO_DRIVER_MIR)
#include <mir_toolkit/mir_client_library.h>
#endif


/**
 *  These are the various supported windowing subsystems
 */
typedef enum
{
    SDL_SYSWM_UNKNOWN,
    SDL_SYSWM_WINDOWS,
    SDL_SYSWM_X11,
    SDL_SYSWM_DIRECTFB,
    SDL_SYSWM_COCOA,
    SDL_SYSWM_UIKIT,
    SDL_SYSWM_WAYLAND,
    SDL_SYSWM_MIR,
    SDL_SYSWM_WINRT,
} SDL_SYSWM_TYPE;

/**
 *  The custom event structure.
 */
struct SDL_SysWMmsg
{
    SDL_version version;
    SDL_SYSWM_TYPE subsystem;
    union
    {
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
        struct {
            HWND hwnd;                  /**< The window for the message */
            UINT msg;                   /**< The type of message */
            WPARAM wParam;              /**< WORD message parameter */
            LPARAM lParam;              /**< LONG message parameter */
        } win;
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
        struct {
            XEvent event;
        } x11;
#endif
#if defined(SDL_VIDEO_DRIVER_DIRECTFB)
        struct {
            DFBEvent event;
        } dfb;
#endif
#if defined(SDL_VIDEO_DRIVER_COCOA)
        struct
        {
            /* No Cocoa window events yet */
        } cocoa;
#endif
#if defined(SDL_VIDEO_DRIVER_UIKIT)
        struct
        {
            /* No UIKit window events yet */
        } uikit;
#endif
        /* Can't have an empty union */
        int dummy;
    } msg;
};

/**
 *  The custom window manager information structure.
 *
 *  When this structure is returned, it holds information about which
 *  low level system it is using, and will be one of SDL_SYSWM_TYPE.
 */
struct SDL_SysWMinfo
{
    SDL_version version;
    SDL_SYSWM_TYPE subsystem;
    union
    {
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
        struct
        {
            HWND window;                /**< The window handle */
        } win;
#endif
#if defined(SDL_VIDEO_DRIVER_WINRT)
        struct
        {
            IInspectable * window;      /**< The WinRT CoreWindow */
        } winrt;
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
        struct
        {
            Display *display;           /**< The X11 display */
            Window window;              /**< The X11 window */
        } x11;
#endif
#if defined(SDL_VIDEO_DRIVER_DIRECTFB)
        struct
        {
            IDirectFB *dfb;             /**< The directfb main interface */
            IDirectFBWindow *window;    /**< The directfb window handle */
            IDirectFBSurface *surface;  /**< The directfb client surface */
        } dfb;
#endif
#if defined(SDL_VIDEO_DRIVER_COCOA)
        struct
        {
            NSWindow *window;           /* The Cocoa window */
        } cocoa;
#endif
#if defined(SDL_VIDEO_DRIVER_UIKIT)
        struct
        {
            UIWindow *window;           /* The UIKit window */
        } uikit;
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
        struct
        {
            struct wl_display *display;            /**< Wayland display */
            struct wl_surface *surface;            /**< Wayland surface */
            struct wl_shell_surface *shell_surface; /**< Wayland shell_surface (window manager handle) */
        } wl;
#endif
#if defined(SDL_VIDEO_DRIVER_MIR)
        struct
        {
            MirConnection *connection;  /**< Mir display server connection */
            MirSurface *surface;  /**< Mir surface */
        } mir;
#endif

        /* Can't have an empty union */
        int dummy;
    } info;
};

#endif /* SDL_PROTOTYPES_ONLY */

typedef struct SDL_SysWMinfo SDL_SysWMinfo;

/* Function prototypes */
/**
 *  \brief This function allows access to driver-dependent window information.
 *
 *  \param window The window about which information is being requested
 *  \param info This structure must be initialized with the SDL version, and is
 *              then filled in with information about the given window.
 *
 *  \return SDL_TRUE if the function is implemented and the version member of
 *          the \c info struct is valid, SDL_FALSE otherwise.
 *
 *  You typically use this function like this:
 *  \code
 *  SDL_SysWMinfo info;
 *  SDL_VERSION(&info.version);
 *  if ( SDL_GetWindowWMInfo(window, &info) ) { ... }
 *  \endcode
 */
extern DECLSPEC SDL_bool SDLCALL SDL_GetWindowWMInfo(SDL_Window * window,
                                                     SDL_SysWMinfo * info);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_syswm_h */

/* vi: set ts=4 sw=4 expandtab: */
