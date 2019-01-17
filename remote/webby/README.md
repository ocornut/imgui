
# Webby - A simple web server for game debugging #

Webby is a web server intended for debugging tools inside a game or other
program with a continously running main loop. It's intended to be used when
all you need is something tiny and performance isn't a key concern.

## Features ##

- No dynamic memory allocations -- server memory is completely fixed
- No threading, all I/O and serving happens on the calling thread
- Supports socket keep-alives
- Supports the 100-Continue scheme for file uploading
- Basic support for WebSockets is available.

Because request/response I/O is synchronous on the calling thread, performance
will suffer when you are serving data. For the use-cases Webby is intended for,
this is fine. You can still run Webby in a background thread at your
discretion if this is a problem.

## Usage ##

- Fill in a `WebbyServerConfig` structure
- Compute the memory requirements by calling `WebbyServerMemoryNeeded()`
- Allocate memory (or just use a static char array..)
- Initialize the server by calling `WebbyServerInit()`
- Every game frame, call `WebbyServerUpdate()`
  - Handle requests in your dispatch function
- When it's time to shut down, call `WebbyServerShutdown()`

## WebSocket Usage ##

- Provide the `WEBBY_SERVER_WEBSOCKETS` flag at init time
- In your `ws_connect` handler, decide to allow the upgrade request.
- When the connection is established, your `ws_connected` handler is called. At this point, you can start sending WebSocket frames over the connection (and you can get incoming frames as well).
- As frames come in, your `ws_frame` handler will be called. Webby doesn't try to merge frames together when the data is split into fragments, so you can be getting one binary/text frame followed by any number of continuation frames. It's up to your handler to make sense of the data and build a buffer as appropriate. The rationale is that otherwise Webby would need a lot more buffer space. To read the frame data, look at the payload size and then call `WebbyRead` to get at the data. Be careful to not read more than what is stated, or you will screw up the protocol.
- To send WebSocket data, call `WebbyBeginSocketFrame`, passing in the type of frame (typically `WEBBY_WS_OP_TEXT_FRAME` or `WEBBY_WS_OP_BINARY_FRAME`). Then write an arbitrary amount of data using `WebbyWrite`. When you're done, call `WebbyEndSocketFrame`. Webby currently emits one WebSocket (continuation) frame for each `WebbyWrite` call (again, to reduce buffering overhead), so try to write big chunks if you can.
- When a connection is closed, you will get a call to your `ws_closed` handler.

## Request handling ##

When you configure the server, you give it a function pointer to your
dispatcher. The dispatcher is called by Webby when a request has been fully
read into memory and is ready for processing. The socket the request came in on
has then been switched to blocking mode, and you're free to read any request
data using `WebbyRead()` (if present, check `content_length`) and then write
your response.

There are two ways to generate a response; explicit size or chunked.

### When you know the size of the data ###

When you know in advance how big the response is going to be, you should pass
that size in bytes to `WebbyBeginResponse()` (it will be sent as the
Content-Length header). You then call `WebbyWrite()` to push that data out, and
finally `WebbyEndResponse()` to finalize the response and prepare the socket
for a new request.

### When the response size is dynamic ###

Sometimes you want to generate an arbitrary amount of text in the response, and
you don't know how much that will be. Rather than buffering everything in RAM,
you can use chunked encoding. First call `WebbyBeginResponse()` as normal, but
pass it -1 for the content length. This triggers sending the
`Transfer-Encoding: chunked` header. You then call `WebbyWrite()` or
`WebbyPrintf()` as desired until the response is complete. When you're done,
call `WebbyEndResponse()` to finish up.

## Limitations ##

- There is not much error checking. 

## Author ##

Webby was written by Andreas Fredriksson (@deplinenoise on Twitter).

## License ##

Webby is available under the BSD license. See the file `LICENSE`.
