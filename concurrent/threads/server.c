#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "../../headers/utils.h"

#define WAITING_FOR_MSG 0
#define IN_MESSAGE 1

void serve_connection(int sockfd);
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
  


void serve_connection(int sockfd) {

  int bytes_sent;

  bytes_sent = send(sockfd, "*", 1, 0);

  if (bytes_sent < 1) {
    perror("Error while sending '*'");
    return;
  }

  int state = WAITING_FOR_MSG;

  while(1) {
    char buf[1024];
    int bytes_received = recv(sockfd, buf, 1024, 0);

    if (bytes_received < 0) {
      perror("Error recv(): ");
      return;
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
            return;
          }
        }
      }
    }
  }

  close(sockfd);
}
