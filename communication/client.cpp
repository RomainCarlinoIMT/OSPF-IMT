#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <regex>
#include <array>
#include <vector>
#include <set>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
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

// Récupère les noms des interfaces réseau (hors "lo")
std::vector<std::string> getNetworkInterfaces() {
    std::string ipOutput = execCommand("ip -o link show | awk -F': ' '{print $2}'");
    std::istringstream stream(ipOutput);
    std::string line;
    std::vector<std::string> interfaces;
    std::set<std::string> ignored = {"lo"};

    while (std::getline(stream, line)) {
        if (!line.empty() && ignored.find(line) == ignored.end()) {
            interfaces.push_back(line);
        }
    }
    return interfaces;
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

    std::cout << "Message sent successfully from interface " << interface_ip << std::endl;

    close(sock);
    return 0;
}

int main() {
    std::string interface_ip = "10.2.0.4";  // L'IP de l'interface à utiliser pour envoyer

    RouterDeclaration router_declaration = create_router_definition("Router1", interface_ip + "/24", 10);
    std::map<std::string, std::map<std::string, RouterDeclaration>> local_lsdb;
    add_router_declaration(local_lsdb, router_declaration);
    debug_known_router(local_lsdb);

    // Récupérer toutes les interfaces et leur débit max
    std::vector<std::string> interfaces;
    try {
        interfaces = getNetworkInterfaces();
    } catch (const std::exception& ex) {
        std::cerr << "Erreur lors de la récupération des interfaces : " << ex.what() << std::endl;
        return 1;
    }

    // Construire le message avec interface + débit
    std::ostringstream message;
    for (const auto& iface : interfaces) {
        std::string rate = getInterfaceRate(iface);
        message << iface << ":" << rate << "; ";
    }

    // Envoi du message multicast
    return send_message(message.str(), interface_ip);
}
