
#ifndef UTILS_H
#define UTILS_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

//  Creates and binds a listening SOCK_STREAM socket of domain AF_INET
int listen_inet_socket(int portnum);

#endif // UTILS_H
