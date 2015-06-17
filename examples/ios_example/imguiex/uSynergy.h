/*
uSynergy client -- Interface for the embedded Synergy client library
  version 1.0.0, July 7th, 2012

Copyright (C) 2012 Synergy Si Ltd.
Copyright (c) 2012 Alex Evans

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
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

   3. This notice may not be removed or altered from any source
   distribution.
*/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif



//---------------------------------------------------------------------------------------------------------------------
//	Configuration
//---------------------------------------------------------------------------------------------------------------------



/**
@brief Determine endianness
**/
#if defined(USYNERGY_LITTLE_ENDIAN) && defined(USYNERGY_BIG_ENDIAN)
	/* Ambiguous: both endians specified */
	#error "Can't define both USYNERGY_LITTLE_ENDIAN and USYNERGY_BIG_ENDIAN"
#elif !defined(USYNERGY_LITTLE_ENDIAN) && !defined(USYNERGY_BIG_ENDIAN)
	/* Attempt to auto detect */
	#if defined(__LITTLE_ENDIAN__) || defined(LITTLE_ENDIAN) || (_BYTE_ORDER == _LITTLE_ENDIAN)
		#define USYNERGY_LITTLE_ENDIAN
	#elif defined(__BIG_ENDIAN__) || defined(BIG_ENDIAN) || (_BYTE_ORDER == _BIG_ENDIAN)
		#define USYNERGY_BIG_ENDIAN
	#else
		#error "Can't detect endian-nes, please defined either USYNERGY_LITTLE_ENDIAN or USYNERGY_BIG_ENDIAN";
	#endif
#else
	/* User-specified endian-nes, nothing to do for us */
#endif



//---------------------------------------------------------------------------------------------------------------------
//	Types and Constants
//---------------------------------------------------------------------------------------------------------------------



/**
@brief Boolean type
**/
typedef int			uSynergyBool;
#define				USYNERGY_FALSE					0				/* False value */
#define				USYNERGY_TRUE					1				/* True value */


/**
@brief User context type

The uSynergyCookie type is an opaque type that is used by uSynergy to communicate to the client. It is passed along to
callback functions as context.
**/
typedef struct { int ignored; } *					uSynergyCookie;



/**
@brief Clipboard types
**/
enum uSynergyClipboardFormat
{
	USYNERGY_CLIPBOARD_FORMAT_TEXT					= 0,			/* Text format, UTF-8, newline is LF */
	USYNERGY_CLIPBOARD_FORMAT_BITMAP				= 1,			/* Bitmap format, BMP 24/32bpp, BI_RGB */
	USYNERGY_CLIPBOARD_FORMAT_HTML					= 2,			/* HTML format, HTML fragment, UTF-8, newline is LF */
};



/**
@brief Constants and limits
**/
#define				USYNERGY_NUM_JOYSTICKS			4				/* Maximum number of supported joysticks */

#define				USYNERGY_PROTOCOL_MAJOR			1				/* Major protocol version */
#define				USYNERGY_PROTOCOL_MINOR			4				/* Minor protocol version */

#define				USYNERGY_IDLE_TIMEOUT			2000			/* Timeout in milliseconds before reconnecting */

#define				USYNERGY_TRACE_BUFFER_SIZE		1024			/* Maximum length of traced message */
#define				USYNERGY_REPLY_BUFFER_SIZE		1024			/* Maximum size of a reply packet */
#define				USYNERGY_RECEIVE_BUFFER_SIZE	4096			/* Maximum size of an incoming packet */



/**
@brief Keyboard constants
**/
#define				USYNERGY_MODIFIER_SHIFT			0x0001			/* Shift key modifier */
#define				USYNERGY_MODIFIER_CTRL			0x0002			/* Ctrl key modifier */
#define				USYNERGY_MODIFIER_ALT			0x0004			/* Alt key modifier */
#define				USYNERGY_MODIFIER_META			0x0008			/* Meta key modifier */
#define				USYNERGY_MODIFIER_WIN			0x0010			/* Windows key modifier */
#define				USYNERGY_MODIFIER_ALT_GR		0x0020			/* AltGr key modifier */
#define				USYNERGY_MODIFIER_LEVEL5LOCK	0x0040			/* Level5Lock key modifier */
#define				USYNERGY_MODIFIER_CAPSLOCK		0x1000			/* CapsLock key modifier */
#define				USYNERGY_MODIFIER_NUMLOCK		0x2000			/* NumLock key modifier */
#define				USYNERGY_MODIFIER_SCROLLOCK		0x4000			/* ScrollLock key modifier */




//---------------------------------------------------------------------------------------------------------------------
//	Functions and Callbacks
//---------------------------------------------------------------------------------------------------------------------



/**
@brief Connect function

This function is called when uSynergy needs to connect to the host. It doesn't imply a network implementation or
destination address, that must all be handled on the user side. The function should return USYNERGY_TRUE if a
connection was established or USYNERGY_FALSE if it could not connect.

When network errors occur (e.g. uSynergySend or uSynergyReceive fail) then the connect call will be called again
so the implementation of the function must close any old connections and clean up resources before retrying.

@param cookie		Cookie supplied in the Synergy context
**/
typedef uSynergyBool (*uSynergyConnectFunc)(uSynergyCookie cookie);



/**
@brief Send function

This function is called when uSynergy needs to send something over the default connection. It should return
USYNERGY_TRUE if sending succeeded and USYNERGY_FALSE otherwise. This function should block until the send
operation is completed.

@param cookie		Cookie supplied in the Synergy context
@param buffer		Address of buffer to send
@param length		Length of buffer to send
**/
typedef uSynergyBool (*uSynergySendFunc)(uSynergyCookie cookie, const uint8_t *buffer, int length);



/**
@brief Receive function

This function is called when uSynergy needs to receive data from the default connection. It should return
USYNERGY_TRUE if receiving data succeeded and USYNERGY_FALSE otherwise. This function should block until data
has been received and wait for data to become available. If @a outLength is set to 0 upon completion it is
assumed that the connection is alive, but still in a connecting state and needs time to settle.

@param cookie		Cookie supplied in the Synergy context
@param buffer		Address of buffer to receive data into
@param maxLength	Maximum amount of bytes to write into the receive buffer
@param outLength	Address of integer that receives the actual amount of bytes written into @a buffer
**/
typedef uSynergyBool (*uSynergyReceiveFunc)(uSynergyCookie cookie, uint8_t *buffer, int maxLength, int* outLength);



/**
@brief Thread sleep function

This function is called when uSynergy wants to suspend operation for a while before retrying an operation. It
is mostly used when a socket times out or disconnect occurs to prevent uSynergy from continuously hammering a
network connection in case the network is down.

@param cookie		Cookie supplied in the Synergy context
@param timeMs		Time to sleep the current thread (in milliseconds)
**/
typedef void		(*uSynergySleepFunc)(uSynergyCookie cookie, int timeMs);



/**
@brief Get time function

This function is called when uSynergy needs to know the current time. This is used to determine when timeouts
have occured. The time base should be a cyclic millisecond time value.

@returns			Time value in milliseconds
**/
typedef uint32_t	(*uSynergyGetTimeFunc)();



/**
@brief Trace function

This function is called when uSynergy wants to trace something. It is optional to show these messages, but they
are often useful when debugging. uSynergy only traces major events like connecting and disconnecting. Usually
only a single trace is shown when the connection is established and no more trace are called.

@param cookie		Cookie supplied in the Synergy context
@param text			Text to be traced
**/
typedef void		(*uSynergyTraceFunc)(uSynergyCookie cookie, const char *text);



/**
@brief Screen active callback

This callback is called when Synergy makes the screen active or inactive. This
callback is usually sent when the mouse enters or leaves the screen.

@param cookie		Cookie supplied in the Synergy context
@param active		Activation flag, 1 if the screen has become active, 0 if the screen has become inactive
**/
typedef void		(*uSynergyScreenActiveCallback)(uSynergyCookie cookie, uSynergyBool active);



/**
@brief Mouse callback

This callback is called when a mouse events happens. The mouse X and Y position,
wheel and button state is communicated in the message. It's up to the user to
interpret if this is a mouse up, down, double-click or other message.

@param cookie		Cookie supplied in the Synergy context
@param x			Mouse X position
@param y			Mouse Y position
@param wheelX		Mouse wheel X position
@param wheelY		Mouse wheel Y position
@param buttonLeft	Left button pressed status, 0 for released, 1 for pressed
@param buttonMiddle	Middle button pressed status, 0 for released, 1 for pressed
@param buttonRight	Right button pressed status, 0 for released, 1 for pressed
**/
typedef void		(*uSynergyMouseCallback)(uSynergyCookie cookie, uint16_t x, uint16_t y, int16_t wheelX, int16_t wheelY, uSynergyBool buttonLeft, uSynergyBool buttonRight, uSynergyBool buttonMiddle);



/**
@brief Key event callback

This callback is called when a key is pressed or released.

@param cookie		Cookie supplied in the Synergy context
@param key			Key code of key that was pressed or released
@param modifiers	Status of modifier keys (alt, shift, etc.)
@param down			Down or up status, 1 is key is pressed down, 0 if key is released (up)
@param repeat		Repeat flag, 1 if the key is down because the key is repeating, 0 if the key is initially pressed by the user
**/
typedef void		(*uSynergyKeyboardCallback)(uSynergyCookie cookie, uint16_t key, uint16_t modifiers, uSynergyBool down, uSynergyBool repeat);



/**
@brief Joystick event callback

This callback is called when a joystick stick or button changes. It is possible that multiple callbacks are
fired when different sticks or buttons change as these are individual messages in the packet stream. Each
callback will contain all the valid state for the different axes and buttons. The last callback received will
represent the most current joystick state.

@param cookie		Cookie supplied in the Synergy context
@param joyNum		Joystick number, always in the range [0 ... USYNERGY_NUM_JOYSTICKS>
@param buttons		Button pressed mask
@param leftStickX	Left stick X position, in range [-127 ... 127]
@param leftStickY	Left stick Y position, in range [-127 ... 127]
@param rightStickX	Right stick X position, in range [-127 ... 127]
@param rightStickY	Right stick Y position, in range [-127 ... 127]
**/
typedef void		(*uSynergyJoystickCallback)(uSynergyCookie cookie, uint8_t joyNum, uint16_t buttons, int8_t leftStickX, int8_t leftStickY, int8_t rightStickX, int8_t rightStickY);



/**
@brief Clipboard event callback

This callback is called when something is placed on the clipboard. Multiple callbacks may be fired for
multiple clipboard formats if they are supported. The data provided is read-only and may not be modified
by the application.

@param cookie		Cookie supplied in the Synergy context
@param format		Clipboard format
@param data			Memory area containing the clipboard raw data
@param size			Size of clipboard data
**/
typedef void		(*uSynergyClipboardCallback)(uSynergyCookie cookie, enum uSynergyClipboardFormat format, const uint8_t *data, uint32_t size);



//---------------------------------------------------------------------------------------------------------------------
//	Context
//---------------------------------------------------------------------------------------------------------------------



/**
@brief uSynergy context
**/
typedef struct
{
	/* Mandatory configuration data, filled in by client */
	uSynergyConnectFunc				m_connectFunc;									/* Connect function */
	uSynergySendFunc				m_sendFunc;										/* Send data function */
	uSynergyReceiveFunc				m_receiveFunc;									/* Receive data function */
	uSynergySleepFunc				m_sleepFunc;									/* Thread sleep function */
	uSynergyGetTimeFunc				m_getTimeFunc;									/* Get current time function */
	const char*						m_clientName;									/* Name of Synergy Screen / Client */
	uint16_t						m_clientWidth;									/* Width of screen */
	uint16_t						m_clientHeight;									/* Height of screen */

	/* Optional configuration data, filled in by client */
	uSynergyCookie					m_cookie;										/* Cookie pointer passed to callback functions (can be NULL) */
	uSynergyTraceFunc				m_traceFunc;									/* Function for tracing status (can be NULL) */
	uSynergyScreenActiveCallback	m_screenActiveCallback;							/* Callback for entering and leaving screen */
	uSynergyMouseCallback			m_mouseCallback;								/* Callback for mouse events */
	uSynergyKeyboardCallback		m_keyboardCallback;								/* Callback for keyboard events */
	uSynergyJoystickCallback		m_joystickCallback;								/* Callback for joystick events */
	uSynergyClipboardCallback		m_clipboardCallback;							/* Callback for clipboard events */

	/* State data, used internall by client, initialized by uSynergyInit() */
	uSynergyBool					m_connected;									/* Is our socket connected? */
	uSynergyBool					m_hasReceivedHello;								/* Have we received a 'Hello' from the server? */
	uSynergyBool					m_isCaptured;									/* Is Synergy active (i.e. this client is receiving input messages?) */
	uint32_t						m_lastMessageTime;								/* Time at which last message was received */
	uint32_t						m_sequenceNumber;								/* Packet sequence number */
	uint8_t							m_receiveBuffer[USYNERGY_RECEIVE_BUFFER_SIZE];	/* Receive buffer */
	int								m_receiveOfs;									/* Receive buffer offset */
	uint8_t							m_replyBuffer[USYNERGY_REPLY_BUFFER_SIZE];		/* Reply buffer */
	uint8_t*						m_replyCur;										/* Write offset into reply buffer */
	uint16_t						m_mouseX;										/* Mouse X position */
	uint16_t						m_mouseY;										/* Mouse Y position */
	int16_t							m_mouseWheelX;									/* Mouse wheel X position */
	int16_t							m_mouseWheelY;									/* Mouse wheel Y position */
	uSynergyBool					m_mouseButtonLeft;								/* Mouse left button */
	uSynergyBool					m_mouseButtonRight;								/* Mouse right button */
	uSynergyBool					m_mouseButtonMiddle;							/* Mouse middle button */
	int8_t							m_joystickSticks[USYNERGY_NUM_JOYSTICKS][4];	/* Joystick stick position in 2 axes for 2 sticks */
	uint16_t						m_joystickButtons[USYNERGY_NUM_JOYSTICKS];		/* Joystick button state */
} uSynergyContext;



//---------------------------------------------------------------------------------------------------------------------
//	Interface
//---------------------------------------------------------------------------------------------------------------------



/**
@brief Initialize uSynergy context

This function initializes @a context for use. Call this function directly after
creating the context, before filling in any configuration data in it. Not calling
this function will cause undefined behavior.

@param context	Context to be initialized
**/
extern void		uSynergyInit(uSynergyContext *context);



/**
@brief Update uSynergy

This function updates uSynergy and does the bulk of the work. It does connection management,
receiving data, reconnecting after errors or timeouts and so on. It assumes that networking
operations are blocking and it can suspend the current thread if it needs to wait. It is
best practice to call uSynergyUpdate from a background thread so it is responsive.

Because uSynergy relies mostly on blocking calls it will mostly stay in thread sleep state
waiting for system mutexes and won't eat much memory.

uSynergyUpdate doesn't do any memory allocations or have any side effects beyond those of
the callbacks it calls.

@param context	Context to be updated
**/
extern void		uSynergyUpdate(uSynergyContext *context);



/**
@brief Send clipboard data

This function sets new clipboard data and sends it to the server. Use this function if
your client cuts or copies data onto the clipboard that it needs to share with the
server.

Currently there is only support for plaintext, but HTML and image data could be
supported with some effort.

@param context	Context to send clipboard data to
@param text		Text to set to the clipboard
**/
extern void		uSynergySendClipboard(uSynergyContext *context, const char *text);



#ifdef __cplusplus
};
#endif
