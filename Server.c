
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

//degjojme per lidhje
    if(listen(server_socket, MAX_CLIENTS) < 0)
        error("[SYS_MSG]: Error Listning");

    //pranojme lidhjet qe vijne
    puts("[SYS_MSG]: Waiting for connection......");

    //vendosim madhesine e adreses qe vjen
    addr_size = sizeof clnt_addr;

    while (1){  
        //e fshijme file descriptor socket set
        FD_ZERO(&readfds);

        //shtojme server socket ne file descriptor
        FD_SET(server_socket, &readfds);
        max_sd = server_socket;

        //shtojme child sockets ne set
        for (i = 0; i < MAX_CLIENTS; i++){
            //socket socket descriptors
            sd = client_socket[i];

            //nese socket descriptor eshte valid, shtoje ne setin e file descriptoreve
            if (sd > 0) FD_SET(sd, &readfds);

            //shtojme numrin me te madh te file descp per funksionin select()
            if (sd > max_sd) max_sd = sd;
        }

        //wait for activity on the sockets indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        // check per errors
        if ((activity < 0) && (errno != EINTR)) error("[SYS_MSG]: Error Select");
        //check per lidhje te reja
        if (FD_ISSET(server_socket, &readfds)){
            //pranojme lidhjet e reja
            new_socket = accept(server_socket, (struct sockaddr *)&clnt_addr, &addr_size);
            if (new_socket < 0) error("[SYS_MSG]: Incomming Connection Not Accepted."); //check per errors
            //printojme tdhenat e lidhjes se re
            printf("[SYS_MSG]: New Connection.\n"
            "\tSocket FD  -> %d\n"
            "\tIP Address -> %s\n"
            "\tPort       -> %d\n", new_socket, inet_ntop(clnt_addr.ss_family, (struct sockaddr*)&clnt_addr, remoteIP, INET6_ADDRSTRLEN), port_num);
            //i dergojme lidhjes se re mesazh mireseardhje
            if (send(new_socket, message, strlen(message), 0) != strlen(message))
                error("[SYS_MSG]: Send failed.");  //check per errors

            puts("[SYS_MSG]: Welcome message sent successfully");

            //shtojme socket te re ne arrayin e socketeve
            for (i = 0; i < MAX_CLIENTS; i++){
                //if nese kemi vend bosh
                if (client_socket[i] == 0){
                    client_socket[i] = new_socket;                          //shtojme socketen ne vendin e lire
                    nameval = recv(new_socket, client_name, sizeof(client_name), 0);   //mer emrin e klientit kur konektimi eshte stabil
                    client_name[nameval] = '\0';                               //printojme emrin per me tregu qe u mor me sukses
                    printf("[SYS_MSG]: %s has been added to the client list.\n", client_name);
                    break;
                }
            }
        }
        //else - IO operation
        for (i = 0; i < MAX_CLIENTS; i++){
            sd = client_socket[i];
            //check per aktivitete ne socket
            if (FD_ISSET(sd, &readfds)){
                nameval = recv(sd, client_name, sizeof(client_name)-1, 0);          //lexojme emrin e derguesit
                msgval = recv(sd, buffer, sizeof(buffer)-1, 0);                   //lexojme mesazhet ne ardhje
                client_name[nameval] = '\0';
                buffer[msgval] = '\0';                           //add null terminator to the end of the recieved messege

                printf("[%s]: %s\n", client_name, buffer);    //printojme mesazhin ne ekran

                if (msgval == 0 || nameval == 0){                           //check per shkeputje
                    getpeername(sd, (struct sockaddr *)&clnt_addr, &addr_size); //tdhenat e socketit

                    printf("[SYS_MSG]:Client Disconnected.\n"               //printojme mesazh per shkeputje tlidhjes
                    "\tName       -> %s\n"
                    "\tIP Address -> %s\n"
                    "\tPort       -> %d\n",client_name, inet_ntop(clnt_addr.ss_family, (struct sockaddr*)&clnt_addr, remoteIP, INET6_ADDRSTRLEN), port_num);

                    FD_CLR(sd, &readfds); // hekim nga master set
                    close(sd); //mbyll socketen
                    client_socket[i] = 0;

                } else {
                    //echo message back te e gjithe klientet, duke perfshire edhe emrin e derguesit
                    for (int j = 0; j < MAX_CLIENTS; j++){
                        if (FD_ISSET(client_socket[j], &readfds))
                        {   if (client_socket[j] != server_socket){
                              if (send(client_socket[j], client_name, nameval, 0) == -1) perror("Send Name");
                              if (send(client_socket[j], buffer, msgval, 0) == -1) perror("Send Msg");
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}