/************************************************************************************

Bridge to allow aarch64 to compile with fewer assumptions and inclusions within the 
compilation requirements and property files. The definitions below exist in 
different files based on whether or not you include generic platform files or the
architecture specific files. 

The assumption is that generic files will be generally more compatible than the 
architecture specific ones, but they are missing some definitions. However, the
architecture specific ones technically deviate in specific signature, but are
effectively the same for this platform.

************************************************************************************/
#if defined(__clang__)

typedef int pid_t;
#define __DEFINED_pid_t
typedef unsigned id_t;
#define __DEFINED_id_t
typedef unsigned uid_t;
#define __DEFINED_uid_t
typedef unsigned gid_t;
#define __DEFINED_gid_t
typedef uint8_t sa_family_t;
#ifndef __DEFINED_sa_family_t
#define __DEFINED_sa_family_t // BBI-HACK: the compiler will need to pass this define through as though defined for other projects that are included to avoid a different definition for the typedef
#endif
typedef unsigned socklen_t;
#define __DEFINED_socklen_t

#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

__BEGIN_DECLS

int socket(int, int, int);
int socketpair(int, int, int, int[2]);

int shutdown(int, int);

int bind(int, const struct sockaddr *, socklen_t);
int connect(int, const struct sockaddr *, socklen_t);
int listen(int, int);
int accept(int, struct sockaddr *__restrict, socklen_t *__restrict);
int accept4(int, struct sockaddr *__restrict, socklen_t *__restrict, int);

int getsockname(int, struct sockaddr *__restrict, socklen_t *__restrict);
int getpeername(int, struct sockaddr *__restrict, socklen_t *__restrict);

ssize_t send(int, const void *, size_t, int);
ssize_t recv(int, void *, size_t, int);
ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
ssize_t recvfrom(int, void *__restrict, size_t, int, struct sockaddr *__restrict, socklen_t *__restrict);
ssize_t sendmsg(int, const struct msghdr *, int);
ssize_t recvmsg(int, struct msghdr *, int);

int getsockopt(int, int, int, void *__restrict, socklen_t *__restrict);
int setsockopt(int, int, int, const void *, socklen_t);

int sockatmark(int);

__END_DECLS

#endif // clang
