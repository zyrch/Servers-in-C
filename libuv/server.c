#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <uv.h>

#include "../headers/utils.h"

#define N_BACKLOG 64
#define BUFF_SIZE 1024

void on_client_closed(uv_handle_t* handle);
void new_client_connect(uv_stream_t* server_stream, int status);
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf);
void on_write_ack(uv_stream_t *req, int status);
void client_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
void client_write(uv_write_t* handle, int status);

typedef enum { WAIT_FOR_MSG, IN_MSG, IN_ACK } ProcessingState;

// Stores the state of a file descriptor
// write_buf     -- Buffer of data needs to be send
// write_buf_len -- length of valid buf
// write_buf_ptr -- ptr to next find to send
typedef struct {
  char write_buf[BUFF_SIZE];
  int write_buf_len;
  ProcessingState pState;
  uv_tcp_t* client;
} client_state_t;

int main(int argc, char **argv) {
  
  int portnum = 8000;

  uv_tcp_t server_stream;

  // initializes the tcp handle
  int status = uv_tcp_init(uv_default_loop(), &server_stream);

  if (status) {
    printf("Error uv_tcp_init() : %s\n", uv_strerror(status));
    return -1;
  }

  struct sockaddr_in server_address;
  // convert a string contaiging IPv4 addresses to sockaddr_in struct
  status = uv_ip4_addr("0.0.0.0", portnum, &server_address);

  if (status) {
    printf("Error uv_ip4_addr() : %s\n", uv_strerror(status));
    return -1;
  }

  // bind tcp handle to socket
  status = uv_tcp_bind(&server_stream, (struct sockaddr *)&server_address, 0);

  if (status) {
    printf("Error up_tcp_bind() : %s\n", uv_strerror(status));
    return -1;
  }

  // register a callback that registers whenever a new clients tries to make connection
  status = uv_listen((uv_stream_t *)&server_stream, N_BACKLOG, new_client_connect); 
  if (status) {
    printf("Error uv_listen() : %s\n", uv_strerror(status));
    return -1;
  }

  // UV_RUN_DEFAULT -- runs the event loop until there are no more active handles
  // returns number of pending handles or requests
  int pending = uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  if (pending) {
    printf("Defualt loop closed with %d pending requests or handles", pending);
    return -1;
  }

  uv_loop_close(uv_default_loop());
}

void on_client_closed(uv_handle_t* handle) {
  uv_tcp_t* client = (uv_tcp_t*)handle;

  if (client->data) {
    free(client->data);
  }
  free(client);
}

void new_client_connect(uv_stream_t* server_stream, int status) {

  if (status < 0) {
    printf("Client Connection Failed: %s", uv_strerror(status));
    return;
  }

  uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(*client));
  if (client == NULL) {
    perror("malloc:");
    exit(-1);
  }

  status = uv_tcp_init(uv_default_loop(), client);

  if (status < 0) {
    printf("Error uv_tcp_init() : %s\n", uv_strerror(status));
    exit(-1);
  }

  client->data = NULL;

  // guaranteed to succed once per call back
  status = uv_accept(server_stream, (uv_stream_t*)client);
  
  if (status < 0) {
    printf("Error uv_accept()  : %s\n", uv_strerror(status));
    // callback to free the data on heap
    uv_close((uv_handle_t*)client, on_client_closed);
  }else {

    struct sockaddr_storage peername;
    int namelen = sizeof(peername);
    if ((status = uv_tcp_getpeername(client, (struct sockaddr*)&peername, &namelen)) < 0) {
      printf("uv_tcp_getpeername failed: %s", uv_strerror(status));
      exit(-1);
    }

    print_peer_info((const struct sockaddr_in*)&peername, namelen);


    client_state_t *client_state = (client_state_t*)malloc(sizeof(*client_state));
    if (client_state == NULL) {
      perror("malloc:");
      exit(-1);
    }

    client_state->pState = IN_ACK;
    client_state->write_buf[0] = '*';
    client_state->write_buf_len = 1;
    client_state->client = client;

    client->data = client_state;
  
    // don't need wrt_len_ptr here, libuv takes care of that
    uv_buf_t writebuf = uv_buf_init(client_state->write_buf, client_state->write_buf_len);

    uv_write_t* req = (uv_write_t*)malloc(sizeof(*req));
    if (req == NULL) {
      perror("malloc:");
      exit(-1);
    }

    req->data = client_state;

    status = uv_write((uv_write_t*)req, (uv_stream_t*)client, &writebuf, 1, (uv_write_cb)on_write_ack);
    if (status < 0) {
      printf("Error uv_write: %s", uv_strerror(status));
      exit(-1);
    }
  }
}


void on_write_ack(uv_stream_t *req, int status) {
  if (status < 0) {
    perror("Write Error");
    exit(-1);
  }

  // reset buff
  client_state_t *client_state = (client_state_t *)req->data;
  client_state->pState = WAIT_FOR_MSG;

  status = uv_read_start((uv_stream_t*) client_state->client, alloc_buffer, client_read);

  if (status < 0) {
    perror("uv_read_start()");
    exit(-1);
  }

  free(req);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf) {
  buf->base = (char*) malloc(suggested_size);
  buf->len = suggested_size;
}

void client_read(uv_stream_t *client, ssize_t nread, const uv_buf_t* buf) {
  if (nread == -1) {
    perror("cliend_read");
    uv_close((uv_handle_t*)client, NULL);
    return;
  }else {
    client_state_t *client_state = client->data;

    for (int i = 0; i < nread; ++i) {
      if (client_state->pState == IN_ACK) {
        assert(false);
      }else if (client_state->pState == IN_MSG) {
        if (buf->base[i] == '$') {
          client_state->pState = WAIT_FOR_MSG;
        }else {
          client_state->write_buf[client_state->write_buf_len++] = buf->base[i] + 1;
        }
      }else if (client_state->pState == WAIT_FOR_MSG) {
        if (buf->base[i] == '^') {
          client_state->pState = IN_MSG;
        }
      }
    }

    if (client_state->write_buf_len > 0) {

      uv_buf_t writebuf = uv_buf_init(client_state->write_buf, client_state->write_buf_len);

      uv_write_t* req = (uv_write_t*)malloc(sizeof(*req));
      if (req == NULL) {
        perror("malloc:");
        exit(-1);
      }

      req->data = client_state;

      int status = uv_write((uv_write_t*)req, client, &writebuf, 1, client_write);
      if (status < 0) {
        printf("Error uv_write: %s", uv_strerror(status));
        exit(-1);
      }

    }
  }
  free(buf->base);
}

void client_write(uv_write_t* handle, int status) {
  if (status < 0) {
    perror("client_write");
    exit(-1);
  }

  client_state_t *client_state = handle->data;
  client_state->write_buf_len = 0;
  free(handle);
}
