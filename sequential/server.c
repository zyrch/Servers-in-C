#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../headers/utils.h"


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
    serve_connection(new_fd);

    printf("Connection with Peer Closed\n");
    fflush(stdout);

  }

  close(sockfd);
}
