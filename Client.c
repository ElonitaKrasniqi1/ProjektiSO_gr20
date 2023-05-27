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
  
//nese nr eshte me i vogel se 3 shfaqet stderr per userin dhe tregon qysh me perdor programin edhe mbyllet programi pa sukses
    if (argc < 3){
      fprintf(stderr, "[SYS_MSG]: usage %s hostname port\n", argv[0]);
      exit(EXIT_FAILURE);
    }

  //seti i pershkruesve te soketes
    fd_set readfds, writefds;
    //fshini setin e pershkruesve te soketave te file descriptoreve
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
  
   //kerko emrin e perdoruesit
    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strlen(name)-1] = '\0';    // shto nje emer te ri
  
  memset(&address, 0,sizeof(address)); //fshi server objektin vendos 0
 
    //specifiko nje adres per socket 
    address.ai_family = AF_INET;
    address.ai_socktype = SOCK_STREAM;
    puts("[SYS_MSG]: Initializing......");  
  
//kerko informacionin e adreses per hostin dhe portin
    int status = getaddrinfo(argv[1], argv[2] , &address, &res);
    if (status < 0)
//Kontrollo nese ka error gjate marrjes
        error("[SYS_MSG]: getaddrinfo fail.");
  
  






