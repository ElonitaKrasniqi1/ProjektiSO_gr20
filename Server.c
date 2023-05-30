#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#define PORT 12345
#define MAX_CLIENTS 5
#define BUFFER_SIZE 4096

typedef struct {
    int socket;
    char name[BUFFER_SIZE];
} Client;

typedef struct {
    char timestamp[30];
    char sender[BUFFER_SIZE];
    char message[BUFFER_SIZE];
} Message;

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void get_timestamp(char* timestamp) {
    time_t raw_time;
    struct tm* time_info;

    time(&raw_time);
    time_info = localtime(&raw_time);

    strftime(timestamp, 30, "%Y-%m-%d %H:%M:%S", time_info);
}

void broadcast_message(Message* message) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != -1) {
            send(clients[i].socket, (char*)message, sizeof(Message), 0);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void handle_client(Client* client) {
    char buffer[BUFFER_SIZE];
    ssize_t received_bytes;

    Message message;
    strcpy(message.sender, client->name);

    // Welcome the client and announce their arrival
    snprintf(buffer, BUFFER_SIZE, "%.100s has joined the chat.\n", client->name);
    strcpy(message.message, buffer);
    get_timestamp(message.timestamp);

    broadcast_message(&message);


// Announce client's arrival
pthread_mutex_lock(&clients_mutex);
printf("%s joined the chat.\n", client->name);
pthread_mutex_unlock(&clients_mutex);

    // Receive and process messages from the client
    while ((received_bytes = recv(client->socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[received_bytes] = '\0';
    printf("%.30s - %.100s: %s", message.timestamp, client->name, buffer);


        strcpy(message.message, buffer);
        get_timestamp(message.timestamp);

        // Broadcast the message to other clients
        broadcast_message(&message);

        // Check if the client wants to exit
        if (strncmp(buffer, "exit", 4) == 0)
            break;

        memset(buffer, 0, BUFFER_SIZE);
    }
    

    // Announce the client's departure and close the socket
    snprintf(buffer, BUFFER_SIZE, "%.100s has left the chat.\n", client->name);
    strcpy(message.message, buffer);
    get_timestamp(message.timestamp);

    broadcast_message(&message);

    close(client->socket);

    pthread_mutex_lock(&clients_mutex);
    client->socket = -1;
    pthread_mutex_unlock(&clients_mutex);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    pthread_t tid;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = -1;
        memset(clients[i].name, 0, BUFFER_SIZE);
    }

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options and bind to a port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    // Accept and handle incoming clients
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accepting failed");
            exit(EXIT_FAILURE);
        }

        // Find an available slot for the new client
        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == -1) {
                clients[i].socket = new_socket;
                break;
            }
        }

        // If no slot is available, reject the client
        if (i == MAX_CLIENTS) {
            const char* message = "Chat room is full. Try again later.\n";
            send(new_socket, message, strlen(message), 0);
            close(new_socket);
            continue;
        }

        // Receive the client's name
        ssize_t received_bytes = recv(clients[i].socket, clients[i].name, BUFFER_SIZE - 1, 0);
        clients[i].name[received_bytes] = '\0';

        // Create a new thread to handle the client
        if (pthread_create(&tid, NULL, (void*)handle_client, &clients[i]) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }

        pthread_detach(tid);
    }

    return 0;
}
