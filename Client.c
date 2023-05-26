#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#define VERSION 1.0.5

// Printon mesazhin e gabimit dhe mbyll programin
void error(const char *msg){
  perror(msg);
  exit(EXIT_FAILURE);
}
int main(int argc, char **argv)
{
    char buffer[255];
    char name[15], newname[15];    //Emri deri ne 15 karaktere
    int client_socket, nameval, msgval, sd; // mbajme vlerat e soketeve dhe ato te kthyera nga leximi dhe shkrimi
    int activity;                    // Perdoret me select() per te kontrolluar per mesazhe te reja te ardhura
    struct addrinfo address, *res, *p;








