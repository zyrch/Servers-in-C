#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../utils/utils.h"

#define WAITING_FOR_MSG 0
#define IN_MESSAGE 1

void serve_conncetion(int sockfd) {

  int bytes_sent;

  bytes_sent = send(sockfd, "*", 1, 0);

  if (bytes_sent < 1) {
    perror("Error while sending '*'");
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
    

int main(int argc, char **argv) {
  
  int sockfd = listen_inet_socket(8000);

  printf("socket file descriptor : %d\n", sockfd);

  while (1) {

    struct sockaddr_in peer_addr;
    socklen_t peer_addr_size = sizeof(peer_addr);

    int new_fd = accept(sockfd, (struct sockaddr *)&peer_addr, &peer_addr_size);

    if (new_fd < 0) {
      perror("Error: ");
      return -1;
    }

    print_peer_info(&peer_addr, peer_addr_size);
    serve_conncetion(new_fd);

    printf("Connection with Peer Closed");
    fflush(stdout);

  }

  close(sockfd);
}
