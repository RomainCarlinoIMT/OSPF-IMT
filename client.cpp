#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main()
{
    // creating socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Failed to create socket");
        return 1;
    }

    // setting address
    sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_port = htons(8080); // Port 8080
    server_address.sin_addr.s_addr = INADDR_ANY;

    //sending connection request
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Failed to connect to server");
        close(client_socket);
        return 1;
    }

    //sending data to the server
    const char* message = "Hello, Server!";
    ssize_t bytes_sent = send(client_socket, message, strlen(message), 0);
    if (bytes_sent < 0) {
        perror("Failed to send data");
        close(client_socket);
        return 1;
    }
    
}