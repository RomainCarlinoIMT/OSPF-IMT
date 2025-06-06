#include <iostream>
#include <string>       // Pour std::string
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>      // Pour memset

int send_message(const std::string& message, const std::string& interface_ip)
{
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Failed to create socket");
        return 1;
    }

    // --- Configuration de l'interface de sortie pour le multicast ---
    struct in_addr local_interface_ip;
    if (inet_aton(interface_ip.c_str(), &local_interface_ip) == 0) {
        std::cerr << "Invalid interface IP address: " << interface_ip << std::endl;
        close(socket_fd);
        return 1;
    }

    // IMPORTANT : Vous aviez omis cette ligne dans votre code.
    // C'est cette ligne qui définit l'interface de sortie pour les paquets multicast.
    if (setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&local_interface_ip, sizeof(local_interface_ip)) < 0) {
        perror("Failed to set IP_MULTICAST_IF");
        close(socket_fd);
        return 1;
    }
    // --- Fin de la configuration de l'interface ---

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));

    // Filling server information
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_port = htons(8080); // Port 8080
    server_address.sin_addr.s_addr = inet_addr("239.0.0.1"); // Multicast address

    // sending a message to the server
    // Utilisez message.length() qui est plus idiomatique et efficace avec std::string
    ssize_t bytes_sent = sendto(socket_fd, message.c_str(), message.length(), 0,
                                (struct sockaddr*)&server_address, sizeof(server_address));
    if (bytes_sent < 0) {
        perror("Failed to send message");
        close(socket_fd);
        return 1;
    }

    std::cout << "Message sent successfully from interface " << interface_ip << std::endl;

    close(socket_fd);
    return 0;
}

int main()
{
    std::string message1 = "Hello, UDP Server!";
    send_message(message1, "127.0.0.1");

    // Correction ici : Convertissez le littéral de chaîne en std::string
    // car la fonction send_message attend une const std::string&
    std::string message2 = "Another message to the server";
    send_message(message2, "127.0.0.1");
    
    // Ou directement comme ceci, si vous n'avez pas besoin d'une variable nommée
    // send_message(std::string("Another message to the server"), "10.2.0.2");

    return 0;
}