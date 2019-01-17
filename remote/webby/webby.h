#ifndef WEBBY_H
#define WEBBY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
   Webby - A tiny little web server for game debugging.
*/

/* Server initialization flags */
enum
{
  WEBBY_SERVER_LOG_DEBUG    = 1 << 0,
  WEBBY_SERVER_WEBSOCKETS   = 1 << 1
};

/* Hard limits */
enum
{
  WEBBY_MAX_HEADERS         = 64
};

struct WebbyServer;

/* A HTTP header */
struct WebbyHeader
{
  const char *name;
  const char *value;
};

/* A HTTP request. */
struct WebbyRequest
{
  /* The method of the request, e.g. "GET", "POST" and so on */
  const char *method;
  /* The URI that was used. */
  const char *uri;
  /* The HTTP version that used. */
  const char *http_version;
  /* The query parameters passed in the URL, or NULL if none were passed. */
  const char *query_params;
  /* The number of bytes of request body that are available via WebbyRead() */
  int content_length;
  /* The number of headers */
  int header_count;
  /* Request headers */
  struct WebbyHeader headers[WEBBY_MAX_HEADERS];
};

/* Connection state, as published to the serving callback. */
struct WebbyConnection
{
  /* The request being served. Read-only. */
  struct WebbyRequest request;
  
  /* User data. Read-write. Webby doesn't care about this. */
  void *user_data;
};

enum
{
  WEBBY_WS_OP_CONTINUATION    = 0,
  WEBBY_WS_OP_TEXT_FRAME      = 1,
  WEBBY_WS_OP_BINARY_FRAME    = 2,
  WEBBY_WS_OP_CLOSE           = 8,
  WEBBY_WS_OP_PING            = 9,
  WEBBY_WS_OP_PONG            = 10
};

enum
{
  WEBBY_WSF_FIN               = 1 << 0,
  WEBBY_WSF_MASKED            = 1 << 1
};

struct WebbyWsFrame
{
  unsigned char flags;
  unsigned char opcode;
  unsigned char header_size;
  unsigned char padding_;
  unsigned char mask_key[4];
  int           payload_length;
};

/* Configuration data required for starting a server. */
struct WebbyServerConfig
{
  /* The bind address. Must be a textual IP address. */
  const char *bind_address;

  /* The port to listen to. */
  unsigned short listening_port;

  /* Flags. Right now WEBBY_SERVER_LOG_DEBUG is the only valid flag. */
  unsigned int flags;

  /* Maximum number of simultaneous connections. */
  int connection_max;

  /* The size of the request buffer. This must be big enough to contain all
   * headers and the request line sent by the client. 2-4k is a good size for
   * this buffer. */
  int request_buffer_size;

  /* The size of the I/O buffer, used when writing the reponse. 4k is a good
   * choice for this buffer.*/
  int io_buffer_size;

  /* Optional callback function that receives debug log text (without
   * newlines). */
  void (*log)(const char *msg);

  /* Request dispatcher function. This function is called when the request
   * structure is ready.
   *
   * If you decide to handle the request, call WebbyBeginResponse(),
   * WebbyWrite() and WebbyEndResponse() and then return 0. Otherwise, return a
   * non-zero value to have Webby send back a 404 response.
   */
  int (*dispatch)(struct WebbyConnection *connection);

  /*
   * WebSocket connection dispatcher. Called when an incoming request wants to
   * update to a WebSocket connection.
   *
   * Return 0 to allow the connection.
   * Return 1 to ignore the connection.
   */
  int (*ws_connect)(struct WebbyConnection *connection);

  /*
   * Called when a WebSocket connection has been established.
   */
  void (*ws_connected)(struct WebbyConnection *connection);

  /*
   * Called when a WebSocket connection has been closed.
   */
  void (*ws_closed)(struct WebbyConnection *connection);

  /*
   * Called when a WebSocket data frame is incoming.
   *
   * Call WebbyRead() to read the payload data.
   *
   * Return non-zero to close the connection.
   */
  int (*ws_frame)(struct WebbyConnection *connection, const struct WebbyWsFrame *frame);
};

/* Returns the amount of memory needed for the specified config. */
int
WebbyServerMemoryNeeded(const struct WebbyServerConfig *config);

/* Initialize a server in the specified memory space. Size must be big enough,
 * as determined by WebbyServerMemoryNeeded(). The memory block must be aligned
 * to at least 8 bytes.
 */
struct WebbyServer*
WebbyServerInit(struct WebbyServerConfig *config, void *memory, size_t memory_size);

/* Update the server. Call frequently (at least once per frame). */
void
WebbyServerUpdate(struct WebbyServer *srv);

/* Shutdown the server and close all sockets. */
void
WebbyServerShutdown(struct WebbyServer *srv);

/*
 * Begin a response.
 *
 * status_code - The HTTP status code to send. Normally 200
 * content_length - size in bytes you intend to write, or -1 for chunked encoding
 * headers - Array of HTTP headers to transmit (can be NULL if header_count ==0)
 * header_count - Number of headers in the array.
 *
 * Returns zero on success, non-zero on error.
 */
int
WebbyBeginResponse(
    struct WebbyConnection *conn,
    int status_code,
    int content_length,
    const struct WebbyHeader headers[],
    int header_count);

/*
 * Finish a response.
 *
 * When you're done writing the response body, call this function. It makes
 * sure that chunked encoding is terminated correctly and that the connection
 * is set up for reuse.
 */
void
WebbyEndResponse(struct WebbyConnection *conn);

/*
 * Read data from the request body. Only read what the client has provided (via
 * the content_length) parameter, or you will end up blocking forever.
 */
int WebbyRead(struct WebbyConnection *conn, void *ptr, size_t len);

/*
 * Write response data to the connection. If you're not using chunked encoding,
 * be careful not to send more than the specified content length. You can call
 * this function multiple times as long as the total number of bytes matches up
 * with the content length.
 */
int WebbyWrite(struct WebbyConnection *conn, const void *ptr, size_t len);

/*
 * Convenience function to do formatted printing to a response. Only useful
 * when chunked encoding is being used.
 */
int WebbyPrintf(struct WebbyConnection *conn, const char *fmt, ...);

/*
 * Convenience function to find a header in a request. Returns the value of the
 * specified header, or NULL if it was not present.
 */
const char *WebbyFindHeader(struct WebbyConnection *conn, const char *name);

/* Helper function to look up a query parameter given a URL encoded string.

   Returns the size of the returned data, or -1 if the query var wasn't found.
 */
int WebbyFindQueryVar(const char *query_params, const char *name, char *buffer, size_t buffer_size);

/* Begin an outgoing websocket frame */
int
WebbyBeginSocketFrame(struct WebbyConnection *conn, int websocket_opcode);

/* End an outgoing websocket frame */
int
WebbyEndSocketFrame(struct WebbyConnection *conn);

int
WebbySendFrame(struct WebbyConnection *conn_pub, int opcode, const void *ptr, size_t len);

#ifdef __cplusplus
}
#endif

#endif
