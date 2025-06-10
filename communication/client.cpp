#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <map>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <regex>
#include <array>
#include "../logic/logic.h"

// Exécute une commande et retourne sa sortie
std::string execCommand(const char* cmd) {
    std::array<char, 256> buffer;
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() a échoué !");
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    return result;
}

// Récupère le débit max configuré (rate) d'une interface via "tc qdisc show"
std::string getInterfaceRate(const std::string& iface) {
    std::string cmd = "tc qdisc show dev " + iface;
    try {
        std::string tcOutput = execCommand(cmd.c_str());
        std::regex rateRegex(R"(rate\s+(\d+[KMG]?bit))");
        std::smatch match;
        if (std::regex_search(tcOutput, match, rateRegex)) {
            return match[1];
        }
    } catch (...) {
        // ignore errors silently for this example
    }
    return "none";
}

// Récupère les interfaces actives et leur adresse IP (hors loopback)
std::map<std::string, std::string> getNetworkInterfacesWithIPs() {
    std::map<std::string, std::string> interfaces_info;
    struct ifaddrs *ifaddr, *ifa;
    char ip_address[INET_ADDRSTRLEN];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return interfaces_info;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;
        if (!(ifa->ifa_flags & IFF_UP)) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);

            if (inet_ntop(AF_INET, &(sa->sin_addr), ip_address, INET_ADDRSTRLEN) != nullptr) {
                if (interfaces_info.find(ifa->ifa_name) == interfaces_info.end()) {
                    interfaces_info[ifa->ifa_name] = ip_address;
                }
            } else {
                std::cerr << "Error converting IP address for interface " << ifa->ifa_name << ": " << strerror(errno) << std::endl;
            }
        }
    }

    freeifaddrs(ifaddr);
    return interfaces_info;
}

// Envoie un message multicast via l'interface donnée
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

int main() {
    std::string interface_ip_for_sending = "10.2.0.4";

    RouterDeclaration router_declaration = create_router_definition("Router1", interface_ip_for_sending + "/24", 10);
    std::map<std::string, std::map<std::string, RouterDeclaration>> local_lsdb;
    add_router_declaration(local_lsdb, router_declaration);
    debug_known_router(local_lsdb);

    std::map<std::string, std::string> active_interfaces = getNetworkInterfacesWithIPs();
    
    if (active_interfaces.empty()) {
        std::cerr << "No active IPv4 network interfaces found (excluding loopback)." << std::endl;
        return 1;
    }

    std::ostringstream message_stream;

    // Construction du message dans le format attendu
    for (const auto& pair : active_interfaces) {
        const std::string& iface_name = pair.first;
        const std::string& iface_ip = pair.second;
        std::string rate = getInterfaceRate(iface_name);

        // Conversion du débit en coût arbitraire pour la démo
        int cost = 10; // Valeur par défaut
        if (rate != "none") {
            if (rate.find("Mbit") != std::string::npos) {
                cost = 1000 / std::stoi(rate.substr(0, rate.find("Mbit"))); // Exemple : plus le débit est élevé, plus le coût est faible
            } else if (rate.find("Kbit") != std::string::npos) {
                cost = 10000 / std::stoi(rate.substr(0, rate.find("Kbit"))); // Plus le débit est faible, plus le coût est élevé
            }
        }

        // Exemple : Nom du routeur = nom de l'interface
        message_stream << iface_name << " " << iface_ip << "/24 " << cost << "; ";
    }

    std::string message = message_stream.str();
    if (!message.empty() && message.back() == ' ') {
        message.pop_back(); // On retire l'espace final
    }

    return send_message(message, interface_ip_for_sending);
}
