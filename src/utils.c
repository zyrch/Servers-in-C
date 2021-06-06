#include <string.h> 
#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>

#include "../headers/utils.h"


#define N_BACKLOG 20
#define WAITING_FOR_MSG 0
#define IN_MESSAGE 1

void serve_connection(int sockfd) {

  int bytes_sent;

  bytes_sent = send(sockfd, "*", 1, 0);

  if (bytes_sent < 1) {
    perror("Error while sending '*'");
    // TODO: use errno
    exit(-1);
  }

  int state = WAITING_FOR_MSG;

  while(1) {
    char buf[1024];
    int bytes_received = recv(sockfd, buf, 1024, 0);

    if (bytes_received < 0) {
      perror("Error recv(): ");
      exit(-1);
    }else if(bytes_received == 0) {
      break;
    }

    for (int i = 0; i < bytes_received; ++i) {
      if (state == WAITING_FOR_MSG) {
        if (buf[i] == '^') {
          state = IN_MESSAGE;
        }
      }else {
        if (buf[i] == '$') {
          state = WAITING_FOR_MSG;
        }else {
          buf[i] += 1;
          bytes_sent = send(sockfd, &buf[i], 1, 0);
          if (bytes_sent < 0) {
            perror("Error in In Message: ");
            exit(-1);
          }
        }
      }
    }
  }

  close(sockfd);
}

void print_peer_info(const struct sockaddr_in* sa, socklen_t salen) {
  // NI_MAXHOST = 1025 and NI_MAXSERV = 32, defined in <netdb.h>
  char hostbuf[NI_MAXHOST];
  char portbuf[NI_MAXSERV];

  if (getnameinfo((struct sockaddr*)sa, salen, hostbuf, NI_MAXHOST, portbuf, NI_MAXSERV, 0) == 0) {
    printf("peer (%s, %s) connected\n", hostbuf, portbuf);
  } else {
    printf("peer (unknonwn) connected\n");
  }
}

int listen_inet_socket(int portnum) {
  
  // family   : AF_INET      -- IPv4 Internet protocols 
  // type     : SOCK_STREAM -- communication semantics
  // protocol : 0           -- default
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("Error: ");
    return -1;
  }

  int yes = 1;

  // Clear the port to prevent "address in use" error message 
  // SOL_SOCKET   -- Protocol levet for option is socket level
  // SO_REUSEADDR -- Reuse of addresses in bind()
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    perror("setsockopt");
    return -1;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portnum);
  // Bind to all available interfaces
  // User `sin_addr.s_addr = inet_addr("127.0.0.1");` to bind to localhost
  server_addr.sin_addr.s_addr = INADDR_ANY;
  
  if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error: ");
    return -1;
  }

  // N_BACKLOG -- number of connections allowed on the incoming queue
  if (listen(sockfd, N_BACKLOG) < 0) {
    perror("Error: ");
    return -1;
  }

  return sockfd;

}

void make_socket_non_blocking(int sockfd) {

  int flags = fcntl(sockfd, F_GETFL, 0);  

  if (flags == -1) {
    perror("fnctl :");
    exit(-1);
  }

  flags |= O_NONBLOCK;
  
  if (fcntl(sockfd, F_SETFL, flags) < 0) {
    perror("fnctl :");
    exit(-1);
  }

}


