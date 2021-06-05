#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "../../utils/utils.h"

#define WAITING_FOR_MSG 0
#define IN_MESSAGE 1

void* server_thread(void *arg);

struct thread_config{
  int sockfd;
};

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

    pthread_t thread;

    struct thread_config config;
    config.sockfd = new_fd;

    if (pthread_create(&thread, NULL, server_thread, &config) < 0) {
      perror("Pthread_Create: ");
      return -1;
    }

    pthread_detach(thread);

  }

  close(sockfd);
}

void* server_thread(void *arg) {
  
  int sockfd = ((struct thread_config*)arg)->sockfd;

  printf("Thread started, socket %d\n", sockfd);
  serve_connection(sockfd);
  printf("Connection with Peer Closed, sockfd %d\n", sockfd);

  return 0;
}
  
