#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "../../headers/utils.h"
#include "../../headers/thpool.h"

#define N_THREADS 4

void server_thread(void *arg);

struct thread_config{
  int sockfd;
};

int main(int argc, char **argv) {
  
  int sockfd = listen_inet_socket(8000);

  printf("socket file descriptor : %d\n", sockfd);

  threadpool thpool = thpool_init(N_THREADS);

  if (thpool == NULL) {
    puts("Error While Creating Thread Pool\n");
    return -1;
  }

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

    if (thpool_add_work(thpool, server_thread, &config) < 0) {
      puts("Error Occured while adding work to thread pool\n");
    }
  }

  thpool_wait(thpool);

  close(sockfd);
}

void server_thread(void *arg) {
  
  int sockfd = ((struct thread_config*)arg)->sockfd;

  serve_connection(sockfd);

}
  

