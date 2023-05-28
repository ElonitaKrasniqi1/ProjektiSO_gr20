
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

#define MAX_CLIENTS 30

#define VERSION 1.0.4           //Lejon per lidhje te shumta me serverin permes select().


//Error message nese ndodh ndonje gabim dhe e mbyll programin
void error(const char *msg){
  perror(msg);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{


    char *message = "Welcome. You have reached the server. Ctrl-C to quit.";  //Mesazhi per welcome
    char buffer[255];                //messages mban mesazhet nga klienti
    char client_name[15];            //mban emrin e klientit
    char remoteIP[INET6_ADDRSTRLEN];

    int opt = 1;                     //opsioni per me bo server adresen te riperdorueshme
    int server_socket, new_socket, client_socket[MAX_CLIENTS]; //soketa e serverit, soketa e re per klientin e ri dhe max_clients array i soketave qe kufizon nr e klientave
    int activity;                    //perdoret me select() per te kontrolluar per lidhje ose mesazhe te reja të ardhshme
    int msgval;                      //mban numrin e karaktereve te mesazhit te ardhshem
    int nameval;                     //mban vleren e emrit te derguesit
    int i;
    int sd, max_sd;                  //mban identifikuesit e soketave

    struct addrinfo address, *res, *p; //informacioni i adreses se serverit
    struct sockaddr_storage clnt_addr; //informacioni i adreses se lidhjes
    socklen_t addr_size;               //madhesia e adreses se lidhjes

    //seti i identifikuesve te soketave
    fd_set readfds;

    //kontrollo argumentet kryesore
    if (argc < 2)
      fprintf(stderr, "[SYS_MSG]: Port Not Provided.\n");



    puts("[SYS_MSG]: Initializing......");

    memset(&address,0 ,sizeof(address)); //pastron adresen

    //specifikoni nje adrese per sokete
    address.ai_family = AF_INET;            //perdor IPV4
    address.ai_socktype = SOCK_STREAM;      //perdor nje lidhje me TCP
    address.ai_flags = AI_PASSIVE;          //perdor IP adresen e makines qe ekzekuton programin

    //merr infon e IP adreses
    int status = getaddrinfo(NULL,argv[1] , &address, &res);
    if (status < 0)
        error("[SYS_MSG]: getaddrinfo fail.");

    int port_num = atoi(argv[1]);  //numri i portit per me degju
    //inicializoje te gjitha client_socket[] me 0
     memset(client_socket,0 ,sizeof(client_socket));

    // loop-through te gjitha rezultatet dhe beni lidhjen ne te paren qe mundemi
    for(p = res; p != NULL; p = p->ai_next) {
        // krijimi i nje soketi 
        if ((server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            perror("[SYS_MSG]: socket creation failed.");
            continue;
        }
        //cakton soketin e serverit per te lejuar lidhje te shumta
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
            error("[SYS_MSG]: setsockopt Failed.");

        //lidh soketin me adresen
        if (bind(server_socket, p->ai_addr, p->ai_addrlen) < 0) {
            close(server_socket);
            perror("[SYS_MSG]: bind");
            continue;
        }
        break;
    }
    // all done me kete strukture
    freeaddrinfo(res);

    //nese lidhja deshton
    if (p == NULL)
    error("[SYS_MSG]: Failed to Bind.");

    printf("[SYS_MSG]: Listening on port %d......\n", port_num);
