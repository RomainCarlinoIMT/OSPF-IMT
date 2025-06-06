#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int send_message(const std::string& message)
{
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_address;
    if (socket_fd < 0) {
        perror("Failed to create socket");
        return 1;
    }

    memset(&server_address, 0, sizeof(server_address));

    // Filling server information
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_port = htons(8080); // Port 8080
    server_address.sin_addr.s_addr = inet_addr("239.0.0.1"); // Multicast address

    // sending a message to the server
    ssize_t bytes_sent = sendto(socket_fd, message.c_str(), strlen(message.c_str()), 0,
                                (struct sockaddr*)&server_address, sizeof(server_address));
    if (bytes_sent < 0) {
        perror("Failed to send message");
        close(socket_fd);
        return 1;
    }

    return 0;
}

int main()
{
    std::string message = "Hello, UDP Server!";
    send_message(message);
    send_message("Another message to the server");
    return 0;
}