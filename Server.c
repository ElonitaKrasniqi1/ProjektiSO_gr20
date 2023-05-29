#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048

typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int cli_count = 0;
int uid = 0;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_timestamp(char* buffer) {
    time_t timer;
    char time_buffer[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    strcat(buffer, time_buffer);
}

void send_message(const char *s, const int uid){
    pthread_mutex_lock(&clients_mutex);
    pthread_mutex_lock(&print_mutex);
    for(int i=0; i<MAX_CLIENTS; ++i){
        if(clients[i]){
            if(clients[i]->uid != uid){
                write(clients[i]->sockfd, s, strlen(s));
            }
        }
    }
    pthread_mutex_unlock(&print_mutex);
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg){
    char buffer[BUFFER_SIZE];
    char name[32];
    int leave_flag = 0;

    client_t *cli = (client_t *)arg;

    // Name protocol
    if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32-1){
        printf("Didn't enter the name correctly.\n");
        leave_flag = 1;
    } else {
        strncpy(cli->name, name, sizeof(cli->name));
        printf("%s has joined\n", cli->name);
    }

    // Conversation protocol
    while(1){
        if (leave_flag) {
            break;
        }

        int receive = recv(cli->sockfd, buffer, BUFFER_SIZE, 0);
        if (receive > 0){
            if(strlen(buffer) > 0){
                if(strcmp(buffer, "/quit") == 0){
                    leave_flag = 1;
                } else if(strcmp(buffer, "/list") == 0){
                    for(int i = 0; i < MAX_CLIENTS; i++){
                        if(clients[i]){
                            send_message(clients[i]->name, cli->uid);
                        }
                    }
                } else if(strncmp(buffer, "/msg", 4) == 0){
                    char recipient[32];
                    strncpy(recipient, buffer + 5, sizeof(recipient));
                    char *space_ptr = strchr(recipient, ' ');
                    if(space_ptr != NULL){
                        *space_ptr = '\0';
                        char *message = space_ptr + 1;
                        for(int i = 0; i < MAX_CLIENTS; i++){
                            if(clients[i] != NULL && strcmp(clients[i]->name, recipient) == 0){
                                send_message(message, cli->uid);
                                break;
                            }
                        }
                    }
                } else {
                    add_timestamp(buffer);
                    strcat(buffer, cli->name); // Add the name of the client to the message

                    pthread_mutex_lock(&print_mutex);
                    printf("%s\n", buffer); // Print the message on the server's terminal
                    pthread_mutex_unlock(&print_mutex);

                    send_message(buffer, cli->uid);
                }
            }
        } else if (receive == 0){
            sprintf(buffer, "%s has left", cli->name);
            printf("%s\n", buffer);
            leave_flag = 1;
        } else {
            printf("ERROR: -1\n");
            leave_flag = 1;
        }
        bzero(buffer, BUFFER_SIZE);
    }

    close(cli->sockfd);
    pthread_mutex_lock(&clients_mutex);

    // Find and remove client from array
    for(int i=0; i<MAX_CLIENTS; ++i) {
        if(clients[i] == cli) {
            clients[i] = NULL;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    free(cli);
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char **argv){
    char *ip = "127.0.0.1";
    int port = 12345;

    int sockfd, new_sock;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len;

    // Set up server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(sockfd, 1);

    printf("Server started...\n");

    while(1){
        cli_len = sizeof(cli_addr);
        new_sock = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);

        // Check if max clients is reached
        if((cli_count + 1) == MAX_CLIENTS){
            printf("Max clients reached. Rejected: \n");
            close(new_sock);
            continue;
        }

        // Client settings
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = cli_addr;
        cli->sockfd = new_sock;
        cli->uid = uid++;

        pthread_t tid;

        // Add client to the array
        pthread_mutex_lock(&clients_mutex);
        for(int i=0; i<MAX_CLIENTS; ++i){
            if(!clients[i]){
                clients[i] = cli;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // Create thread for client communication
        pthread_create(&tid, NULL, &handle_client, (void*)cli);

        sleep(1);
    }

    return 0;
}