#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>

#include "../../../headers/utils.h"

#define BUFF_SIZE 1024
#define MAXFDS 1024

int CUR_MAXFDS;

typedef enum { WAIT_FOR_MSG, IN_MSG, IN_ACK } ProcessingState;

typedef struct {
  bool want_read;
  bool want_write;
} fd_status_t;

typedef struct {
  char write_buf[BUFF_SIZE];
  int write_buf_len;
  int write_buf_ptr; 
  ProcessingState pState;
  fd_status_t fd_status;
} fd_state_t;

fd_state_t fd_state[MAXFDS];

fd_status_t fd_status_R = (fd_status_t) {.want_read = true, .want_write = false};
fd_status_t fd_status_W = (fd_status_t) {.want_read = false, .want_write = true};
fd_status_t fd_status_RW = (fd_status_t) {.want_read = true, .want_write = true};

void client_recv(int sockfd) {
  
  assert(sockfd < MAXFDS);

  fd_state_t *client_state = &fd_state[sockfd];
  
  if (client_state->pState == IN_ACK) {
    client_state->fd_status = fd_status_R;
    return;
  }

  char buf[BUFF_SIZE];
  int bytes_received = recv(sockfd, buf, sizeof buf, 0); 

  if (bytes_received < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // temporarily unavailable
      client_state->fd_status = fd_status_R;
      return;
    }
    perror("Error recv(): ");
    exit(-1);

  }else if (bytes_received == 0) {
    // connection closed
    client_state->pState = IN_ACK;
    client_state->fd_status = fd_status_R;
    return;
  }

  bool ready_to_send = false;
  for (int i = 0; i < bytes_received; ++i) {
    if (client_state->pState == WAIT_FOR_MSG) {
      if (buf[i] == '^') {
        client_state->pState = IN_MSG;
      }
    }else if (buf[i] == '$') {
      client_state->pState = WAIT_FOR_MSG;
    }else {
      assert(client_state->write_buf_len < BUFF_SIZE);
      client_state->write_buf[client_state->write_buf_len++] = buf[i];
      ready_to_send = true; 
    }
  }

  // TODO: why this can't we do both
  client_state->fd_status = (fd_status_t){.want_read = !ready_to_send,
                                          .want_write = ready_to_send};
}


void client_send(int sockfd) {
  assert(sockfd < MAXFDS);
  fd_state_t *client_state = &fd_state[sockfd];

  if (client_state->write_buf_ptr == client_state->write_buf_len) {
    client_state->fd_status = fd_status_RW;
    return;
  }
  
  int sendlen = client_state->write_buf_len - client_state->write_buf_ptr;
  int bytes_sent = send(sockfd, client_state->write_buf + client_state->write_buf_ptr, sendlen, 0);
  
  if (bytes_sent < 0) {
    // TODO: why wasn't this done in previous versions
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // temporarily unavailabe
      client_state->fd_status = fd_status_R;
    } else {
      perror("Error send:");
      return;
    }
  }

  if (bytes_sent == sendlen) {
    client_state->write_buf_ptr = 0;
    client_state->write_buf_len = 0;

    if (client_state->pState == IN_ACK) 
      client_state->pState = WAIT_FOR_MSG;

    client_state->fd_status = fd_status_R;
  }else {
    client_state->write_buf_ptr += bytes_sent;
    client_state->fd_status = fd_status_W;
  }
}


void client_connect(int sockfd) {
  
  assert(sockfd < MAXFDS);

  fd_state_t *client_state = &fd_state[sockfd];

  client_state->pState = IN_ACK;
  client_state->write_buf_ptr = 0;
  client_state->write_buf_len = 0;
  client_state->fd_status = fd_status_W;

  struct sockaddr_in peer_addr;
  socklen_t peer_addr_size = sizeof(peer_addr);

  int new_fd = accept(sockfd, (struct sockaddr *)&peer_addr, &peer_addr_size);

  if (new_fd < 0) {
    perror("Error: ");
    return;
  }
  print_peer_info(&peer_addr, peer_addr_size);
}

void update_fds(fd_set *read, fd_set *write, int fd) {
  if (fd_state[fd].fd_status.want_read) {
    FD_SET(fd, read);
  }else {
    FD_CLR(fd, read);
  }

  if (fd_state[fd].fd_status.want_write) {
    FD_SET(fd, write);
  }else {
    FD_CLR(fd, write);
  }
}


int main(int argc, char **argv) {
  
  // listen_inet_socket exits if there is an error
  int sockfd = listen_inet_socket(8000);

  printf("socket file descriptor : %d\n", sockfd);

  fd_set global_readfds;
  fd_set global_writefds;

  FD_ZERO(&global_readfds);
  FD_ZERO(&global_writefds);

  FD_SET(sockfd, &global_readfds);
  CUR_MAXFDS = sockfd;

  while (1) {

    // since select() changes the fd_set passed to it, copies
    // are made to preserve the global_* fd_set
    fd_set local_readfds = global_readfds;
    fd_set local_writefds = global_writefds;

    // Timeout -- NULL - One return if a change is observed
    // returns number of modified descriptors
    int modified = select(CUR_MAXFDS + 1, &local_readfds, &local_writefds, NULL, NULL);

    if (modified == 0) {
      perror("Select: ");
      return -1;
    }

    for (int fd = 0; fd < CUR_MAXFDS + 1 && modified; ++fd) {

      if (FD_ISSET(fd, &local_readfds)) {
        --modified;
        if (fd == sockfd) {
          // New Client connect
          client_connect(sockfd);
        }else {
          // receive from client and switch set to write
          client_recv(sockfd);
        }
        update_fds(&global_readfds, &global_writefds, fd);
      }

      if (FD_ISSET(fd, &local_writefds)){
        --modified;
        // write to client and switch set
        client_send(sockfd);
        update_fds(&global_readfds, &global_writefds, fd);
      }
    }
  }

  close(sockfd);
}
