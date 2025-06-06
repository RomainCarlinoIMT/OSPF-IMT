#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>  // pour les flags d'interface

void join_multicast_all_interfaces(int sock, const std::string& multicast_ip) {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;  // IPv4 uniquement
        if (!(ifa->ifa_flags & IFF_UP)) continue;           // interface up
        if (!(ifa->ifa_flags & IFF_MULTICAST)) continue;    // multicast supporté

        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
        mreq.imr_interface = ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;

        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            std::cerr << "Failed to join multicast on interface " << ifa->ifa_name
                      << " (" << inet_ntoa(mreq.imr_interface) << "): " << strerror(errno) << "\n";
        } else {
            std::cout << "Joined multicast " << multicast_ip << " on interface " << ifa->ifa_name
                      << " (" << inet_ntoa(mreq.imr_interface) << ")\n";
        }
    }

    freeifaddrs(ifaddr);
}

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    std::string multicast_ip = "239.0.0.1";
    join_multicast_all_interfaces(sock, multicast_ip);

    std::cout << "Server listening on UDP port 8080...\n";

    char buffer[1024];
    sockaddr_in sender_addr{};
    socklen_t sender_len = sizeof(sender_addr);

    while (true) {
        ssize_t len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                               (sockaddr*)&sender_addr, &sender_len);
        if (len < 0) {
            perror("recvfrom");
            break;
        }
        buffer[len] = '\0';
        std::cout << "[UDP] Received from " << inet_ntoa(sender_addr.sin_addr) << ": " << buffer << "\n";
    }

    close(sock);
    return 0;
}
