#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>

#include "../../headers/utils.h"

#define BUFF_SIZE 1024
#define MAXFDS 100000

int CUR_MAXFDS;

typedef enum { WAIT_FOR_MSG, IN_MSG, IN_ACK } ProcessingState;

typedef struct {
  bool want_read;
  bool want_write;
} fd_status_t;

// Stores the state of a file descriptor
// write_buf     -- Buffer of data needs to be send
// write_buf_len -- length of valid buf
// write_buf_ptr -- ptr to next find to send
typedef struct {
  char write_buf[BUFF_SIZE];
  int write_buf_len;
  int write_buf_ptr; 
  ProcessingState pState;
  fd_status_t fd_status;
} fd_state_t;

// After a file descriptor is closed data is overwritten by the new file descriptor
fd_state_t fd_state[MAXFDS];

// fd_status_t struct of each type
fd_status_t fd_status_R = (fd_status_t) {.want_read = true, .want_write = false};
fd_status_t fd_status_W = (fd_status_t) {.want_read = false, .want_write = true};
fd_status_t fd_status_RW = (fd_status_t) {.want_read = true, .want_write = true};
fd_status_t fd_status_NRW = (fd_status_t) {.want_read = false, .want_write = false};

void client_recv(int sockfd) {
  
  // receives data from sockfd 
  // updates the states of fd_state_t accordingly
  // if EAGAIN or EWOULDBLOCK is returns it does nothing
  
  assert(sockfd < MAXFDS);

  fd_state_t *client_state = &fd_state[sockfd];
  
  // Cannot recieve data util ACK has been sent to client
  if (client_state->pState == IN_ACK) {
    client_state->fd_status = fd_status_W;
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
    puts("connection closed");
    // connection closed
    client_state->pState = IN_ACK;
    client_state->fd_status = fd_status_NRW;
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
      client_state->write_buf[client_state->write_buf_len++] = buf[i] + 1;
      ready_to_send = true; 
    }
  }

  // TODO: why this can't we do both
  client_state->fd_status = (fd_status_t){.want_read = !ready_to_send,
                                          .want_write = ready_to_send};
}


void client_send(int sockfd) {

  // sends data from fd_state[sockfd].write_buf to sockfd, 
  // updates the states of fd_state_t accordingly
  // if EAGAIN or EWOULDBLOCK is returns it does nothing
  assert(sockfd < MAXFDS);
  fd_state_t *client_state = &fd_state[sockfd];

  if (client_state->write_buf_ptr == client_state->write_buf_len) {
    // if buffer is full sent
    // both write_buf_ptr and write_buf_len should be zero
    client_state->fd_status = fd_status_RW;
    return;
  }
  
  int sendlen = client_state->write_buf_len - client_state->write_buf_ptr;
  int bytes_sent = send(sockfd, client_state->write_buf + client_state->write_buf_ptr, sendlen, 0);
  
  if (bytes_sent < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // temporarily unavailable
      client_state->fd_status = fd_status_R;
      return;
    } else {
      perror("Error send:");
      exit(-1);
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


int client_connect(int sockfd) {
  
  assert(sockfd < MAXFDS);

  struct sockaddr_in peer_addr;
  socklen_t peer_addr_size = sizeof(peer_addr);

  int new_fd = accept(sockfd, (struct sockaddr *)&peer_addr, &peer_addr_size);
  make_socket_non_blocking(new_fd);

  assert(new_fd < MAXFDS);

  if (new_fd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      printf("accept returns EAGAIN or EWOULDBLOCK\n");
      return -1;
    }else {
      perror("Error: ");
      exit(-1);
    }
  }

  if (new_fd > CUR_MAXFDS) {
    CUR_MAXFDS = new_fd;
  }

  print_peer_info(&peer_addr, peer_addr_size);
  
  fd_state_t *client_state = &fd_state[new_fd];

  client_state->pState = IN_ACK;
  client_state->write_buf_ptr = 0;
  client_state->write_buf_len = 0;
  client_state->fd_status = fd_status_W;

 
  client_state->write_buf[0] = '*';
  client_state->write_buf_len = 1;


  return new_fd;
}

void update_fds(int epfd, int fd) {
  
  struct epoll_event new_event;
  new_event.data.fd = fd;

  int cnt = 0;

  if (fd_state[fd].fd_status.want_read) {
    ++cnt;
    new_event.events |= EPOLLIN;
  }

  if (fd_state[fd].fd_status.want_write) {
    ++cnt;
    new_event.events |= EPOLLOUT;
  }

  if (!cnt) {
    // In most cases this step is not necessary
    // discriptor is automatically deleted from epoll after closing
    // see man epoll Q6 for more details
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
      perror("epoll_ctl DEL");
      exit(-1);
    }
    close(fd);
    return;
  }

  if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &new_event) < 0) {
    perror("epoll_ctl MOD");
  }
}

int main(int argc, char **argv) {
  
  // listen_inet_socket exits if there is an error
  int sockfd = listen_inet_socket(8000);
  make_socket_non_blocking(sockfd);

  printf("socket file descriptor : %d\n", sockfd);

  int epfd = epoll_create1(0);

  // define an event from file descriptor to be watched by epoll
  struct epoll_event accept_event;
  accept_event.data.fd = sockfd;
  // only in events for listening socket
  accept_event.events = EPOLLIN;

  // add epoll_event to epfd, epoll will now watch the file descriptor defined by epoll_event
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &accept_event) < 0) {
    perror("Adding with epoll_clt");
    return -1;
  }
  
  // allocated memory of events from epoll_wait
  struct epoll_event* events = calloc(MAXFDS, sizeof(struct epoll_event));
  if (events == NULL) {
    perror("Unable to allocate memory for epoll_events");
    return -1;
  }

  while (1) {
    // Timeout -- -1 - One return if a change is observed
    // returns number of modified descriptors
    int modified = epoll_wait(epfd, events, MAXFDS, -1);

    if (modified < 0) {
      perror("Select: ");
      return -1;
    }

    for (int i = 0; i < modified; ++i) {
      // Check if error occured in epoll
      if (events[i].events & EPOLLERR) {
        perror("epoll_wait returned EPOLLERR");
        return -1;
      }

      if (events[i].events & EPOLLIN) {
        // IN event 
        if (events[i].data.fd == sockfd) {

          int new_fd = client_connect(events[i].data.fd);

          struct epoll_event new_event;
          new_event.data.fd = new_fd;
          new_event.events = EPOLLOUT;

          if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &new_event) < 0) {
            perror("Adding with epoll_ctl");
            return -1;
          }
        }else {
          client_recv(events[i].data.fd);
          update_fds(epfd, events[i].data.fd);
        }
      }else if (events[i].events & EPOLLOUT) {
        // OUT event
        client_send(events[i].data.fd);
        update_fds(epfd, events[i].data.fd);
      }

    }
  }

  close(sockfd);
}
