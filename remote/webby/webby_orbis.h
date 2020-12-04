#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <net.h>
#include <libnet/socket.h>


typedef int webby_socket_t;
typedef socklen_t webby_socklen_t;

#define SOMAXCONN 16

#define WB_ALIGN(x) __attribute__((aligned(x)))
#define WB_INVALID_SOCKET (-1)

static int wb_socket_error(void)
{
    return errno;
}

static int wb_valid_socket(webby_socket_t socket)
{
    return socket >= 0;
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
    int ret = -1;
    if (blocking) {
        int opt_val = 0; // set non-blocking to false
        ret = sceNetSetsockopt(socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &opt_val, sizeof(opt_val));
    } else {
        int opt_val = 1; // set non-blocking to true
        ret = sceNetSetsockopt(socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &opt_val, sizeof(opt_val));
    }
    //auto err = sce_net_errno;
    return ret;
}