#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 12345
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 4096

int main() {
    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

    // Create a socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address and port
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &(server_address.sin_addr)) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server. Enter your name: ");
    fgets(buffer, BUFFER_SIZE, stdin);

    // Send the client name to the server
    send(client_socket, buffer, strlen(buffer), 0);

    // Start the chat
    printf("Chat started. You can start sending messages.\n");

    while (1) {
        printf("Enter your message (or 'exit' to quit): ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Send the message to the server
        send(client_socket, buffer, strlen(buffer), 0);

        // Check if the client wants to exit
        if (strncmp(buffer, "exit", 4) == 0)
            break;

        // Receive and display the server's response
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t received_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (received_bytes > 0) {
            printf("Server response: %s\n", buffer);
        }
    }

    // Close the socket
    close(client_socket);

    return 0;
}
