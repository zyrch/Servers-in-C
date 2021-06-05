#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"


int main(int argc, char **argv) {
  
  int sockfd = listen_inet_socket(8000);

  printf("FD : %d\n", sockfd);



  close(sockfd);

}
