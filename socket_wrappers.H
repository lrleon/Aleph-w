# ifndef SOCKET_WRAPPERS_H
# define SOCKET_WRAPPERS_H


extern "C" {

# include <sys/types.h>	        /* basic system data types */


ssize_t readn(int, void *, size_t);
ssize_t writen(int, const void *, size_t);

ssize_t flushn(int, size_t);

}

# endif // SOCKET_WRAPPERS_H





