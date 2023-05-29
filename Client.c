#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#define MAX_BUFFER 2048

typedef struct {
    char name[32];
    int sockfd;
} client_t;

void *receive_handler(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[MAX_BUFFER];
    while (1) {
        int receive = recv(client->sockfd, buffer, MAX_BUFFER, 0);
        if (receive > 0) {
            printf("%s\n", buffer); // Print the relayed message on the client's terminal
        } else if (receive == 0) {
            break;
        } else {
            // -1
        }
        memset(buffer, 0, sizeof(buffer));
    }
    return NULL;
}

int main(int argc, char **argv) {
    char *ip = "127.0.0.1";
    int port = 12345;
    char buffer[MAX_BUFFER];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) > 0) {
        printf("Connection to the server failed...\n");
        exit(1);
    }

    printf("Connected to server. Enter your name: ");
    fgets(buffer, 32, stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    send(sockfd, buffer, strlen(buffer), 0);

    client_t client;
    strcpy(client.name, buffer);
    client.sockfd = sockfd;

    printf("Welcome %s! You can now send messages!\n", client.name);

    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, &receive_handler, (void *)&client) != 0) {
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    while (1) {
        fgets(buffer, MAX_BUFFER, stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        send(sockfd, buffer, strlen(buffer), 0);

        if (strcmp(buffer, "/quit") == 0) {
            break;
        }
    }

    close(sockfd);

    return EXIT_SUCCESS;
}