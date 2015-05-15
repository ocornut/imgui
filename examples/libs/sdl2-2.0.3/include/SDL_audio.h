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
 *  \file SDL_audio.h
 *
 *  Access to the raw audio mixing buffer for the SDL library.
 */

#ifndef _SDL_audio_h
#define _SDL_audio_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_endian.h"
#include "SDL_mutex.h"
#include "SDL_thread.h"
#include "SDL_rwops.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief Audio format flags.
 *
 *  These are what the 16 bits in SDL_AudioFormat currently mean...
 *  (Unspecified bits are always zero).
 *
 *  \verbatim
    ++-----------------------sample is signed if set
    ||
    ||       ++-----------sample is bigendian if set
    ||       ||
    ||       ||          ++---sample is float if set
    ||       ||          ||
    ||       ||          || +---sample bit size---+
    ||       ||          || |                     |
    15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    \endverbatim
 *
 *  There are macros in SDL 2.0 and later to query these bits.
 */
typedef Uint16 SDL_AudioFormat;

/**
 *  \name Audio flags
 */
/* @{ */

#define SDL_AUDIO_MASK_BITSIZE       (0xFF)
#define SDL_AUDIO_MASK_DATATYPE      (1<<8)
#define SDL_AUDIO_MASK_ENDIAN        (1<<12)
#define SDL_AUDIO_MASK_SIGNED        (1<<15)
#define SDL_AUDIO_BITSIZE(x)         (x & SDL_AUDIO_MASK_BITSIZE)
#define SDL_AUDIO_ISFLOAT(x)         (x & SDL_AUDIO_MASK_DATATYPE)
#define SDL_AUDIO_ISBIGENDIAN(x)     (x & SDL_AUDIO_MASK_ENDIAN)
#define SDL_AUDIO_ISSIGNED(x)        (x & SDL_AUDIO_MASK_SIGNED)
#define SDL_AUDIO_ISINT(x)           (!SDL_AUDIO_ISFLOAT(x))
#define SDL_AUDIO_ISLITTLEENDIAN(x)  (!SDL_AUDIO_ISBIGENDIAN(x))
#define SDL_AUDIO_ISUNSIGNED(x)      (!SDL_AUDIO_ISSIGNED(x))

/**
 *  \name Audio format flags
 *
 *  Defaults to LSB byte order.
 */
/* @{ */
#define AUDIO_U8        0x0008  /**< Unsigned 8-bit samples */
#define AUDIO_S8        0x8008  /**< Signed 8-bit samples */
#define AUDIO_U16LSB    0x0010  /**< Unsigned 16-bit samples */
#define AUDIO_S16LSB    0x8010  /**< Signed 16-bit samples */
#define AUDIO_U16MSB    0x1010  /**< As above, but big-endian byte order */
#define AUDIO_S16MSB    0x9010  /**< As above, but big-endian byte order */
#define AUDIO_U16       AUDIO_U16LSB
#define AUDIO_S16       AUDIO_S16LSB
/* @} */

/**
 *  \name int32 support
 */
/* @{ */
#define AUDIO_S32LSB    0x8020  /**< 32-bit integer samples */
#define AUDIO_S32MSB    0x9020  /**< As above, but big-endian byte order */
#define AUDIO_S32       AUDIO_S32LSB
/* @} */

/**
 *  \name float32 support
 */
/* @{ */
#define AUDIO_F32LSB    0x8120  /**< 32-bit floating point samples */
#define AUDIO_F32MSB    0x9120  /**< As above, but big-endian byte order */
#define AUDIO_F32       AUDIO_F32LSB
/* @} */

/**
 *  \name Native audio byte ordering
 */
/* @{ */
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define AUDIO_U16SYS    AUDIO_U16LSB
#define AUDIO_S16SYS    AUDIO_S16LSB
#define AUDIO_S32SYS    AUDIO_S32LSB
#define AUDIO_F32SYS    AUDIO_F32LSB
#else
#define AUDIO_U16SYS    AUDIO_U16MSB
#define AUDIO_S16SYS    AUDIO_S16MSB
#define AUDIO_S32SYS    AUDIO_S32MSB
#define AUDIO_F32SYS    AUDIO_F32MSB
#endif
/* @} */

/**
 *  \name Allow change flags
 *
 *  Which audio format changes are allowed when opening a device.
 */
/* @{ */
#define SDL_AUDIO_ALLOW_FREQUENCY_CHANGE    0x00000001
#define SDL_AUDIO_ALLOW_FORMAT_CHANGE       0x00000002
#define SDL_AUDIO_ALLOW_CHANNELS_CHANGE     0x00000004
#define SDL_AUDIO_ALLOW_ANY_CHANGE          (SDL_AUDIO_ALLOW_FREQUENCY_CHANGE|SDL_AUDIO_ALLOW_FORMAT_CHANGE|SDL_AUDIO_ALLOW_CHANNELS_CHANGE)
/* @} */

/* @} *//* Audio flags */

/**
 *  This function is called when the audio device needs more data.
 *
 *  \param userdata An application-specific parameter saved in
 *                  the SDL_AudioSpec structure
 *  \param stream A pointer to the audio data buffer.
 *  \param len    The length of that buffer in bytes.
 *
 *  Once the callback returns, the buffer will no longer be valid.
 *  Stereo samples are stored in a LRLRLR ordering.
 */
typedef void (SDLCALL * SDL_AudioCallback) (void *userdata, Uint8 * stream,
                                            int len);

/**
 *  The calculated values in this structure are calculated by SDL_OpenAudio().
 */
typedef struct SDL_AudioSpec
{
    int freq;                   /**< DSP frequency -- samples per second */
    SDL_AudioFormat format;     /**< Audio data format */
    Uint8 channels;             /**< Number of channels: 1 mono, 2 stereo */
    Uint8 silence;              /**< Audio buffer silence value (calculated) */
    Uint16 samples;             /**< Audio buffer size in samples (power of 2) */
    Uint16 padding;             /**< Necessary for some compile environments */
    Uint32 size;                /**< Audio buffer size in bytes (calculated) */
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;


struct SDL_AudioCVT;
typedef void (SDLCALL * SDL_AudioFilter) (struct SDL_AudioCVT * cvt,
                                          SDL_AudioFormat format);

/**
 *  A structure to hold a set of audio conversion filters and buffers.
 */
#ifdef __GNUC__
/* This structure is 84 bytes on 32-bit architectures, make sure GCC doesn't
   pad it out to 88 bytes to guarantee ABI compatibility between compilers.
   vvv
   The next time we rev the ABI, make sure to size the ints and add padding.
*/
#define SDL_AUDIOCVT_PACKED __attribute__((packed))
#else
#define SDL_AUDIOCVT_PACKED
#endif
/* */
typedef struct SDL_AudioCVT
{
    int needed;                 /**< Set to 1 if conversion possible */
    SDL_AudioFormat src_format; /**< Source audio format */
    SDL_AudioFormat dst_format; /**< Target audio format */
    double rate_incr;           /**< Rate conversion increment */
    Uint8 *buf;                 /**< Buffer to hold entire audio data */
    int len;                    /**< Length of original audio buffer */
    int len_cvt;                /**< Length of converted audio buffer */
    int len_mult;               /**< buffer must be len*len_mult big */
    double len_ratio;           /**< Given len, final size is len*len_ratio */
    SDL_AudioFilter filters[10];        /**< Filter list */
    int filter_index;           /**< Current audio conversion function */
} SDL_AUDIOCVT_PACKED SDL_AudioCVT;


/* Function prototypes */

/**
 *  \name Driver discovery functions
 *
 *  These functions return the list of built in audio drivers, in the
 *  order that they are normally initialized by default.
 */
/* @{ */
extern DECLSPEC int SDLCALL SDL_GetNumAudioDrivers(void);
extern DECLSPEC const char *SDLCALL SDL_GetAudioDriver(int index);
/* @} */

/**
 *  \name Initialization and cleanup
 *
 *  \internal These functions are used internally, and should not be used unless
 *            you have a specific need to specify the audio driver you want to
 *            use.  You should normally use SDL_Init() or SDL_InitSubSystem().
 */
/* @{ */
extern DECLSPEC int SDLCALL SDL_AudioInit(const char *driver_name);
extern DECLSPEC void SDLCALL SDL_AudioQuit(void);
/* @} */

/**
 *  This function returns the name of the current audio driver, or NULL
 *  if no driver has been initialized.
 */
extern DECLSPEC const char *SDLCALL SDL_GetCurrentAudioDriver(void);

/**
 *  This function opens the audio device with the desired parameters, and
 *  returns 0 if successful, placing the actual hardware parameters in the
 *  structure pointed to by \c obtained.  If \c obtained is NULL, the audio
 *  data passed to the callback function will be guaranteed to be in the
 *  requested format, and will be automatically converted to the hardware
 *  audio format if necessary.  This function returns -1 if it failed
 *  to open the audio device, or couldn't set up the audio thread.
 *
 *  When filling in the desired audio spec structure,
 *    - \c desired->freq should be the desired audio frequency in samples-per-
 *      second.
 *    - \c desired->format should be the desired audio format.
 *    - \c desired->samples is the desired size of the audio buffer, in
 *      samples.  This number should be a power of two, and may be adjusted by
 *      the audio driver to a value more suitable for the hardware.  Good values
 *      seem to range between 512 and 8096 inclusive, depending on the
 *      application and CPU speed.  Smaller values yield faster response time,
 *      but can lead to underflow if the application is doing heavy processing
 *      and cannot fill the audio buffer in time.  A stereo sample consists of
 *      both right and left channels in LR ordering.
 *      Note that the number of samples is directly related to time by the
 *      following formula:  \code ms = (samples*1000)/freq \endcode
 *    - \c desired->size is the size in bytes of the audio buffer, and is
 *      calculated by SDL_OpenAudio().
 *    - \c desired->silence is the value used to set the buffer to silence,
 *      and is calculated by SDL_OpenAudio().
 *    - \c desired->callback should be set to a function that will be called
 *      when the audio device is ready for more data.  It is passed a pointer
 *      to the audio buffer, and the length in bytes of the audio buffer.
 *      This function usually runs in a separate thread, and so you should
 *      protect data structures that it accesses by calling SDL_LockAudio()
 *      and SDL_UnlockAudio() in your code.
 *    - \c desired->userdata is passed as the first parameter to your callback
 *      function.
 *
 *  The audio device starts out playing silence when it's opened, and should
 *  be enabled for playing by calling \c SDL_PauseAudio(0) when you are ready
 *  for your audio callback function to be called.  Since the audio driver
 *  may modify the requested size of the audio buffer, you should allocate
 *  any local mixing buffers after you open the audio device.
 */
extern DECLSPEC int SDLCALL SDL_OpenAudio(SDL_AudioSpec * desired,
                                          SDL_AudioSpec * obtained);

/**
 *  SDL Audio Device IDs.
 *
 *  A successful call to SDL_OpenAudio() is always device id 1, and legacy
 *  SDL audio APIs assume you want this device ID. SDL_OpenAudioDevice() calls
 *  always returns devices >= 2 on success. The legacy calls are good both
 *  for backwards compatibility and when you don't care about multiple,
 *  specific, or capture devices.
 */
typedef Uint32 SDL_AudioDeviceID;

/**
 *  Get the number of available devices exposed by the current driver.
 *  Only valid after a successfully initializing the audio subsystem.
 *  Returns -1 if an explicit list of devices can't be determined; this is
 *  not an error. For example, if SDL is set up to talk to a remote audio
 *  server, it can't list every one available on the Internet, but it will
 *  still allow a specific host to be specified to SDL_OpenAudioDevice().
 *
 *  In many common cases, when this function returns a value <= 0, it can still
 *  successfully open the default device (NULL for first argument of
 *  SDL_OpenAudioDevice()).
 */
extern DECLSPEC int SDLCALL SDL_GetNumAudioDevices(int iscapture);

/**
 *  Get the human-readable name of a specific audio device.
 *  Must be a value between 0 and (number of audio devices-1).
 *  Only valid after a successfully initializing the audio subsystem.
 *  The values returned by this function reflect the latest call to
 *  SDL_GetNumAudioDevices(); recall that function to redetect available
 *  hardware.
 *
 *  The string returned by this function is UTF-8 encoded, read-only, and
 *  managed internally. You are not to free it. If you need to keep the
 *  string for any length of time, you should make your own copy of it, as it
 *  will be invalid next time any of several other SDL functions is called.
 */
extern DECLSPEC const char *SDLCALL SDL_GetAudioDeviceName(int index,
                                                           int iscapture);


/**
 *  Open a specific audio device. Passing in a device name of NULL requests
 *  the most reasonable default (and is equivalent to calling SDL_OpenAudio()).
 *
 *  The device name is a UTF-8 string reported by SDL_GetAudioDeviceName(), but
 *  some drivers allow arbitrary and driver-specific strings, such as a
 *  hostname/IP address for a remote audio server, or a filename in the
 *  diskaudio driver.
 *
 *  \return 0 on error, a valid device ID that is >= 2 on success.
 *
 *  SDL_OpenAudio(), unlike this function, always acts on device ID 1.
 */
extern DECLSPEC SDL_AudioDeviceID SDLCALL SDL_OpenAudioDevice(const char
                                                              *device,
                                                              int iscapture,
                                                              const
                                                              SDL_AudioSpec *
                                                              desired,
                                                              SDL_AudioSpec *
                                                              obtained,
                                                              int
                                                              allowed_changes);



/**
 *  \name Audio state
 *
 *  Get the current audio state.
 */
/* @{ */
typedef enum
{
    SDL_AUDIO_STOPPED = 0,
    SDL_AUDIO_PLAYING,
    SDL_AUDIO_PAUSED
} SDL_AudioStatus;
extern DECLSPEC SDL_AudioStatus SDLCALL SDL_GetAudioStatus(void);

extern DECLSPEC SDL_AudioStatus SDLCALL
SDL_GetAudioDeviceStatus(SDL_AudioDeviceID dev);
/* @} *//* Audio State */

/**
 *  \name Pause audio functions
 *
 *  These functions pause and unpause the audio callback processing.
 *  They should be called with a parameter of 0 after opening the audio
 *  device to start playing sound.  This is so you can safely initialize
 *  data for your callback function after opening the audio device.
 *  Silence will be written to the audio device during the pause.
 */
/* @{ */
extern DECLSPEC void SDLCALL SDL_PauseAudio(int pause_on);
extern DECLSPEC void SDLCALL SDL_PauseAudioDevice(SDL_AudioDeviceID dev,
                                                  int pause_on);
/* @} *//* Pause audio functions */

/**
 *  This function loads a WAVE from the data source, automatically freeing
 *  that source if \c freesrc is non-zero.  For example, to load a WAVE file,
 *  you could do:
 *  \code
 *      SDL_LoadWAV_RW(SDL_RWFromFile("sample.wav", "rb"), 1, ...);
 *  \endcode
 *
 *  If this function succeeds, it returns the given SDL_AudioSpec,
 *  filled with the audio data format of the wave data, and sets
 *  \c *audio_buf to a malloc()'d buffer containing the audio data,
 *  and sets \c *audio_len to the length of that audio buffer, in bytes.
 *  You need to free the audio buffer with SDL_FreeWAV() when you are
 *  done with it.
 *
 *  This function returns NULL and sets the SDL error message if the
 *  wave file cannot be opened, uses an unknown data format, or is
 *  corrupt.  Currently raw and MS-ADPCM WAVE files are supported.
 */
extern DECLSPEC SDL_AudioSpec *SDLCALL SDL_LoadWAV_RW(SDL_RWops * src,
                                                      int freesrc,
                                                      SDL_AudioSpec * spec,
                                                      Uint8 ** audio_buf,
                                                      Uint32 * audio_len);

/**
 *  Loads a WAV from a file.
 *  Compatibility convenience function.
 */
#define SDL_LoadWAV(file, spec, audio_buf, audio_len) \
    SDL_LoadWAV_RW(SDL_RWFromFile(file, "rb"),1, spec,audio_buf,audio_len)

/**
 *  This function frees data previously allocated with SDL_LoadWAV_RW()
 */
extern DECLSPEC void SDLCALL SDL_FreeWAV(Uint8 * audio_buf);

/**
 *  This function takes a source format and rate and a destination format
 *  and rate, and initializes the \c cvt structure with information needed
 *  by SDL_ConvertAudio() to convert a buffer of audio data from one format
 *  to the other.
 *
 *  \return -1 if the format conversion is not supported, 0 if there's
 *  no conversion needed, or 1 if the audio filter is set up.
 */
extern DECLSPEC int SDLCALL SDL_BuildAudioCVT(SDL_AudioCVT * cvt,
                                              SDL_AudioFormat src_format,
                                              Uint8 src_channels,
                                              int src_rate,
                                              SDL_AudioFormat dst_format,
                                              Uint8 dst_channels,
                                              int dst_rate);

/**
 *  Once you have initialized the \c cvt structure using SDL_BuildAudioCVT(),
 *  created an audio buffer \c cvt->buf, and filled it with \c cvt->len bytes of
 *  audio data in the source format, this function will convert it in-place
 *  to the desired format.
 *
 *  The data conversion may expand the size of the audio data, so the buffer
 *  \c cvt->buf should be allocated after the \c cvt structure is initialized by
 *  SDL_BuildAudioCVT(), and should be \c cvt->len*cvt->len_mult bytes long.
 */
extern DECLSPEC int SDLCALL SDL_ConvertAudio(SDL_AudioCVT * cvt);

#define SDL_MIX_MAXVOLUME 128
/**
 *  This takes two audio buffers of the playing audio format and mixes
 *  them, performing addition, volume adjustment, and overflow clipping.
 *  The volume ranges from 0 - 128, and should be set to ::SDL_MIX_MAXVOLUME
 *  for full audio volume.  Note this does not change hardware volume.
 *  This is provided for convenience -- you can mix your own audio data.
 */
extern DECLSPEC void SDLCALL SDL_MixAudio(Uint8 * dst, const Uint8 * src,
                                          Uint32 len, int volume);

/**
 *  This works like SDL_MixAudio(), but you specify the audio format instead of
 *  using the format of audio device 1. Thus it can be used when no audio
 *  device is open at all.
 */
extern DECLSPEC void SDLCALL SDL_MixAudioFormat(Uint8 * dst,
                                                const Uint8 * src,
                                                SDL_AudioFormat format,
                                                Uint32 len, int volume);

/**
 *  \name Audio lock functions
 *
 *  The lock manipulated by these functions protects the callback function.
 *  During a SDL_LockAudio()/SDL_UnlockAudio() pair, you can be guaranteed that
 *  the callback function is not running.  Do not call these from the callback
 *  function or you will cause deadlock.
 */
/* @{ */
extern DECLSPEC void SDLCALL SDL_LockAudio(void);
extern DECLSPEC void SDLCALL SDL_LockAudioDevice(SDL_AudioDeviceID dev);
extern DECLSPEC void SDLCALL SDL_UnlockAudio(void);
extern DECLSPEC void SDLCALL SDL_UnlockAudioDevice(SDL_AudioDeviceID dev);
/* @} *//* Audio lock functions */

/**
 *  This function shuts down audio processing and closes the audio device.
 */
extern DECLSPEC void SDLCALL SDL_CloseAudio(void);
extern DECLSPEC void SDLCALL SDL_CloseAudioDevice(SDL_AudioDeviceID dev);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_audio_h */

/* vi: set ts=4 sw=4 expandtab: */
