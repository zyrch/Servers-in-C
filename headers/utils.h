
#ifndef UTILS_H
#define UTILS_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>



// Creates and binds a listening SOCK_STREAM socket of domain AF_INET
int listen_inet_socket(int portnum);

// Prints info about client connected using struct returned by accept() 
void print_peer_info(const struct sockaddr_in* sa, socklen_t salen);

void serve_connection(int sockfd);

#endif // UTILS_H
