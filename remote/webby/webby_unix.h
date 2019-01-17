
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

typedef int webby_socket_t;
typedef socklen_t webby_socklen_t;

#define WB_ALIGN(x) __attribute__((aligned(x)))
#define WB_INVALID_SOCKET (-1)

static int wb_socket_error(void)
{
  return errno;
}

static int wb_valid_socket(webby_socket_t socket)
{
  return socket > 0;
}

static void wb_close_socket(webby_socket_t socket)
{
  close(socket);
}

static int wb_is_blocking_error(int error)
{
  return EAGAIN == error;
}

static int wb_set_blocking(webby_socket_t socket, int blocking)
{
  int flags = fcntl(socket, F_GETFL);
  if (blocking)
    return fcntl(socket, F_SETFL, flags & ~O_NONBLOCK);
  else
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}
