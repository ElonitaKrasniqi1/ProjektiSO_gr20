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
    memset (&address, 0, sizeof(address));
  
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
        perror("[SYS_MSG]: getaddrinfo fail.");
  
  // loop rreth te gjitha rezultateve dhe konekto te paren e mundshme
    puts("[SYS_MSG]: Connection......");
    for(p = res; p != NULL; p = p->ai_next) {
        //krijimi i socket 
        if ((client_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("[SYS_MSG]: Error on Socket"); //bejme check per error
            continue;
        }
      
      //// Lidhuni me serverin duke perdorur adresen dhe gjatesin e adreses se dhene ne strukturen p.
        if (connect(client_socket, p->ai_addr, p->ai_addrlen) == -1) {
            close(client_socket);
            perror("[SYS_MSG]: Error on Connection");
            continue;
        }

        break;
    }
  
  //nese struktura eshte null mesazh errori
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    puts("[SYS_MSG]: Connection Established.");
    freeaddrinfo(res); // all done with this structure

    //shto client socket ne file descrip
    FD_SET(client_socket, &readfds);
    FD_SET(client_socket, &writefds);

    sd = client_socket;
  
      
   // presim serverin per welcome message
    msgval = read(client_socket, buffer, sizeof(buffer));       //lexojme nga serveri 
    if (msgval < 0) 
        perror("[SYS_MSG]: Error on Reading");
    printf("[SERVER]: %s\n", buffer);

    //dergojme emrin te serveri pasi qe konektimi te jete stabil
    nameval = send(client_socket, name, strlen(name), 0);   //shkruajm te serveri
    if (nameval < 0) 
        perror("[SYS_MSG]: Error on Writting");    //bejme check  per error message

    //ketu fillon komunikimi 
    while (1){
      //nese socket descriptori eshte valid atehere e vendosim te file descriptor set
      if (sd > 0) 
          FD_SET(sd, &readfds);
      //Prit per aktivitet ne soketat pa kufizim kohor.
      activity = select(sd + 1, &readfds, &writefds, NULL, NULL);
      // check per errora
      if ((activity < 0) && (errno != EINTR)) 
          perror("[SYS_MSG]: Error Select");

      if (FD_ISSET(sd, &readfds)){
          nameval = recv(sd, newname, sizeof(newname), 0);          //merr emrin e derguesit
//// Kontrollo per gabime gjate pranimit te mesazhit ose emrit. 
          msgval = recv(sd, buffer, sizeof(buffer), 0);
          if (msgval < 0 || nameval < 0) 
          perror("Reiev Error");

          newname[nameval] = '\0';
          buffer[msgval] = '\0';                        // Shto null terminator ne fund te mesazhit te pranuar.
          printf("[%s]: %s\n",newname, buffer);
       }
// Nese soketi sd eshte pjese e writefds, printo ">>> ", prit per input nga shfletuesi,
        if (FD_ISSET(sd, &writefds)){
         printf(">>> ");
         fgets(buffer, sizeof(buffer), stdin);
// dergo emrin dhe bufferin ne server duke perdorur send().
         send(client_socket, name, strlen(name), 0);
         send(client_socket, buffer, strlen(buffer), 0);
        }
    }
//mbyll client socketin
    close(client_socket);
    return 0;
}
  
  






