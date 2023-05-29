#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 12345
#define MAX_CLIENTS 5
#define BUFFER_SIZE 4096

typedef struct {
    int socket;
    char name[BUFFER_SIZE];
} Client;

void handle_client(Client* client, Client* clients) {
    char buffer[BUFFER_SIZE];
    ssize_t received_bytes;

    // Welcome the client and announce their arrival
    snprintf(buffer, BUFFER_SIZE, "%.100s has joined the chat.\n", client->name);
    printf("%s", buffer);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i != client->socket && clients[i].socket != -1) {
            send(clients[i].socket, buffer, strlen(buffer), 0);
        }
    }

    // Receive and process messages from the client
    while ((received_bytes = recv(client->socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[received_bytes] = '\0';
        printf("%.100s: %s", client->name, buffer);

        // Broadcast the message to other clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (i != client->socket && clients[i].socket != -1) {
                send(clients[i].socket, buffer, strlen(buffer), 0);
            }
        }

        // Check if the client wants to exit
        if (strncmp(buffer, "exit", 4) == 0)
            break;

        memset(buffer, 0, BUFFER_SIZE);
    }

    // Announce the client's departure and close the socket
    snprintf(buffer, BUFFER_SIZE, "%.100s has left the chat.\n", client->name);
    printf("%s", buffer);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i != client->socket && clients[i].socket != -1) {
            send(clients[i].socket, buffer, strlen(buffer), 0);
        }
    }
    close(client->socket);
    client->socket = -1;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    Client clients[MAX_CLIENTS];
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

        // Create a new process to handle the client
        if (fork() == 0) {
            close(server_fd);
            handle_client(&clients[i], clients);
            exit(EXIT_SUCCESS);
        }

        close(new_socket);
    }

    return 0;
}
