///************************************************************************************
//
//Bridge to allow aarch64 to compile with fewer assumptions and inclusions within the 
//compilation requirements and property files. The definitions below exist in 
//different files based on whether or not you include generic platform files or the
//architecture specific files. 
//
//The assumption is that generic files will be generally more compatible than the 
//architecture specific ones, but they are missing some definitions. However, the
//architecture specific ones technically deviate in specific signature, but are
//effectively the same for this platform.
//
//************************************************************************************/
#if defined(__clang__)

#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

__BEGIN_DECLS

int socket(int, int, int);
int bind(int, const struct sockaddr *, socklen_t);
int listen(int, int);
int accept(int, struct sockaddr *__restrict, socklen_t *__restrict);
ssize_t send(int, const void *, size_t, int);
ssize_t recv(int, void *, size_t, int);
int setsockopt(int, int, int, const void *, socklen_t);

__END_DECLS

#endif // clang
