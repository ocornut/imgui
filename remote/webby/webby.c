#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include "webby.h"

/* Copyright (c) 2012, Andreas Fredriksson < dep at defmacro dot se > */

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#if defined(__PS3__)
#include "webby_ps3.h"
#elif defined(__XBOX__)
#include "webby_xbox.h"
#elif defined(_WIN32)
#include "webby_win32.h"
#else
#include "webby_unix.h"
#endif

#define WB_WEBSOCKET_VERSION "13"
#define WB_ALIGN_ARB(x, a) (((x) + ((a)-1)) & ~((a)-1))
#define WB_ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

static const char continue_header[] = "HTTP/1.1 100 Continue\r\n\r\n";
static const size_t continue_header_len = sizeof(continue_header) - 1;

static const char websocket_guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
static const size_t websocket_guid_len = sizeof(websocket_guid) - 1;

static const unsigned char websocket_pong[] = { 0x80, WEBBY_WS_OP_PONG, 0x00 };

static const struct WebbyHeader plain_text_headers[] =
{
  { "Content-Type", "text/plain" },
};

#ifdef _MSC_VER
/* MSVC keeps complaining about constant conditionals inside the FD_SET() macro. */
#pragma warning(disable: 4127)
#endif

struct WebbyConnection;
struct WebbyRequest;

enum
{
  WB_ALIVE                  = 1 << 0,
  WB_FRESH_CONNECTION       = 1 << 1,
  WB_CLOSE_AFTER_RESPONSE   = 1 << 2,
  WB_CHUNKED_RESPONSE       = 1 << 3,
  WB_WEBSOCKET              = 1 << 4
};

enum
{
  WBC_REQUEST,
  WBC_SEND_CONTINUE,
  WBC_SERVE,
  WBC_WEBSOCKET
};

struct WebbyBuffer
{
  int used;
  int max;
  unsigned char* data;
};

struct WebbyConnectionPrv
{
  struct WebbyConnection    public_data;

  unsigned short            flags;
  unsigned short            state;    /* WBC_xxx */
  webby_socket_t            socket;

  struct WebbyBuffer        header_buf;
  struct WebbyBuffer        io_buf;
  int                       header_body_left;
  int                       io_data_left;
  int                       continue_data_left;
  int                       body_bytes_read;
  struct WebbyServer*       server;
  struct WebbyWsFrame       ws_frame;
  unsigned char             ws_opcode;
  int                       blocking_count; /* number of times blocking has been requested */
};

struct WebbyServer
{
  struct WebbyServerConfig  config;
  size_t                    memory_size;
  webby_socket_t            socket;
  int                       connection_count;
  struct WebbyConnectionPrv connections[1];
};

static void dbg(struct WebbyServer *srv, const char *fmt, ...)
{
  char buffer[1024];
  va_list args;

  if (srv->config.flags & WEBBY_SERVER_LOG_DEBUG)
  {
    va_start(args, fmt);
    vsnprintf(buffer, sizeof buffer, fmt, args);
    va_end(args);

    buffer[(sizeof buffer)-1] = '\0';
    (*srv->config.log)(buffer);
  }
}

static int make_connection_blocking(struct WebbyConnectionPrv *conn)
{
  if (0 == conn->blocking_count)
  {
    if (0 != wb_set_blocking(conn->socket, 1))
    {
      dbg(conn->server, "failed to switch connection to blocking");
      conn->flags &= ~WB_ALIVE;
      return -1;
    }
  }

  ++conn->blocking_count;
  return 0;
}

static int make_connection_nonblocking(struct WebbyConnectionPrv *conn)
{
  int count = conn->blocking_count;

  if (1 == count)
  {
    if (0 != wb_set_blocking(conn->socket, 0))
    {
      dbg(conn->server, "failed to switch connection to non-blocking");
      conn->flags &= ~WB_ALIVE;
      return -1;
    }
  }

  conn->blocking_count = count - 1;

  return 0;
}

/* URL-decode input buffer into destination buffer.
 * 0-terminate the destination buffer. Return the length of decoded data.
 * form-url-encoded data differs from URI encoding in a way that it
 * uses '+' as character for space, see RFC 1866 section 8.2.1
 * http://ftp.ics.uci.edu/pub/ietf/html/rfc1866.txt
 *
 * This bit of code was taken from mongoose.
 */
static size_t url_decode(const char *src, size_t src_len, char *dst, size_t dst_len, int is_form_url_encoded)
{
  size_t i, j;
  int a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

  for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
    if (src[i] == '%' &&
      isxdigit(* (const unsigned char *) (src + i + 1)) &&
      isxdigit(* (const unsigned char *) (src + i + 2))) {
        a = tolower(* (const unsigned char *) (src + i + 1));
        b = tolower(* (const unsigned char *) (src + i + 2));
        dst[j] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
        i += 2;
    } else if (is_form_url_encoded && src[i] == '+') {
      dst[j] = ' ';
    } else {
      dst[j] = src[i];
    }
  }

#undef HEXTOI

  dst[j] = '\0'; /* Null-terminate the destination */

  return j;
}

/* Pulled from mongoose */
int WebbyFindQueryVar(const char *buf, const char *name, char *dst, size_t dst_len)
{
  const char *p, *e, *s;
  size_t name_len;
  int len;
  size_t buf_len = strlen(buf);

  name_len = strlen(name);
  e = buf + buf_len;
  len = -1;
  dst[0] = '\0';

  // buf is "var1=val1&var2=val2...". Find variable first
  for (p = buf; p != NULL && p + name_len < e; p++)
  {
    if ((p == buf || p[-1] == '&') && p[name_len] == '=' && 0 == strncasecmp(name, p, name_len))
    {
      // Point p to variable value
      p += name_len + 1;

      // Point s to the end of the value
      s = (const char *) memchr(p, '&', (size_t)(e - p));
      if (s == NULL) {
        s = e;
      }
      assert(s >= p);

      // Decode variable into destination buffer
      if ((size_t) (s - p) < dst_len)
      {
        len = (int) url_decode(p, (size_t)(s - p), dst, dst_len, 1);
      }
      break;
    }
  }

  return len;
}

enum
{
  BASE64_QUADS_BEFORE_LINEBREAK = 19
};

static size_t base64_bufsize(size_t input_size)
{
  size_t triplets = (input_size + 2) / 3;
  size_t base_size = 4 * triplets;
  size_t line_breaks = 2 * (triplets / BASE64_QUADS_BEFORE_LINEBREAK);
  size_t null_termination = 1;
  return base_size + line_breaks + null_termination;
}

static int base64_encode(char* output, size_t output_size, const unsigned char *input, size_t input_size)
{
  static const char enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="; 
  size_t i = 0;
  int line_out = 0;

  if (output_size < base64_bufsize(input_size))
    return 1;

  while (i < input_size)
  {
    unsigned int idx_0, idx_1, idx_2, idx_3;
    unsigned int i0;

    i0 = (input[i]) << 16; i++;
    i0 |= (i < input_size ? input[i] : 0) << 8; i++;
    i0 |= (i < input_size ? input[i] : 0); i++;

    idx_0 = (i0 & 0xfc0000) >> 18; i0 <<= 6;
    idx_1 = (i0 & 0xfc0000) >> 18; i0 <<= 6;
    idx_2 = (i0 & 0xfc0000) >> 18; i0 <<= 6;
    idx_3 = (i0 & 0xfc0000) >> 18;

    if (i - 1 > input_size)
      idx_2 = 64;
    if (i > input_size)
      idx_3 = 64;

    *output++ = enc[idx_0];
    *output++ = enc[idx_1];
    *output++ = enc[idx_2];
    *output++ = enc[idx_3];

    if (++line_out == BASE64_QUADS_BEFORE_LINEBREAK) {
      *output++ = '\r';
      *output++ = '\n';
    }
  }

  *output = '\0';
  return 0;
}

static unsigned int sha1_rol(unsigned int value, unsigned int bits)
{
  return ((value) << bits) | (value >> (32 - bits));
}

struct sha1 {
  unsigned int state[5];
  unsigned int msg_size[2];
  size_t buf_used;
  unsigned char buffer[64];
};

static void sha1_hash_block(unsigned int state[5], const unsigned char *block)
{
  int i;
  unsigned int a, b, c, d, e;
  unsigned int w[80];

  /* Prepare message schedule */
  for (i = 0; i < 16; ++i)
    w[i] =
      (((unsigned int)block[(i*4)+0]) << 24) |
      (((unsigned int)block[(i*4)+1]) << 16) |
      (((unsigned int)block[(i*4)+2]) <<  8) |
      (((unsigned int)block[(i*4)+3]) <<  0);

  for (i = 16; i < 80; ++i)
    w[i] = sha1_rol(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);

  /* Initialize working variables */
  a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4];

  /* This is the core loop for each 20-word span. */
#define SHA1_LOOP(start, end, func, constant) \
  for (i = (start); i < (end); ++i) \
  { \
    unsigned int t = sha1_rol(a, 5) + (func) + e + (constant) + w[i]; \
    e = d; d = c; c = sha1_rol(b, 30); b = a; a = t; \
  }

  SHA1_LOOP( 0, 20, ((b & c) ^ (~b & d)),           0x5a827999)
  SHA1_LOOP(20, 40, (b ^ c ^ d),                    0x6ed9eba1)
  SHA1_LOOP(40, 60, ((b & c) ^ (b & d) ^ (c & d)),  0x8f1bbcdc)
  SHA1_LOOP(60, 80, (b ^ c ^ d),                    0xca62c1d6)

#undef SHA1_LOOP

  /* Update state */
  state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

static void sha1_init(struct sha1 *s)
{
  s->state[0] = 0x67452301;
  s->state[1] = 0xefcdab89;
  s->state[2] = 0x98badcfe;
  s->state[3] = 0x10325476;
  s->state[4] = 0xc3d2e1f0;
  s->msg_size[0] = 0;
  s->msg_size[1] = 0;
  s->buf_used = 0;
}

static void sha1_update(struct sha1 *s, const void *data_, size_t size)
{
  const char *data = (const char*) data_;
  unsigned int size_lo;
  unsigned int size_lo_orig;
  size_t remain = size;

  while (remain > 0)
  {
    size_t buf_space = (sizeof s->buffer) - s->buf_used;
    size_t copy_size = remain < buf_space ? remain : buf_space;
    memcpy(s->buffer + s->buf_used, data, copy_size);
    s->buf_used += copy_size;
    data += copy_size;
    remain -= copy_size;

    if (s->buf_used == sizeof s->buffer)
    {
      sha1_hash_block(s->state, s->buffer);
      s->buf_used = 0;
    }
  }

  size_lo = size_lo_orig = s->msg_size[1];
  size_lo += (unsigned int) (size * 8);
  
  if (size_lo < size_lo_orig)
    s->msg_size[0] += 1;

  s->msg_size[1] = size_lo;
}

static void sha1_final(unsigned char digest[20], struct sha1 *s)
{
  unsigned char zero = 0x00;
  unsigned char one_bit = 0x80;
  unsigned char count_data[8];
  int i;

  /* Generate size data in bit endian format */
  for (i = 0; i < 8; ++i)
  {
    unsigned int word = s->msg_size[i >> 2];
    count_data[i] = (unsigned char) (word >> ((3 - (i & 3)) * 8));
  }

  /* Set trailing one-bit */
  sha1_update(s, &one_bit, 1);

  /* Emit null padding to to make room for 64 bits of size info in the last 512 bit block */
  while (s->buf_used != 56)
    sha1_update(s, &zero, 1);

  /* Write size in bits as last 64-bits */
  sha1_update(s, count_data, 8);

  /* Make sure we actually finalized our last block */
  assert(0 == s->buf_used);

  /* Generate digest */
  for (i = 0; i < 20; ++i)
  {
    unsigned int word = s->state[i >> 2];
    unsigned char byte = (unsigned char) ((word >> ((3 - (i & 3)) * 8)) & 0xff);
    digest[i] = byte;
  }
}

static int discard_incoming_data(struct WebbyConnection* conn, int count)
{
  while (count > 0)
  {
    char buffer[1024];
    int read_size = count > (int) sizeof buffer ? (int) sizeof buffer : count;
    if (0 != WebbyRead(conn, buffer, (size_t) read_size))
      return -1;

    count -= read_size;
  }

  return 0;
}

const char *WebbyFindHeader(struct WebbyConnection *conn, const char *name)
{
  int i, count;
  for (i = 0, count = conn->request.header_count; i < count; ++i)
  {
    if (0 == strcasecmp(conn->request.headers[i].name, name))
    {
      return conn->request.headers[i].value;
    }
  }

  return NULL;
}


int
WebbyServerMemoryNeeded(const struct WebbyServerConfig *config)
{
  return
    WB_ALIGN_ARB(sizeof(struct WebbyServer), 16) +
    WB_ALIGN_ARB((config->connection_max - 1) * sizeof(struct WebbyConnectionPrv), 16) +
    config->connection_max * config->request_buffer_size +
    config->connection_max * config->io_buffer_size;
}

struct WebbyServer*
WebbyServerInit(struct WebbyServerConfig *config, void *memory, size_t memory_size)
{
  int i;
  struct WebbyServer *server = (struct WebbyServer*) memory;
  unsigned char *buffer = (unsigned char*) memory;

  memset(buffer, 0, memory_size);

  server->config = *config;
  server->memory_size = memory_size;
  server->socket = WB_INVALID_SOCKET;

  buffer +=
    WB_ALIGN_ARB(sizeof(struct WebbyServer), 16) +
    WB_ALIGN_ARB((config->connection_max - 1) * sizeof(struct WebbyConnectionPrv), 16);

  for (i = 0; i < config->connection_max; ++i)
  {
    server->connections[i].server = server;

    server->connections[i].header_buf.data = buffer;
    buffer += config->request_buffer_size;

    server->connections[i].io_buf.data = buffer;
    buffer += config->io_buffer_size;
  }

  assert((size_t)(buffer - (unsigned char*) memory) <= memory_size);

  server->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  dbg(server, "Server socket = %d", (int) server->socket);

  if (!wb_valid_socket(server->socket))
  {
    dbg(server, "failed to initialized server socket: %d", wb_socket_error());
    goto error;
  }

  {
    int on = 1;
    int off = 0;
    setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(int));
    setsockopt(server->socket, SOL_SOCKET, SO_LINGER, (const char*) &off, sizeof(int));
  }

#ifdef __APPLE__
  /* Don't generate SIGPIPE when writing to dead socket, we check all writes. */
  signal(SIGPIPE, SIG_IGN);
#endif

  if (0 != wb_set_blocking(server->socket, 0))
  {
    goto error;
  }

  {
    struct sockaddr_in bind_addr;

    dbg(server, "binding to %s:%d", config->bind_address, config->listening_port);

    memset(&bind_addr, 0, sizeof bind_addr); // use 0.0.0.0
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = inet_addr(config->bind_address);
    bind_addr.sin_port = htons((unsigned short) config->listening_port);

    if (0 != bind(server->socket, (struct sockaddr*) &bind_addr, sizeof bind_addr))
    {
      dbg(server, "bind() failed: %d", wb_socket_error());
      goto error;
    }
  }

  if (0 != listen(server->socket, SOMAXCONN))
  {
    dbg(server, "listen() failed: %d", wb_socket_error());
    wb_close_socket(server->socket);
    goto error;
  }

  dbg(server, "server initialized");
  return server;

error:
  if (wb_valid_socket(server->socket))
  {
    wb_close_socket(server->socket);
  }
  return NULL;
}

void WebbyServerShutdown(struct WebbyServer *srv)
{
  int i;
  wb_close_socket(srv->socket);

  for (i = 0; i < srv->connection_count; ++i)
  {
    wb_close_socket(srv->connections[i].socket);
  }

  memset(srv, 0, srv->memory_size);
}

static int wb_config_incoming_socket(webby_socket_t socket)
{
  int err;

  if (0 != (err = wb_set_blocking(socket, 0)))
  {
    return err;
  }

  {
    int off = 0;
    setsockopt(socket, SOL_SOCKET, SO_LINGER, (const char*) &off, sizeof(int));
  }

  return 0;
}

static void reset_connection(struct WebbyServer *srv, struct WebbyConnectionPrv *conn)
{
  conn->header_buf.used       = 0;
  conn->header_buf.max        = srv->config.request_buffer_size;
  conn->io_buf.used           = 0;
  conn->io_buf.max            = srv->config.io_buffer_size;
  conn->header_body_left      = 0;
  conn->io_data_left          = 0;
  conn->continue_data_left    = 0;
  conn->body_bytes_read       = 0;
  conn->state                 = WBC_REQUEST;
  conn->public_data.user_data = NULL;
  conn->blocking_count        = 0;
}

static int wb_on_incoming(struct WebbyServer *srv)
{
  int connection_index;
  char WB_ALIGN(8) client_addr[64];
  struct WebbyConnectionPrv* connection;
  webby_socklen_t client_addr_len = sizeof client_addr;
  webby_socket_t fd;

  /* Make sure we have space for a new connection */
  connection_index = srv->connection_count;

  if (connection_index == srv->config.connection_max)
  {
    dbg(srv, "out of connection slots");
    return 1;
  }

  /* Accept the incoming connection. */
  fd = accept(srv->socket, (struct sockaddr*) &client_addr[0], &client_addr_len);

  if (!wb_valid_socket(fd))
  {
    int err = wb_socket_error();
    if (!wb_is_blocking_error(err))
      dbg(srv, "accept() failed: %d", err);
    return 1;
  }

  connection = &srv->connections[connection_index];

  reset_connection(srv, connection);

  connection->flags       = WB_FRESH_CONNECTION;

  srv->connection_count = connection_index + 1;

  /* Configure socket */
  if (0 != wb_config_incoming_socket(fd))
  {
    wb_close_socket(fd);
    return 1;
  }

  /* OK, keep this connection */
  dbg(srv, "tagging connection %d as alive", connection_index);
  connection->flags |= WB_ALIVE;
  connection->socket = fd;
  return 0;
}

static int wb_peek_request_size(const unsigned char *buf, int len)
{
  int i;
  int max = len - 3;

  for (i = 0; i < max; ++i)
  {
    if ('\r' != buf[i]) 
      continue;

    if ('\n' != buf[i + 1])
      continue;

    if ('\r' != buf[i + 2])
      continue;

    if ('\n' != buf[i + 3])
      continue;

    /* OK; we have CRLFCRLF which indicates the end of the header section */
    return i + 4;
  }

  return -1;
}

static char* skipws(char *p)
{
  for (;;)
  {
    char ch = *p;
    if (' ' == ch || '\t' == ch)
      ++p;
    else
      break;
  }

  return p;
}

enum
{
  WB_TOK_SKIPWS = 1 << 0
};

static int tok_inplace(char *buf, const char* separator, char *tokens[], int max, int flags)
{
  int token_count = 0;
  char *b = buf;
  char *e = buf;
  int separator_len = (int) strlen(separator);

  while (token_count < max)
  {
    if (flags & WB_TOK_SKIPWS)
    {
      b = skipws(b);
    }

    if (NULL != (e = strstr(b, separator)))
    {
      int len = (int) (e - b);
      if (len > 0)
      {
        tokens[token_count++] = b;
      }
      *e = '\0';
      b = e + separator_len;
    }
    else
    {
      tokens[token_count++] = b;
      break;
    }
  }

  return token_count;
}

static void wb_close_client(struct WebbyServer *srv, struct WebbyConnectionPrv* connection)
{
  (void) srv;

  if (connection->socket != WB_INVALID_SOCKET)
  {
    wb_close_socket(connection->socket);
    connection->socket = WB_INVALID_SOCKET;
  }

  connection->flags = 0;
}

static int send_fully(webby_socket_t socket, const unsigned char *buffer, int size)
{
  while (size > 0)
  {
    int err = send(socket, (const char*) buffer, size, 0);

    if (err <= 0)
      return 1;

    buffer += err;
    size -= err;
  }

  return 0;
}


static int wb_setup_request(struct WebbyServer *srv, struct WebbyConnectionPrv *connection, int request_size)
{
  char* lines[WEBBY_MAX_HEADERS + 2];
  int line_count;
  char* tok[16];
  char* query_params;
  int tok_count;

  int i;
  int header_count;

  char *buf = (char*) connection->header_buf.data;
  struct WebbyRequest *req = &connection->public_data.request;

  /* Null-terminate the request envelope by overwriting the last CRLF with 00LF */
  buf[request_size - 2] = '\0';

  /* Split header into lines */
  line_count = tok_inplace(buf, "\r\n", lines, WB_ARRAY_SIZE(lines), 0);

  header_count = line_count - 2;

  if (line_count < 1 || header_count > (int) WB_ARRAY_SIZE(req->headers))
    return 1;

  /* Parse request line */
  tok_count = tok_inplace(lines[0], " ", tok, WB_ARRAY_SIZE(tok), 0);

  if (3 != tok_count)
    return 1;

  req->method = tok[0];
  req->uri = tok[1];
  req->http_version = tok[2];
  req->content_length = 0;

  /* See if there are any query parameters */
  if (NULL != (query_params = (char*) strchr(req->uri, '?')))
  {
    req->query_params = query_params + 1;
    *query_params = '\0';
  }
  else
    req->query_params = NULL;
  
  /* Decode the URI in place */
  {
    size_t uri_len = strlen(req->uri);
    url_decode(req->uri, uri_len, (char*) req->uri, uri_len + 1, /* url encoded: */ 1);
  }

  /* Parse headers */
  for (i = 0; i < header_count; ++i)
  {
    tok_count = tok_inplace(lines[i + 1], ":", tok, 2, WB_TOK_SKIPWS);

    if (tok_count != 2)
    {
      return 1;
    }

    req->headers[i].name = tok[0];
    req->headers[i].value = tok[1];

    if (0 == strcasecmp("content-length", tok[0]))
    {
      req->content_length = strtoul(tok[1], NULL, 10);
      dbg(srv, "request has body; content length is %d", req->content_length);
    }
    else if (0 == strcasecmp("transfer-encoding", tok[0]))
    {
      dbg(srv, "cowardly refusing to handle Transfer-Encoding: %s", tok[1]);
      return 1;
    }
  }

  req->header_count = header_count;

  return 0;
}

enum
{
  WB_FILL_OK,
  WB_FILL_ERROR,
  WB_FILL_FULL
};

/* Read as much as possible without blocking while there is buffer space. */
static int wb_fill_buffer(struct WebbyServer *srv, struct WebbyBuffer *buf, webby_socket_t socket)
{
  int err;
  int buf_left;

  for (;;)
  {
    buf_left = buf->max - buf->used;

    dbg(srv, "buffer space left = %d", buf_left);

    if (0 == buf_left)
    {
      return WB_FILL_FULL;
    }

    /* Read what we can into the current buffer space. */
    err = recv(socket, (char*) buf->data + buf->used, buf_left, 0);

    if (err < 0)
    {
      int sock_err = wb_socket_error();

      if (wb_is_blocking_error(sock_err))
      {
        return WB_FILL_OK;
      }
      else
      {
        /* Read error. Give up. */
        dbg(srv, "read error %d - connection dead", sock_err);
        return WB_FILL_ERROR;
      }
    }
    else if (err == 0)
    {
      /* The peer has closed the connection. */
      dbg(srv, "peer has closed the connection");
      return WB_FILL_ERROR;
    }
    else
    {
      buf->used += err;
    }
  }
}

static int is_websocket_request(struct WebbyConnection* conn)
{
  const char *hdr;

  if (NULL == (hdr = WebbyFindHeader(conn, "Connection")))
    return 0;

  //Firefox sends: "keep-alive, Upgrade"
  if (0 == strstr(hdr, "Upgrade"))
    return 0;

  if (NULL == (hdr = WebbyFindHeader(conn, "Upgrade")))
    return 0;

  if (0 != strcasecmp(hdr, "websocket"))
    return 0;

  return 1;
}

static int send_websocket_upgrade(struct WebbyServer *srv, struct WebbyConnectionPrv* connection)
{
  const char *hdr;
  struct sha1 sha;
  unsigned char digest[20];
  char output_digest[64];
  struct WebbyHeader headers[3];
  struct WebbyConnection *conn = &connection->public_data;

  if (0 == (srv->config.flags & WEBBY_SERVER_WEBSOCKETS))
  {
    dbg(srv, "websockets not enabled in server config");
    return 1;
  }

  if (NULL == (hdr = WebbyFindHeader(conn, "Sec-WebSocket-Version")))
  {
    dbg(srv, "Sec-WebSocket-Version header not present");
    return 1;
  }

  if (0 != strcmp(hdr, WB_WEBSOCKET_VERSION))
  {
    dbg(srv, "WebSocket version %s not supported (we only do %s)", hdr, WB_WEBSOCKET_VERSION);
    return 1;
  }

  if (NULL == (hdr = WebbyFindHeader(conn, "Sec-WebSocket-Key")))
  {
    dbg(srv, "Sec-WebSocket-Key header not present");
    return 1;
  }

  /* Compute SHA1 hash of Sec-Websocket-Key + the websocket guid as required by
   * the RFC.
   *
   * This handshake is bullshit. It adds zero security. Just forces me to drag
   * in SHA1 and create a base64 encoder.
   */
  sha1_init(&sha);
  sha1_update(&sha, hdr, (int) strlen(hdr));
  sha1_update(&sha, websocket_guid, websocket_guid_len);
  sha1_final(&digest[0], &sha);

  if (0 != base64_encode(output_digest, sizeof output_digest, &digest[0], sizeof digest))
    return 1;

  headers[0].name  = "Upgrade";
  headers[0].value = "websocket";
  headers[1].name  = "Connection";
  headers[1].value = "Upgrade";
  headers[2].name  = "Sec-WebSocket-Accept";
  headers[2].value = output_digest;

  WebbyBeginResponse(&connection->public_data, 101, 0, headers, WB_ARRAY_SIZE(headers));
  WebbyEndResponse(&connection->public_data);
  return 0;
}

static int scan_websocket_frame(const struct WebbyBuffer *buf, struct WebbyWsFrame *frame)
{
  unsigned char flags = 0;
  unsigned int len = 0;
  unsigned int opcode = 0;
  unsigned char* data = buf->data;
  unsigned char* data_max = data + buf->used;
  int i;
  int len_bytes = 0;
  int mask_bytes = 0;
  unsigned char header0, header1;

  if (buf->used < 2)
    return -1;

  header0 = *data++;
  header1 = *data++;

  if (header0 & 0x80)
  {
    flags |= WEBBY_WSF_FIN;
  }

  if (header1 & 0x80)
  {
    flags |= WEBBY_WSF_MASKED;
    mask_bytes = 4;
  }

  opcode = header0 & 0xf;
  len = header1 & 0x7f;

  if (len == 126)
    len_bytes = 2;
  else if (len == 127)
    len_bytes = 8;

  if (data + len_bytes + mask_bytes > data_max)
    return -1;

  /* Read big endian length from length bytes (if greater than 125) */
  len = len_bytes == 0 ? len : 0;
  for (i = 0; i < len_bytes; ++i)
  {
    /* This will totally overflow for 64-bit values. I don't care.
     * If you're transmitting more than 4 GB of data using Webby,
     * seek help. */
    len <<= 8;
    len |= *data++;
  }

  /* Read mask word if present */
  for (i = 0; i < mask_bytes; ++i)
  {
    frame->mask_key[i] = *data++;
  }

  frame->header_size = (unsigned char) (data - buf->data);
  frame->flags = flags;
  frame->opcode = (unsigned char) opcode;
  frame->payload_length = (int) len;
  return 0;
}

static void wb_update_client(struct WebbyServer *srv, struct WebbyConnectionPrv* connection)
{
   unsigned char* bufferBackup = connection->io_buf.data;
  /* This is no longer a fresh connection. Only read from it when select() says
   * so in the future. */
  connection->flags &= ~WB_FRESH_CONNECTION;

  for (;;)
  {
    switch (connection->state)
    {
      case WBC_REQUEST: {
        const char *expect_header;
        int result = wb_fill_buffer(srv, &connection->header_buf, connection->socket);
        int request_size;

        if (WB_FILL_ERROR == result)
        {
          connection->flags &= ~WB_ALIVE;
          return;
        }

        /* Scan to see if the buffer has a complete HTTP request header package. */ 
        request_size = wb_peek_request_size(connection->header_buf.data, connection->header_buf.used);

        dbg(srv, "peek request size: %d", request_size);
        if (request_size < 0)
        {
          /* Nothing yet. */
          if (connection->header_buf.max == connection->header_buf.used)
          {
            dbg(srv, "giving up as buffer is full");
            /* Give up, we can't fit the request in our buffer. */
            connection->flags &= ~WB_ALIVE;
          }
          return;
        }

        /* Set up request data. */
        if (0 != wb_setup_request(srv, connection, request_size))
        {
          dbg(srv, "failed to set up request");
          connection->flags &= ~WB_ALIVE;
          return;
        }

        /* Remember how much of the remaining buffer is body data. */
        connection->header_body_left = connection->header_buf.used - request_size;

        /* If the client expects a 100 Continue, send one now. */
        if (NULL != (expect_header = WebbyFindHeader(&connection->public_data, "Expect")))
        {
          if (0 == strcasecmp(expect_header, "100-continue"))
          {
            dbg(srv, "connection expects a 100 Continue header.. making him happy");
            connection->continue_data_left = (int) continue_header_len;
            connection->state = WBC_SEND_CONTINUE;
          }
          else
          {
            dbg(srv, "unrecognized Expected header %s", expect_header);
            connection->state = WBC_SERVE;
          }
        }
        else
        {
          connection->state = WBC_SERVE;
        }

        break;
      }

      case WBC_SEND_CONTINUE: {
        int left = connection->continue_data_left;
        int written = 0;

        written = send(connection->socket, continue_header + continue_header_len - left, left, 0);

        dbg(srv, "continue write: %d bytes", written);
        
        if (written < 0)
        {
          dbg(srv, "failed to write 100-continue header");
          connection->flags &= ~WB_ALIVE;
          return;
        }

        left -= written;
        connection->continue_data_left = left;

        if (0 == left)
        {
          connection->state = WBC_SERVE;
        }

        break;
      }

      case WBC_SERVE: {
        /* Clear I/O buffer for output */
        connection->io_buf.used = 0;

        /* Switch socket to blocking mode. */
        if (0 != make_connection_blocking(connection))
          return;

        /* Figure out if this is a request to upgrade to WebSockets */
        if (is_websocket_request(&connection->public_data))
        {
          dbg(srv, "received a websocket upgrade request");

          if (!srv->config.ws_connect || 0 != (*srv->config.ws_connect)(&connection->public_data))
          {
            dbg(srv, "user callback failed connection attempt");
            WebbyBeginResponse(&connection->public_data, 400, -1, plain_text_headers, WB_ARRAY_SIZE(plain_text_headers));
            WebbyPrintf(&connection->public_data, "WebSockets not supported at %s\r\n", connection->public_data.request.uri);
            WebbyEndResponse(&connection->public_data);
          }
          else
          {
            /* OK, let's try to upgrade the connection to WebSockets */
            if (0 != send_websocket_upgrade(srv, connection))
            {
              dbg(srv, "websocket upgrade failed");
              WebbyBeginResponse(&connection->public_data, 400, -1, plain_text_headers, WB_ARRAY_SIZE(plain_text_headers));
              WebbyPrintf(&connection->public_data, "WebSockets couldn't not be enabled\r\n");
              WebbyEndResponse(&connection->public_data);
            }
            else
            {
              /* OK, we're now a websocket */
              connection->flags |= WB_WEBSOCKET;
              dbg(srv, "connection %d upgraded to websocket", (int) (connection - srv->connections));

              (*srv->config.ws_connected)(&connection->public_data);
            }
          }
        }
        else if (0 != (*srv->config.dispatch)(&connection->public_data))
        {
          static const struct WebbyHeader headers[] =
          {
            { "Content-Type", "text/plain" },
          };
          WebbyBeginResponse(&connection->public_data, 404, -1, headers, WB_ARRAY_SIZE(headers));
          WebbyPrintf(&connection->public_data, "No handler for %s\r\n", connection->public_data.request.uri);
          WebbyEndResponse(&connection->public_data);
        }

        /* Back to non-blocking mode, can make the socket die. */
        make_connection_nonblocking(connection);

        /* Ready for another request, unless we should close the connection. */
        if (connection->flags & WB_ALIVE)
        {
          if (connection->flags & WB_CLOSE_AFTER_RESPONSE)
          {
            connection->flags &= ~WB_ALIVE;
            return;
          }
          else
          {
            /* Reset connection for next request. */
            reset_connection(srv, connection);

            if (0 == (connection->flags & WB_WEBSOCKET))
            {
              /* Loop back to request state */
              connection->state = WBC_REQUEST;
            }
            else
            {
              /* Clear I/O buffer for input */
              connection->io_buf.used = 0;
              /* Go to the web socket serving state */
              connection->state = WBC_WEBSOCKET;
            }
          }
        }

        break;
      }

      case WBC_WEBSOCKET: {

        /* In this state, we're trying to read a websocket frame into the I/O
         * buffer. Once we have enough data, we call the websocket frame
         * callback and let the client read the data through WebbyRead.
         */ 

        if (WB_FILL_ERROR == wb_fill_buffer(srv, &connection->io_buf, connection->socket))
        {
          /* Give up on this connection */
          connection->flags &= ~WB_ALIVE;
          return;
        }

        if (0 != scan_websocket_frame(&connection->io_buf, &connection->ws_frame))
        {
          /* Nothing yet */
          return;
        }

parseWebsocketFrame:
        connection->body_bytes_read = 0;
        connection->io_data_left = connection->io_buf.used - connection->ws_frame.header_size;
        dbg(srv, "%d bytes of incoming websocket data buffered", (int) connection->io_data_left);

        /* Switch socket to blocking mode */
        if (0 != make_connection_blocking(connection))
          return;

        switch (connection->ws_frame.opcode)
        {
          case WEBBY_WS_OP_CLOSE:
            dbg(srv, "received websocket close request");
            connection->flags &= ~WB_ALIVE;
            return;

          case WEBBY_WS_OP_PING:
            dbg(srv, "received websocket ping request");
            if (0 != send_fully(connection->socket, websocket_pong, sizeof websocket_pong))
            {
              connection->flags &= ~WB_ALIVE;
              return;
            }
            break;

          default:
            /* Dispatch frame to user handler. */
            if (0 != (*srv->config.ws_frame)(&connection->public_data, &connection->ws_frame))
            {
              connection->flags &= ~WB_ALIVE;
              return;
            }
        }

        /* Discard any data the client didn't read to retain the socket state. */
        if (connection->body_bytes_read < connection->ws_frame.payload_length)
        {
          int size = connection->ws_frame.payload_length - connection->body_bytes_read;
          if (0 != discard_incoming_data(&connection->public_data, size))
          {
            connection->flags &= ~WB_ALIVE;
            return;
          }
        }
        /* Back to non-blocking mode */
        if (0 != make_connection_nonblocking(connection))
          return;

        if(connection->io_data_left > 0)
        {
            int dataToSkip  = connection->ws_frame.header_size + connection->ws_frame.payload_length;
        
            connection->io_buf.data += dataToSkip;
            connection->io_buf.used -= dataToSkip;
            connection->io_buf.max  -= dataToSkip;
            connection->io_data_left-= dataToSkip;
            if (0 != scan_websocket_frame(&connection->io_buf, &connection->ws_frame))
            {
                goto parseWebsocketFrame;
            }
        }

        reset_connection(srv, connection);
        connection->io_buf.data = bufferBackup;
        connection->state = WBC_WEBSOCKET;

        break;
      }
    }
  }
}

void
WebbyServerUpdate(struct WebbyServer *srv)
{
  int i, count, err;
  webby_socket_t max_socket;
  fd_set read_fds, write_fds, except_fds;
  struct timeval timeout;

  /* Build set of sockets to check for events */
  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);
  FD_ZERO(&except_fds);

  max_socket = 0;

  /* Only accept incoming connections if we have space */
  if (srv->connection_count < srv->config.connection_max)
  {
    FD_SET(srv->socket, &read_fds);
    FD_SET(srv->socket, &except_fds);
    max_socket = srv->socket;
  }

  for (i = 0, count = srv->connection_count; i < count; ++i)
  {
    webby_socket_t socket = srv->connections[i].socket;
    FD_SET(socket, &read_fds);
    FD_SET(socket, &except_fds);

    if (srv->connections[i].state == WBC_SEND_CONTINUE)
      FD_SET(socket, &write_fds);

    if (socket > max_socket)
    {
      max_socket = socket;
    }
  }

  timeout.tv_sec = 0;
  timeout.tv_usec = 5;

  err = select((int) (max_socket + 1), &read_fds, &write_fds, &except_fds, &timeout);

  /* Handle incoming connections */
  if (FD_ISSET(srv->socket, &read_fds))
  {
    do
    {
      dbg(srv, "awake on incoming");
      err = wb_on_incoming(srv);
    } while (0 == err);
  }

  /* Handle incoming connection data */
  for (i = 0, count = srv->connection_count; i < count; ++i)
  {
    struct WebbyConnectionPrv *conn = &srv->connections[i];

    if (FD_ISSET(conn->socket, &read_fds) || FD_ISSET(conn->socket, &write_fds) || conn->flags & WB_FRESH_CONNECTION)
    {
      dbg(srv, "reading from connection %d", i);
      wb_update_client(srv, conn);
    }
  }

  /* Close stale connections & compact connection array. */
  for (i = 0; i < srv->connection_count; )
  {
    struct WebbyConnectionPrv *connection = &srv->connections[i];
    if (0 == (connection->flags & WB_ALIVE))
    {
      int remain;
      dbg(srv, "closing connection %d (%08x)", i, connection->flags);

      if (connection->flags & WB_WEBSOCKET)
      {
        (*srv->config.ws_closed)(&connection->public_data);
      }

      remain = srv->connection_count - i - 1;
      wb_close_client(srv, connection);
      memmove(&srv->connections[i], &srv->connections[i + 1], remain*sizeof(srv->connections[i]));
      --srv->connection_count;
    }
    else
    {
      ++i;
    }
  }
}

static int wb_flush(struct WebbyBuffer *buf, webby_socket_t socket)
{
  if (buf->used > 0)
  {
    if (0 != send_fully(socket, buf->data, buf->used))
      return 1;
  }
  buf->used = 0;
  return 0;
}

static int wb_push(struct WebbyServer *srv, struct WebbyConnectionPrv *conn, const void *data_, int len)
{
  struct WebbyBuffer *buf = &conn->io_buf;
  const unsigned char* data = (const unsigned char*) data_;

  if (conn->state != WBC_SERVE)
  {
    dbg(srv, "attempt to write in non-serve state");
    return 1;
  }

  if (0 == len)
  {
    return wb_flush(buf, conn->socket);
  }

  while (len > 0)
  {
    int buf_space = buf->max - buf->used;
    int copy_size = len < buf_space ? len : buf_space;
    memcpy(buf->data + buf->used, data, copy_size);
    buf->used += copy_size;

    data += copy_size;
    len -= copy_size;

    if (buf->used == buf->max)
    {
      if (0 != wb_flush(buf, conn->socket))
        return 1;

      if (len >= buf->max)
      {
        return send_fully(conn->socket, data, len);
      }
    }
  }

  return 0;
}

int WebbyPrintf(struct WebbyConnection* conn, const char* fmt, ...)
{
  int len;
  char buffer[1024];
  va_list args;

  va_start(args, fmt);
  len = vsnprintf(buffer, sizeof buffer, fmt, args);
  va_end(args);

  return WebbyWrite(conn, buffer, len);
}

static const short status_nums[] = {
   100, 101, 200, 201, 202, 203, 204, 205, 206, 300, 301, 302, 303, 304, 305,
   307, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413,
   414, 415, 416, 417, 500, 501, 502, 503, 504, 505
};

static const char* status_text[] = {
  "Continue", "Switching Protocols", "OK", "Created", "Accepted",
  "Non-Authoritative Information", "No Content", "Reset Content",
  "Partial Content", "Multiple Choices", "Moved Permanently", "Found",
  "See Other", "Not Modified", "Use Proxy", "Temporary Redirect", "Bad Request",
  "Unauthorized", "Payment Required", "Forbidden", "Not Found", "Method Not Allowed",
  "Not Acceptable", "Proxy Authentication Required", "Request Time-out", "Conflict",
  "Gone", "Length Required", "Precondition Failed", "Request Entity Too Large",
  "Request-URI Too Large", "Unsupported Media Type", "Requested range not satisfiable",
  "Expectation Failed", "Internal Server Error", "Not Implemented", "Bad Gateway",
  "Service Unavailable", "Gateway Time-out", "HTTP Version not supported"
};

static const char *wb_status_text(int status_code)
{
  int i;
  for (i = 0; i < (int) WB_ARRAY_SIZE(status_nums); ++i)
  {
    if (status_nums[i] == status_code)
      return status_text[i];
  }
  return "Unknown";
}

int WebbyBeginResponse(
    struct WebbyConnection *conn_pub,
    int status_code,
    int content_length,
    const struct WebbyHeader headers[],
    int header_count)
{
  int i = 0;
  struct WebbyConnectionPrv *conn = (struct WebbyConnectionPrv *) conn_pub;

  if (conn->body_bytes_read < conn->public_data.request.content_length)
  {
    int body_left = conn->public_data.request.content_length - conn->body_bytes_read;
    if (0 != discard_incoming_data(conn_pub, body_left))
    {
      conn->flags &= ~WB_ALIVE;
      return -1;
    }
  }

  WebbyPrintf(conn_pub, "HTTP/1.1 %d %s\r\n", status_code, wb_status_text(status_code));

  if (content_length >= 0)
  {
    WebbyPrintf(conn_pub, "Content-Length: %d\r\n", content_length);
  }
  else
  {
    WebbyPrintf(conn_pub, "Transfer-Encoding: chunked\r\n");
  }

  WebbyPrintf(conn_pub, "Server: Webby\r\n");

  for (i = 0; i < header_count; ++i)
  {
    if (0 == strcasecmp(headers[i].name, "Connection"))
    {
      if (0 == strcasecmp(headers[i].value, "close"))
      {
        conn->flags |= WB_CLOSE_AFTER_RESPONSE;
      }
    }
    WebbyPrintf(conn_pub, "%s: %s\r\n", headers[i].name, headers[i].value);
  }

  if (0 == (conn->flags & WB_CLOSE_AFTER_RESPONSE))
  {
    /* See if the client wants us to close the connection. */
    const char* connection_header = WebbyFindHeader(conn_pub, "Connection");
    if (connection_header && 0 == strcasecmp("close", connection_header))
    {
      conn->flags |= WB_CLOSE_AFTER_RESPONSE;
      WebbyPrintf(conn_pub, "Connection: close\r\n");
    }
  }

  WebbyPrintf(conn_pub, "\r\n");

  if (content_length < 0)
  {
    conn->flags |= WB_CHUNKED_RESPONSE;
  }

  return 0;
}

static size_t make_websocket_header(unsigned char buffer[10], unsigned char opcode, int payload_len, int fin)
{
  buffer[0] = (fin ? 0x80 : 0x00) | opcode;

  if (payload_len < 126)
  {
    buffer[1] = (unsigned char) (payload_len & 0x7f);
    return 2;
  }
  else if (payload_len < 65536)
  {
    buffer[1] = 126;
    buffer[2] = (unsigned char) (payload_len >> 8);
    buffer[3] = (unsigned char) payload_len;
    return 4;
  }
  else
  {
    buffer[1] = 127;
    /* Ignore high 32-bits. I didn't want to require 64-bit types and typdef hell in the API. */
    buffer[2] = buffer[3] = buffer[4] = buffer[5] = 0;
    buffer[6] = (unsigned char) (payload_len >> 24);
    buffer[7] = (unsigned char) (payload_len >> 16);
    buffer[8] = (unsigned char) (payload_len >> 8);
    buffer[9] = (unsigned char) payload_len;
    return 10;
  }
}

int
WebbyBeginSocketFrame(struct WebbyConnection *conn_pub, int opcode)
{
  struct WebbyConnectionPrv *conn = (struct WebbyConnectionPrv *) conn_pub;

  conn->ws_opcode = (unsigned char) opcode;

  /* Switch socket to blocking mode */
  return make_connection_blocking(conn);
}

int
WebbyEndSocketFrame(struct WebbyConnection *conn_pub)
{
  struct WebbyConnectionPrv *conn = (struct WebbyConnectionPrv *) conn_pub;
  unsigned char header[10];
  size_t header_size;

  header_size = make_websocket_header(header, conn->ws_opcode, 0, 1);

  if (0 != send_fully(conn->socket, header, (int) header_size))
    conn->flags &= ~WB_ALIVE;

  /* Switch socket to non-blocking mode */
  return make_connection_nonblocking(conn);
}

static int read_buffered_data(int *data_left, struct WebbyBuffer* buffer, char **dest_ptr, size_t *dest_len)
{
  int offset, read_size;
  int left = *data_left;
  int len;

  if (left == 0)
    return 0;

  len = (int) *dest_len;
  offset = buffer->used - left;
  read_size = len > left ? left : len;

  memcpy(*dest_ptr, buffer->data + offset, read_size);

  (*dest_ptr) += read_size;
  (*dest_len) -= (size_t) read_size;
  (*data_left) -= read_size;

  return read_size;
}

int WebbyRead(struct WebbyConnection *conn, void *ptr_, size_t len)
{
  struct WebbyConnectionPrv* conn_prv = (struct WebbyConnectionPrv*) conn;
  char *ptr = (char*) ptr_;
  int count;
  int start_pos = conn_prv->body_bytes_read;

  if (conn_prv->header_body_left > 0)
  {
    count = read_buffered_data(&conn_prv->header_body_left, &conn_prv->header_buf, &ptr, &len);
    conn_prv->body_bytes_read += count;
  }

  /* Read buffered websocket data */
  if (conn_prv->io_data_left > 0)
  {
    count = read_buffered_data(&conn_prv->io_data_left, &conn_prv->io_buf, &ptr, &len);
    conn_prv->body_bytes_read += count;
  }

  while (len > 0)
  {
    int err = recv(conn_prv->socket, ptr, (int) len, 0);

    if (err < 0)
    {
      conn_prv->flags &= ~WB_ALIVE;
      return err;
    }

    len -= err;
    ptr += err;
    conn_prv->body_bytes_read += err;
  }

  if ((conn_prv->flags & WB_WEBSOCKET) && (conn_prv->ws_frame.flags & WEBBY_WSF_MASKED))
  {
    /* XOR outgoing data with websocket ofuscation key */
    int i;
    int end_pos = conn_prv->body_bytes_read;
    const unsigned char *mask = conn_prv->ws_frame.mask_key;
    ptr = (char*) ptr_; /* start over */
    for (i = start_pos; i < end_pos; ++i)
    {
      unsigned char byte = *ptr;
      *ptr++ = byte ^ mask[i & 3];
    }
  }

  return 0;
}

int WebbyWrite(struct WebbyConnection *conn, const void *ptr, size_t len)
{
  struct WebbyConnectionPrv *conn_priv = (struct WebbyConnectionPrv *) conn;

  if (conn_priv->flags & WB_WEBSOCKET)
  {
    unsigned char header[10];
    size_t header_size;
    header_size = make_websocket_header(header, conn_priv->ws_opcode, (int) len, 0);

    /* Overwrite opcode to be continuation packages from here on out */
    conn_priv->ws_opcode = WEBBY_WS_OP_CONTINUATION;

    if (0 != send_fully(conn_priv->socket, header, (int) header_size))
    {
      conn_priv->flags &= ~WB_ALIVE;
      return -1;
    }
    if (0 != send_fully(conn_priv->socket, (const unsigned char*) ptr, (int) len))
    {
      conn_priv->flags &= ~WB_ALIVE;
      return -1;
    }
    return 0;
  }
  else if (conn_priv->flags & WB_CHUNKED_RESPONSE)
  {
    char chunk_header[128];
    int header_len = snprintf(chunk_header, sizeof chunk_header, "%x\r\n", (int) len);
    wb_push(conn_priv->server, conn_priv, chunk_header, header_len);
    wb_push(conn_priv->server, conn_priv, ptr, (int) len);
    return wb_push(conn_priv->server, conn_priv, "\r\n", 2);
  }
  else
  {
    return wb_push(conn_priv->server, conn_priv, ptr, (int) len);
  }
}

void WebbyEndResponse(struct WebbyConnection *conn)
{
  struct WebbyConnectionPrv *conn_priv = (struct WebbyConnectionPrv *) conn;

  if (conn_priv->flags & WB_CHUNKED_RESPONSE)
  {
    /* Write final chunk */
    wb_push(conn_priv->server, conn_priv, "0\r\n\r\n", 5);

    conn_priv->flags &= ~WB_CHUNKED_RESPONSE;
  }

  /* Flush buffers */
  wb_push(conn_priv->server, conn_priv, "", 0);
}

int
WebbySendFrame(struct WebbyConnection *conn_pub, int opcode, const void *ptr, size_t len)
{
  struct WebbyConnectionPrv *conn = (struct WebbyConnectionPrv *) conn_pub;
  unsigned char header[10];
  size_t header_size;

  /* Switch socket to blocking mode */
  make_connection_blocking(conn);

  header_size = make_websocket_header(header, (unsigned char)opcode, (int)len, 1);
  send_fully(conn->socket, header, (int)header_size);
  send_fully(conn->socket, ptr, (int)len);

  /* Switch socket to non-blocking mode */
  return make_connection_nonblocking(conn);
}
