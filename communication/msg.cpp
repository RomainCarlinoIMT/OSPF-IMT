#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "../logic/logic.h"

int send_message(const std::string& message, const std::string& interface_ip) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct in_addr local_interface;
    if (inet_aton(interface_ip.c_str(), &local_interface) == 0) {
        std::cerr << "Invalid interface IP: " << interface_ip << std::endl;
        close(sock);
        return 1;
    }

    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
                   &local_interface, sizeof(local_interface)) < 0) {
        perror("setsockopt IP_MULTICAST_IF");
        close(sock);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("239.0.0.1");

    ssize_t sent = sendto(sock, message.c_str(), message.length(), 0,
                          (struct sockaddr*)&addr, sizeof(addr));
    if (sent < 0) {
        perror("sendto");
        close(sock);
        return 1;
    }

    std::cout << "Message sent successfully from interface with IP: " << interface_ip << std::endl;

    close(sock);
    return 0;
}

bool send_router_declaration(const RouterDeclaration& router_declaration, const std::string& interface_ip) {
    std::string message = serialize_router_definition(router_declaration);
    int result = send_message(message, interface_ip);
    if (result != 0) {
        std::cerr << "Failed to send router declaration message." << std::endl;
        return false;
    }
    std::cout << "Router declaration sent successfully." << std::endl;

    return 0;
}

bool send_all_proper_router_declarations(
    const std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb,
    const std::string& interface_ip) {
    
    for (const auto& [router_id, declarations] : local_lsdb) {
        for (const auto& [interface, declaration] : declarations) {
            if (!send_router_declaration(declaration, interface_ip)) {
                return false;
            }
        }
    }
    return true;
}