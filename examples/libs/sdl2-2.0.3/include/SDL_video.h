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
 *  \file SDL_video.h
 *
 *  Header file for SDL video functions.
 */

#ifndef _SDL_video_h
#define _SDL_video_h

#include "SDL_stdinc.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_surface.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief  The structure that defines a display mode
 *
 *  \sa SDL_GetNumDisplayModes()
 *  \sa SDL_GetDisplayMode()
 *  \sa SDL_GetDesktopDisplayMode()
 *  \sa SDL_GetCurrentDisplayMode()
 *  \sa SDL_GetClosestDisplayMode()
 *  \sa SDL_SetWindowDisplayMode()
 *  \sa SDL_GetWindowDisplayMode()
 */
typedef struct
{
    Uint32 format;              /**< pixel format */
    int w;                      /**< width */
    int h;                      /**< height */
    int refresh_rate;           /**< refresh rate (or zero for unspecified) */
    void *driverdata;           /**< driver-specific data, initialize to 0 */
} SDL_DisplayMode;

/**
 *  \brief The type used to identify a window
 *
 *  \sa SDL_CreateWindow()
 *  \sa SDL_CreateWindowFrom()
 *  \sa SDL_DestroyWindow()
 *  \sa SDL_GetWindowData()
 *  \sa SDL_GetWindowFlags()
 *  \sa SDL_GetWindowGrab()
 *  \sa SDL_GetWindowPosition()
 *  \sa SDL_GetWindowSize()
 *  \sa SDL_GetWindowTitle()
 *  \sa SDL_HideWindow()
 *  \sa SDL_MaximizeWindow()
 *  \sa SDL_MinimizeWindow()
 *  \sa SDL_RaiseWindow()
 *  \sa SDL_RestoreWindow()
 *  \sa SDL_SetWindowData()
 *  \sa SDL_SetWindowFullscreen()
 *  \sa SDL_SetWindowGrab()
 *  \sa SDL_SetWindowIcon()
 *  \sa SDL_SetWindowPosition()
 *  \sa SDL_SetWindowSize()
 *  \sa SDL_SetWindowBordered()
 *  \sa SDL_SetWindowTitle()
 *  \sa SDL_ShowWindow()
 */
typedef struct SDL_Window SDL_Window;

/**
 *  \brief The flags on a window
 *
 *  \sa SDL_GetWindowFlags()
 */
typedef enum
{
    SDL_WINDOW_FULLSCREEN = 0x00000001,         /**< fullscreen window */
    SDL_WINDOW_OPENGL = 0x00000002,             /**< window usable with OpenGL context */
    SDL_WINDOW_SHOWN = 0x00000004,              /**< window is visible */
    SDL_WINDOW_HIDDEN = 0x00000008,             /**< window is not visible */
    SDL_WINDOW_BORDERLESS = 0x00000010,         /**< no window decoration */
    SDL_WINDOW_RESIZABLE = 0x00000020,          /**< window can be resized */
    SDL_WINDOW_MINIMIZED = 0x00000040,          /**< window is minimized */
    SDL_WINDOW_MAXIMIZED = 0x00000080,          /**< window is maximized */
    SDL_WINDOW_INPUT_GRABBED = 0x00000100,      /**< window has grabbed input focus */
    SDL_WINDOW_INPUT_FOCUS = 0x00000200,        /**< window has input focus */
    SDL_WINDOW_MOUSE_FOCUS = 0x00000400,        /**< window has mouse focus */
    SDL_WINDOW_FULLSCREEN_DESKTOP = ( SDL_WINDOW_FULLSCREEN | 0x00001000 ),
    SDL_WINDOW_FOREIGN = 0x00000800,            /**< window not created by SDL */
    SDL_WINDOW_ALLOW_HIGHDPI = 0x00002000       /**< window should be created in high-DPI mode if supported */
} SDL_WindowFlags;

/**
 *  \brief Used to indicate that you don't care what the window position is.
 */
#define SDL_WINDOWPOS_UNDEFINED_MASK    0x1FFF0000
#define SDL_WINDOWPOS_UNDEFINED_DISPLAY(X)  (SDL_WINDOWPOS_UNDEFINED_MASK|(X))
#define SDL_WINDOWPOS_UNDEFINED         SDL_WINDOWPOS_UNDEFINED_DISPLAY(0)
#define SDL_WINDOWPOS_ISUNDEFINED(X)    \
            (((X)&0xFFFF0000) == SDL_WINDOWPOS_UNDEFINED_MASK)

/**
 *  \brief Used to indicate that the window position should be centered.
 */
#define SDL_WINDOWPOS_CENTERED_MASK    0x2FFF0000
#define SDL_WINDOWPOS_CENTERED_DISPLAY(X)  (SDL_WINDOWPOS_CENTERED_MASK|(X))
#define SDL_WINDOWPOS_CENTERED         SDL_WINDOWPOS_CENTERED_DISPLAY(0)
#define SDL_WINDOWPOS_ISCENTERED(X)    \
            (((X)&0xFFFF0000) == SDL_WINDOWPOS_CENTERED_MASK)

/**
 *  \brief Event subtype for window events
 */
typedef enum
{
    SDL_WINDOWEVENT_NONE,           /**< Never used */
    SDL_WINDOWEVENT_SHOWN,          /**< Window has been shown */
    SDL_WINDOWEVENT_HIDDEN,         /**< Window has been hidden */
    SDL_WINDOWEVENT_EXPOSED,        /**< Window has been exposed and should be
                                         redrawn */
    SDL_WINDOWEVENT_MOVED,          /**< Window has been moved to data1, data2
                                     */
    SDL_WINDOWEVENT_RESIZED,        /**< Window has been resized to data1xdata2 */
    SDL_WINDOWEVENT_SIZE_CHANGED,   /**< The window size has changed, either as a result of an API call or through the system or user changing the window size. */
    SDL_WINDOWEVENT_MINIMIZED,      /**< Window has been minimized */
    SDL_WINDOWEVENT_MAXIMIZED,      /**< Window has been maximized */
    SDL_WINDOWEVENT_RESTORED,       /**< Window has been restored to normal size
                                         and position */
    SDL_WINDOWEVENT_ENTER,          /**< Window has gained mouse focus */
    SDL_WINDOWEVENT_LEAVE,          /**< Window has lost mouse focus */
    SDL_WINDOWEVENT_FOCUS_GAINED,   /**< Window has gained keyboard focus */
    SDL_WINDOWEVENT_FOCUS_LOST,     /**< Window has lost keyboard focus */
    SDL_WINDOWEVENT_CLOSE           /**< The window manager requests that the
                                         window be closed */
} SDL_WindowEventID;

/**
 *  \brief An opaque handle to an OpenGL context.
 */
typedef void *SDL_GLContext;

/**
 *  \brief OpenGL configuration attributes
 */
typedef enum
{
    SDL_GL_RED_SIZE,
    SDL_GL_GREEN_SIZE,
    SDL_GL_BLUE_SIZE,
    SDL_GL_ALPHA_SIZE,
    SDL_GL_BUFFER_SIZE,
    SDL_GL_DOUBLEBUFFER,
    SDL_GL_DEPTH_SIZE,
    SDL_GL_STENCIL_SIZE,
    SDL_GL_ACCUM_RED_SIZE,
    SDL_GL_ACCUM_GREEN_SIZE,
    SDL_GL_ACCUM_BLUE_SIZE,
    SDL_GL_ACCUM_ALPHA_SIZE,
    SDL_GL_STEREO,
    SDL_GL_MULTISAMPLEBUFFERS,
    SDL_GL_MULTISAMPLESAMPLES,
    SDL_GL_ACCELERATED_VISUAL,
    SDL_GL_RETAINED_BACKING,
    SDL_GL_CONTEXT_MAJOR_VERSION,
    SDL_GL_CONTEXT_MINOR_VERSION,
    SDL_GL_CONTEXT_EGL,
    SDL_GL_CONTEXT_FLAGS,
    SDL_GL_CONTEXT_PROFILE_MASK,
    SDL_GL_SHARE_WITH_CURRENT_CONTEXT,
    SDL_GL_FRAMEBUFFER_SRGB_CAPABLE
} SDL_GLattr;

typedef enum
{
    SDL_GL_CONTEXT_PROFILE_CORE           = 0x0001,
    SDL_GL_CONTEXT_PROFILE_COMPATIBILITY  = 0x0002,
    SDL_GL_CONTEXT_PROFILE_ES             = 0x0004 /* GLX_CONTEXT_ES2_PROFILE_BIT_EXT */
} SDL_GLprofile;

typedef enum
{
    SDL_GL_CONTEXT_DEBUG_FLAG              = 0x0001,
    SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG = 0x0002,
    SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG      = 0x0004,
    SDL_GL_CONTEXT_RESET_ISOLATION_FLAG    = 0x0008
} SDL_GLcontextFlag;


/* Function prototypes */

/**
 *  \brief Get the number of video drivers compiled into SDL
 *
 *  \sa SDL_GetVideoDriver()
 */
extern DECLSPEC int SDLCALL SDL_GetNumVideoDrivers(void);

/**
 *  \brief Get the name of a built in video driver.
 *
 *  \note The video drivers are presented in the order in which they are
 *        normally checked during initialization.
 *
 *  \sa SDL_GetNumVideoDrivers()
 */
extern DECLSPEC const char *SDLCALL SDL_GetVideoDriver(int index);

/**
 *  \brief Initialize the video subsystem, optionally specifying a video driver.
 *
 *  \param driver_name Initialize a specific driver by name, or NULL for the
 *                     default video driver.
 *
 *  \return 0 on success, -1 on error
 *
 *  This function initializes the video subsystem; setting up a connection
 *  to the window manager, etc, and determines the available display modes
 *  and pixel formats, but does not initialize a window or graphics mode.
 *
 *  \sa SDL_VideoQuit()
 */
extern DECLSPEC int SDLCALL SDL_VideoInit(const char *driver_name);

/**
 *  \brief Shuts down the video subsystem.
 *
 *  This function closes all windows, and restores the original video mode.
 *
 *  \sa SDL_VideoInit()
 */
extern DECLSPEC void SDLCALL SDL_VideoQuit(void);

/**
 *  \brief Returns the name of the currently initialized video driver.
 *
 *  \return The name of the current video driver or NULL if no driver
 *          has been initialized
 *
 *  \sa SDL_GetNumVideoDrivers()
 *  \sa SDL_GetVideoDriver()
 */
extern DECLSPEC const char *SDLCALL SDL_GetCurrentVideoDriver(void);

/**
 *  \brief Returns the number of available video displays.
 *
 *  \sa SDL_GetDisplayBounds()
 */
extern DECLSPEC int SDLCALL SDL_GetNumVideoDisplays(void);

/**
 *  \brief Get the name of a display in UTF-8 encoding
 *
 *  \return The name of a display, or NULL for an invalid display index.
 *
 *  \sa SDL_GetNumVideoDisplays()
 */
extern DECLSPEC const char * SDLCALL SDL_GetDisplayName(int displayIndex);

/**
 *  \brief Get the desktop area represented by a display, with the primary
 *         display located at 0,0
 *
 *  \return 0 on success, or -1 if the index is out of range.
 *
 *  \sa SDL_GetNumVideoDisplays()
 */
extern DECLSPEC int SDLCALL SDL_GetDisplayBounds(int displayIndex, SDL_Rect * rect);

/**
 *  \brief Returns the number of available display modes.
 *
 *  \sa SDL_GetDisplayMode()
 */
extern DECLSPEC int SDLCALL SDL_GetNumDisplayModes(int displayIndex);

/**
 *  \brief Fill in information about a specific display mode.
 *
 *  \note The display modes are sorted in this priority:
 *        \li bits per pixel -> more colors to fewer colors
 *        \li width -> largest to smallest
 *        \li height -> largest to smallest
 *        \li refresh rate -> highest to lowest
 *
 *  \sa SDL_GetNumDisplayModes()
 */
extern DECLSPEC int SDLCALL SDL_GetDisplayMode(int displayIndex, int modeIndex,
                                               SDL_DisplayMode * mode);

/**
 *  \brief Fill in information about the desktop display mode.
 */
extern DECLSPEC int SDLCALL SDL_GetDesktopDisplayMode(int displayIndex, SDL_DisplayMode * mode);

/**
 *  \brief Fill in information about the current display mode.
 */
extern DECLSPEC int SDLCALL SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode * mode);


/**
 *  \brief Get the closest match to the requested display mode.
 *
 *  \param displayIndex The index of display from which mode should be queried.
 *  \param mode The desired display mode
 *  \param closest A pointer to a display mode to be filled in with the closest
 *                 match of the available display modes.
 *
 *  \return The passed in value \c closest, or NULL if no matching video mode
 *          was available.
 *
 *  The available display modes are scanned, and \c closest is filled in with the
 *  closest mode matching the requested mode and returned.  The mode format and
 *  refresh_rate default to the desktop mode if they are 0.  The modes are
 *  scanned with size being first priority, format being second priority, and
 *  finally checking the refresh_rate.  If all the available modes are too
 *  small, then NULL is returned.
 *
 *  \sa SDL_GetNumDisplayModes()
 *  \sa SDL_GetDisplayMode()
 */
extern DECLSPEC SDL_DisplayMode * SDLCALL SDL_GetClosestDisplayMode(int displayIndex, const SDL_DisplayMode * mode, SDL_DisplayMode * closest);

/**
 *  \brief Get the display index associated with a window.
 *
 *  \return the display index of the display containing the center of the
 *          window, or -1 on error.
 */
extern DECLSPEC int SDLCALL SDL_GetWindowDisplayIndex(SDL_Window * window);

/**
 *  \brief Set the display mode used when a fullscreen window is visible.
 *
 *  By default the window's dimensions and the desktop format and refresh rate
 *  are used.
 *
 *  \param window The window for which the display mode should be set.
 *  \param mode The mode to use, or NULL for the default mode.
 *
 *  \return 0 on success, or -1 if setting the display mode failed.
 *
 *  \sa SDL_GetWindowDisplayMode()
 *  \sa SDL_SetWindowFullscreen()
 */
extern DECLSPEC int SDLCALL SDL_SetWindowDisplayMode(SDL_Window * window,
                                                     const SDL_DisplayMode
                                                         * mode);

/**
 *  \brief Fill in information about the display mode used when a fullscreen
 *         window is visible.
 *
 *  \sa SDL_SetWindowDisplayMode()
 *  \sa SDL_SetWindowFullscreen()
 */
extern DECLSPEC int SDLCALL SDL_GetWindowDisplayMode(SDL_Window * window,
                                                     SDL_DisplayMode * mode);

/**
 *  \brief Get the pixel format associated with the window.
 */
extern DECLSPEC Uint32 SDLCALL SDL_GetWindowPixelFormat(SDL_Window * window);

/**
 *  \brief Create a window with the specified position, dimensions, and flags.
 *
 *  \param title The title of the window, in UTF-8 encoding.
 *  \param x     The x position of the window, ::SDL_WINDOWPOS_CENTERED, or
 *               ::SDL_WINDOWPOS_UNDEFINED.
 *  \param y     The y position of the window, ::SDL_WINDOWPOS_CENTERED, or
 *               ::SDL_WINDOWPOS_UNDEFINED.
 *  \param w     The width of the window.
 *  \param h     The height of the window.
 *  \param flags The flags for the window, a mask of any of the following:
 *               ::SDL_WINDOW_FULLSCREEN,    ::SDL_WINDOW_OPENGL,
 *               ::SDL_WINDOW_HIDDEN,        ::SDL_WINDOW_BORDERLESS,
 *               ::SDL_WINDOW_RESIZABLE,     ::SDL_WINDOW_MAXIMIZED,
 *               ::SDL_WINDOW_MINIMIZED,     ::SDL_WINDOW_INPUT_GRABBED,
 *               ::SDL_WINDOW_ALLOW_HIGHDPI.
 *
 *  \return The id of the window created, or zero if window creation failed.
 *
 *  \sa SDL_DestroyWindow()
 */
extern DECLSPEC SDL_Window * SDLCALL SDL_CreateWindow(const char *title,
                                                      int x, int y, int w,
                                                      int h, Uint32 flags);

/**
 *  \brief Create an SDL window from an existing native window.
 *
 *  \param data A pointer to driver-dependent window creation data
 *
 *  \return The id of the window created, or zero if window creation failed.
 *
 *  \sa SDL_DestroyWindow()
 */
extern DECLSPEC SDL_Window * SDLCALL SDL_CreateWindowFrom(const void *data);

/**
 *  \brief Get the numeric ID of a window, for logging purposes.
 */
extern DECLSPEC Uint32 SDLCALL SDL_GetWindowID(SDL_Window * window);

/**
 *  \brief Get a window from a stored ID, or NULL if it doesn't exist.
 */
extern DECLSPEC SDL_Window * SDLCALL SDL_GetWindowFromID(Uint32 id);

/**
 *  \brief Get the window flags.
 */
extern DECLSPEC Uint32 SDLCALL SDL_GetWindowFlags(SDL_Window * window);

/**
 *  \brief Set the title of a window, in UTF-8 format.
 *
 *  \sa SDL_GetWindowTitle()
 */
extern DECLSPEC void SDLCALL SDL_SetWindowTitle(SDL_Window * window,
                                                const char *title);

/**
 *  \brief Get the title of a window, in UTF-8 format.
 *
 *  \sa SDL_SetWindowTitle()
 */
extern DECLSPEC const char *SDLCALL SDL_GetWindowTitle(SDL_Window * window);

/**
 *  \brief Set the icon for a window.
 *
 *  \param window The window for which the icon should be set.
 *  \param icon The icon for the window.
 */
extern DECLSPEC void SDLCALL SDL_SetWindowIcon(SDL_Window * window,
                                               SDL_Surface * icon);

/**
 *  \brief Associate an arbitrary named pointer with a window.
 *
 *  \param window   The window to associate with the pointer.
 *  \param name     The name of the pointer.
 *  \param userdata The associated pointer.
 *
 *  \return The previous value associated with 'name'
 *
 *  \note The name is case-sensitive.
 *
 *  \sa SDL_GetWindowData()
 */
extern DECLSPEC void* SDLCALL SDL_SetWindowData(SDL_Window * window,
                                                const char *name,
                                                void *userdata);

/**
 *  \brief Retrieve the data pointer associated with a window.
 *
 *  \param window   The window to query.
 *  \param name     The name of the pointer.
 *
 *  \return The value associated with 'name'
 *
 *  \sa SDL_SetWindowData()
 */
extern DECLSPEC void *SDLCALL SDL_GetWindowData(SDL_Window * window,
                                                const char *name);

/**
 *  \brief Set the position of a window.
 *
 *  \param window   The window to reposition.
 *  \param x        The x coordinate of the window, ::SDL_WINDOWPOS_CENTERED, or
                    ::SDL_WINDOWPOS_UNDEFINED.
 *  \param y        The y coordinate of the window, ::SDL_WINDOWPOS_CENTERED, or
                    ::SDL_WINDOWPOS_UNDEFINED.
 *
 *  \note The window coordinate origin is the upper left of the display.
 *
 *  \sa SDL_GetWindowPosition()
 */
extern DECLSPEC void SDLCALL SDL_SetWindowPosition(SDL_Window * window,
                                                   int x, int y);

/**
 *  \brief Get the position of a window.
 *
 *  \param window   The window to query.
 *  \param x        Pointer to variable for storing the x position, may be NULL
 *  \param y        Pointer to variable for storing the y position, may be NULL
 *
 *  \sa SDL_SetWindowPosition()
 */
extern DECLSPEC void SDLCALL SDL_GetWindowPosition(SDL_Window * window,
                                                   int *x, int *y);

/**
 *  \brief Set the size of a window's client area.
 *
 *  \param window   The window to resize.
 *  \param w        The width of the window, must be >0
 *  \param h        The height of the window, must be >0
 *
 *  \note You can't change the size of a fullscreen window, it automatically
 *        matches the size of the display mode.
 *
 *  \sa SDL_GetWindowSize()
 */
extern DECLSPEC void SDLCALL SDL_SetWindowSize(SDL_Window * window, int w,
                                               int h);

/**
 *  \brief Get the size of a window's client area.
 *
 *  \param window   The window to query.
 *  \param w        Pointer to variable for storing the width, may be NULL
 *  \param h        Pointer to variable for storing the height, may be NULL
 *
 *  \sa SDL_SetWindowSize()
 */
extern DECLSPEC void SDLCALL SDL_GetWindowSize(SDL_Window * window, int *w,
                                               int *h);

/**
 *  \brief Set the minimum size of a window's client area.
 *
 *  \param window    The window to set a new minimum size.
 *  \param min_w     The minimum width of the window, must be >0
 *  \param min_h     The minimum height of the window, must be >0
 *
 *  \note You can't change the minimum size of a fullscreen window, it
 *        automatically matches the size of the display mode.
 *
 *  \sa SDL_GetWindowMinimumSize()
 *  \sa SDL_SetWindowMaximumSize()
 */
extern DECLSPEC void SDLCALL SDL_SetWindowMinimumSize(SDL_Window * window,
                                                      int min_w, int min_h);

/**
 *  \brief Get the minimum size of a window's client area.
 *
 *  \param window   The window to query.
 *  \param w        Pointer to variable for storing the minimum width, may be NULL
 *  \param h        Pointer to variable for storing the minimum height, may be NULL
 *
 *  \sa SDL_GetWindowMaximumSize()
 *  \sa SDL_SetWindowMinimumSize()
 */
extern DECLSPEC void SDLCALL SDL_GetWindowMinimumSize(SDL_Window * window,
                                                      int *w, int *h);

/**
 *  \brief Set the maximum size of a window's client area.
 *
 *  \param window    The window to set a new maximum size.
 *  \param max_w     The maximum width of the window, must be >0
 *  \param max_h     The maximum height of the window, must be >0
 *
 *  \note You can't change the maximum size of a fullscreen window, it
 *        automatically matches the size of the display mode.
 *
 *  \sa SDL_GetWindowMaximumSize()
 *  \sa SDL_SetWindowMinimumSize()
 */
extern DECLSPEC void SDLCALL SDL_SetWindowMaximumSize(SDL_Window * window,
                                                      int max_w, int max_h);

/**
 *  \brief Get the maximum size of a window's client area.
 *
 *  \param window   The window to query.
 *  \param w        Pointer to variable for storing the maximum width, may be NULL
 *  \param h        Pointer to variable for storing the maximum height, may be NULL
 *
 *  \sa SDL_GetWindowMinimumSize()
 *  \sa SDL_SetWindowMaximumSize()
 */
extern DECLSPEC void SDLCALL SDL_GetWindowMaximumSize(SDL_Window * window,
                                                      int *w, int *h);

/**
 *  \brief Set the border state of a window.
 *
 *  This will add or remove the window's SDL_WINDOW_BORDERLESS flag and
 *  add or remove the border from the actual window. This is a no-op if the
 *  window's border already matches the requested state.
 *
 *  \param window The window of which to change the border state.
 *  \param bordered SDL_FALSE to remove border, SDL_TRUE to add border.
 *
 *  \note You can't change the border state of a fullscreen window.
 *
 *  \sa SDL_GetWindowFlags()
 */
extern DECLSPEC void SDLCALL SDL_SetWindowBordered(SDL_Window * window,
                                                   SDL_bool bordered);

/**
 *  \brief Show a window.
 *
 *  \sa SDL_HideWindow()
 */
extern DECLSPEC void SDLCALL SDL_ShowWindow(SDL_Window * window);

/**
 *  \brief Hide a window.
 *
 *  \sa SDL_ShowWindow()
 */
extern DECLSPEC void SDLCALL SDL_HideWindow(SDL_Window * window);

/**
 *  \brief Raise a window above other windows and set the input focus.
 */
extern DECLSPEC void SDLCALL SDL_RaiseWindow(SDL_Window * window);

/**
 *  \brief Make a window as large as possible.
 *
 *  \sa SDL_RestoreWindow()
 */
extern DECLSPEC void SDLCALL SDL_MaximizeWindow(SDL_Window * window);

/**
 *  \brief Minimize a window to an iconic representation.
 *
 *  \sa SDL_RestoreWindow()
 */
extern DECLSPEC void SDLCALL SDL_MinimizeWindow(SDL_Window * window);

/**
 *  \brief Restore the size and position of a minimized or maximized window.
 *
 *  \sa SDL_MaximizeWindow()
 *  \sa SDL_MinimizeWindow()
 */
extern DECLSPEC void SDLCALL SDL_RestoreWindow(SDL_Window * window);

/**
 *  \brief Set a window's fullscreen state.
 *
 *  \return 0 on success, or -1 if setting the display mode failed.
 *
 *  \sa SDL_SetWindowDisplayMode()
 *  \sa SDL_GetWindowDisplayMode()
 */
extern DECLSPEC int SDLCALL SDL_SetWindowFullscreen(SDL_Window * window,
                                                    Uint32 flags);

/**
 *  \brief Get the SDL surface associated with the window.
 *
 *  \return The window's framebuffer surface, or NULL on error.
 *
 *  A new surface will be created with the optimal format for the window,
 *  if necessary. This surface will be freed when the window is destroyed.
 *
 *  \note You may not combine this with 3D or the rendering API on this window.
 *
 *  \sa SDL_UpdateWindowSurface()
 *  \sa SDL_UpdateWindowSurfaceRects()
 */
extern DECLSPEC SDL_Surface * SDLCALL SDL_GetWindowSurface(SDL_Window * window);

/**
 *  \brief Copy the window surface to the screen.
 *
 *  \return 0 on success, or -1 on error.
 *
 *  \sa SDL_GetWindowSurface()
 *  \sa SDL_UpdateWindowSurfaceRects()
 */
extern DECLSPEC int SDLCALL SDL_UpdateWindowSurface(SDL_Window * window);

/**
 *  \brief Copy a number of rectangles on the window surface to the screen.
 *
 *  \return 0 on success, or -1 on error.
 *
 *  \sa SDL_GetWindowSurface()
 *  \sa SDL_UpdateWindowSurfaceRect()
 */
extern DECLSPEC int SDLCALL SDL_UpdateWindowSurfaceRects(SDL_Window * window,
                                                         const SDL_Rect * rects,
                                                         int numrects);

/**
 *  \brief Set a window's input grab mode.
 *
 *  \param window The window for which the input grab mode should be set.
 *  \param grabbed This is SDL_TRUE to grab input, and SDL_FALSE to release input.
 *
 *  \sa SDL_GetWindowGrab()
 */
extern DECLSPEC void SDLCALL SDL_SetWindowGrab(SDL_Window * window,
                                               SDL_bool grabbed);

/**
 *  \brief Get a window's input grab mode.
 *
 *  \return This returns SDL_TRUE if input is grabbed, and SDL_FALSE otherwise.
 *
 *  \sa SDL_SetWindowGrab()
 */
extern DECLSPEC SDL_bool SDLCALL SDL_GetWindowGrab(SDL_Window * window);

/**
 *  \brief Set the brightness (gamma correction) for a window.
 *
 *  \return 0 on success, or -1 if setting the brightness isn't supported.
 *
 *  \sa SDL_GetWindowBrightness()
 *  \sa SDL_SetWindowGammaRamp()
 */
extern DECLSPEC int SDLCALL SDL_SetWindowBrightness(SDL_Window * window, float brightness);

/**
 *  \brief Get the brightness (gamma correction) for a window.
 *
 *  \return The last brightness value passed to SDL_SetWindowBrightness()
 *
 *  \sa SDL_SetWindowBrightness()
 */
extern DECLSPEC float SDLCALL SDL_GetWindowBrightness(SDL_Window * window);

/**
 *  \brief Set the gamma ramp for a window.
 *
 *  \param window The window for which the gamma ramp should be set.
 *  \param red The translation table for the red channel, or NULL.
 *  \param green The translation table for the green channel, or NULL.
 *  \param blue The translation table for the blue channel, or NULL.
 *
 *  \return 0 on success, or -1 if gamma ramps are unsupported.
 *
 *  Set the gamma translation table for the red, green, and blue channels
 *  of the video hardware.  Each table is an array of 256 16-bit quantities,
 *  representing a mapping between the input and output for that channel.
 *  The input is the index into the array, and the output is the 16-bit
 *  gamma value at that index, scaled to the output color precision.
 *
 *  \sa SDL_GetWindowGammaRamp()
 */
extern DECLSPEC int SDLCALL SDL_SetWindowGammaRamp(SDL_Window * window,
                                                   const Uint16 * red,
                                                   const Uint16 * green,
                                                   const Uint16 * blue);

/**
 *  \brief Get the gamma ramp for a window.
 *
 *  \param window The window from which the gamma ramp should be queried.
 *  \param red   A pointer to a 256 element array of 16-bit quantities to hold
 *               the translation table for the red channel, or NULL.
 *  \param green A pointer to a 256 element array of 16-bit quantities to hold
 *               the translation table for the green channel, or NULL.
 *  \param blue  A pointer to a 256 element array of 16-bit quantities to hold
 *               the translation table for the blue channel, or NULL.
 *
 *  \return 0 on success, or -1 if gamma ramps are unsupported.
 *
 *  \sa SDL_SetWindowGammaRamp()
 */
extern DECLSPEC int SDLCALL SDL_GetWindowGammaRamp(SDL_Window * window,
                                                   Uint16 * red,
                                                   Uint16 * green,
                                                   Uint16 * blue);

/**
 *  \brief Destroy a window.
 */
extern DECLSPEC void SDLCALL SDL_DestroyWindow(SDL_Window * window);


/**
 *  \brief Returns whether the screensaver is currently enabled (default on).
 *
 *  \sa SDL_EnableScreenSaver()
 *  \sa SDL_DisableScreenSaver()
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IsScreenSaverEnabled(void);

/**
 *  \brief Allow the screen to be blanked by a screensaver
 *
 *  \sa SDL_IsScreenSaverEnabled()
 *  \sa SDL_DisableScreenSaver()
 */
extern DECLSPEC void SDLCALL SDL_EnableScreenSaver(void);

/**
 *  \brief Prevent the screen from being blanked by a screensaver
 *
 *  \sa SDL_IsScreenSaverEnabled()
 *  \sa SDL_EnableScreenSaver()
 */
extern DECLSPEC void SDLCALL SDL_DisableScreenSaver(void);


/**
 *  \name OpenGL support functions
 */
/* @{ */

/**
 *  \brief Dynamically load an OpenGL library.
 *
 *  \param path The platform dependent OpenGL library name, or NULL to open the
 *              default OpenGL library.
 *
 *  \return 0 on success, or -1 if the library couldn't be loaded.
 *
 *  This should be done after initializing the video driver, but before
 *  creating any OpenGL windows.  If no OpenGL library is loaded, the default
 *  library will be loaded upon creation of the first OpenGL window.
 *
 *  \note If you do this, you need to retrieve all of the GL functions used in
 *        your program from the dynamic library using SDL_GL_GetProcAddress().
 *
 *  \sa SDL_GL_GetProcAddress()
 *  \sa SDL_GL_UnloadLibrary()
 */
extern DECLSPEC int SDLCALL SDL_GL_LoadLibrary(const char *path);

/**
 *  \brief Get the address of an OpenGL function.
 */
extern DECLSPEC void *SDLCALL SDL_GL_GetProcAddress(const char *proc);

/**
 *  \brief Unload the OpenGL library previously loaded by SDL_GL_LoadLibrary().
 *
 *  \sa SDL_GL_LoadLibrary()
 */
extern DECLSPEC void SDLCALL SDL_GL_UnloadLibrary(void);

/**
 *  \brief Return true if an OpenGL extension is supported for the current
 *         context.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_GL_ExtensionSupported(const char
                                                           *extension);

/**
 *  \brief Reset all previously set OpenGL context attributes to their default values
 */
extern DECLSPEC void SDLCALL SDL_GL_ResetAttributes(void);

/**
 *  \brief Set an OpenGL window attribute before window creation.
 */
extern DECLSPEC int SDLCALL SDL_GL_SetAttribute(SDL_GLattr attr, int value);

/**
 *  \brief Get the actual value for an attribute from the current context.
 */
extern DECLSPEC int SDLCALL SDL_GL_GetAttribute(SDL_GLattr attr, int *value);

/**
 *  \brief Create an OpenGL context for use with an OpenGL window, and make it
 *         current.
 *
 *  \sa SDL_GL_DeleteContext()
 */
extern DECLSPEC SDL_GLContext SDLCALL SDL_GL_CreateContext(SDL_Window *
                                                           window);

/**
 *  \brief Set up an OpenGL context for rendering into an OpenGL window.
 *
 *  \note The context must have been created with a compatible window.
 */
extern DECLSPEC int SDLCALL SDL_GL_MakeCurrent(SDL_Window * window,
                                               SDL_GLContext context);

/**
 *  \brief Get the currently active OpenGL window.
 */
extern DECLSPEC SDL_Window* SDLCALL SDL_GL_GetCurrentWindow(void);

/**
 *  \brief Get the currently active OpenGL context.
 */
extern DECLSPEC SDL_GLContext SDLCALL SDL_GL_GetCurrentContext(void);

/**
 *  \brief Get the size of a window's underlying drawable (for use with glViewport).
 *
 *  \param window   Window from which the drawable size should be queried
 *  \param w        Pointer to variable for storing the width, may be NULL
 *  \param h        Pointer to variable for storing the height, may be NULL
 *
 * This may differ from SDL_GetWindowSize if we're rendering to a high-DPI
 * drawable, i.e. the window was created with SDL_WINDOW_ALLOW_HIGHDPI on a
 * platform with high-DPI support (Apple calls this "Retina"), and not disabled
 * by the SDL_HINT_VIDEO_HIGHDPI_DISABLED hint.
 *
 *  \sa SDL_GetWindowSize()
 *  \sa SDL_CreateWindow()
 */
extern DECLSPEC void SDLCALL SDL_GL_GetDrawableSize(SDL_Window * window, int *w,
                                                    int *h);

/**
 *  \brief Set the swap interval for the current OpenGL context.
 *
 *  \param interval 0 for immediate updates, 1 for updates synchronized with the
 *                  vertical retrace. If the system supports it, you may
 *                  specify -1 to allow late swaps to happen immediately
 *                  instead of waiting for the next retrace.
 *
 *  \return 0 on success, or -1 if setting the swap interval is not supported.
 *
 *  \sa SDL_GL_GetSwapInterval()
 */
extern DECLSPEC int SDLCALL SDL_GL_SetSwapInterval(int interval);

/**
 *  \brief Get the swap interval for the current OpenGL context.
 *
 *  \return 0 if there is no vertical retrace synchronization, 1 if the buffer
 *          swap is synchronized with the vertical retrace, and -1 if late
 *          swaps happen immediately instead of waiting for the next retrace.
 *          If the system can't determine the swap interval, or there isn't a
 *          valid current context, this will return 0 as a safe default.
 *
 *  \sa SDL_GL_SetSwapInterval()
 */
extern DECLSPEC int SDLCALL SDL_GL_GetSwapInterval(void);

/**
 * \brief Swap the OpenGL buffers for a window, if double-buffering is
 *        supported.
 */
extern DECLSPEC void SDLCALL SDL_GL_SwapWindow(SDL_Window * window);

/**
 *  \brief Delete an OpenGL context.
 *
 *  \sa SDL_GL_CreateContext()
 */
extern DECLSPEC void SDLCALL SDL_GL_DeleteContext(SDL_GLContext context);

/* @} *//* OpenGL support functions */


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_video_h */

/* vi: set ts=4 sw=4 expandtab: */
