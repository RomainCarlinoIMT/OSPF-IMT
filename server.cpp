#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    // Creating the servers socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        return 1;
    }
    printf("Server socket created successfully.\n");

    // Setting up server parameters
    sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
    server_address.sin_port = htons(8080); // Port 8080

    // Binding the socket
    bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    // Listening for incoming connections
    if (listen(server_socket, 5) < 0) {
        perror("Failed to listen on socket");
        return 1;
    }
    printf("Server is listening on port 8080.\n");
    
    // Accepting a connection
    int client_socket = accept(server_socket, nullptr, nullptr);
    if (client_socket < 0) {
        perror("Failed to accept connection");
        return 1;
    }
    printf("Client connected successfully.\n");

    // receiving data from the client
    char buffer[1024];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("Failed to receive data");
        return 1;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received data
    printf("Received data: %s\n", buffer);


    //closing the sockets
    close(server_socket);
    return 0;
}
