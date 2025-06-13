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
#include <vector>
#include <fstream>
#include <sstream>
#include <queue>
#include <set>
#include <climits>
#include "../logic/logic.h"
#include "msg.h"

std::map<std::string, std::string> forwardingTable;
std::map<std::string, std::map<std::string, RouterDeclaration>> local_lsdb; 

std::string get_local_hostname() {
    char hostname[256]; // Taille typique suffisante pour les hostnames
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        exit(EXIT_FAILURE);
    }
    return std::string(hostname);
}


// Joindre le groupe multicast sur toutes les interfaces IPv4 actives supportant le multicast
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

void updateRoutingTable(const std::string& message, std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb) {
    std::istringstream ss(message);
    std::string router_info;

    while (std::getline(ss, router_info, ';')) {
        std::istringstream router_ss(router_info);
        std::string router_name, interface_ip, cost_str;

        if (std::getline(router_ss, router_name, '|') &&
            std::getline(router_ss, interface_ip, '|') &&
            std::getline(router_ss, cost_str, '|')) {

            int cost = std::stoi(cost_str);
            RouterDeclaration new_router = create_router_definition(router_name, interface_ip + "/24", cost);

            add_router_declaration(local_lsdb, new_router);
            std::cout << "Updated routing table with: " << router_name << " - " << interface_ip << " - " << cost << std::endl;
        }
    }
}

void computeShortestPaths(const std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, const std::string& LOCAL_ROUTER_ID) {
    // Distance minimale vers chaque routeur
    std::map<std::string, int> distances;
    // Pour retrouver le next hop
    std::map<std::string, std::string> previous;

    // Initialisation
    for (const auto& router : local_lsdb) {
        distances[router.first] = INT_MAX;
    }
    distances[LOCAL_ROUTER_ID] = 0;

    // Min-heap (distance, router_id)
    using P = std::pair<int, std::string>;
    std::priority_queue<P, std::vector<P>, std::greater<>> queue;

    queue.push({0, LOCAL_ROUTER_ID});

    while (!queue.empty()) {
        auto [dist, current] = queue.top();
        queue.pop();

        if (local_lsdb.find(current) == local_lsdb.end()) continue;

        for (const auto& neighbor : local_lsdb.at(current)) {
            int cost = neighbor.second.link_cost;
            const std::string& neighborId = neighbor.first;

            if (distances[current] + cost < distances[neighborId]) {
                distances[neighborId] = distances[current] + cost;
                previous[neighborId] = current;
                queue.push({distances[neighborId], neighborId});
            }
        }
    }

    // Mise à jour de la table de routage
    forwardingTable.clear();
    for (const auto& [destination, _] : distances) {
        if (destination == LOCAL_ROUTER_ID || distances[destination] == INT_MAX) continue;
    
        std::string nextHop = destination;
        while (previous.find(nextHop) != previous.end() && previous[nextHop] != LOCAL_ROUTER_ID) {
            nextHop = previous[nextHop];
        }
    
        if (previous.find(nextHop) != previous.end()) { // vérifie qu'on a bien trouvé un chemin
            forwardingTable[destination] = nextHop;
        }
    }
}

// Fonction pour afficher la table de routage
void updateForwardingTable() {
    std::cout << "\n==== Forwarding Table ====\n";
    for (const auto& [destination, nextHop] : forwardingTable) {
        std::cout << "Destination: " << destination << " via " << nextHop << "\n";
    }
    std::cout << "==========================\n";
}


void apply_route_to_system(const std::string& destination, const std::string& nextHop) {
    // Exemple de commande ip route (à adapter selon ton réseau)
    std::string cmd = "ip route replace " + destination + " via " + nextHop;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Failed to apply route: " << cmd << std::endl;
    } else {
        std::cout << "Applied route: " << cmd << std::endl;
    }
}


// Traitement des messages reçus
void on_receive(int sock, std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, const std::string& LOCAL_ROUTER_ID) {
    char buffer[1024]; // Normaly this should be less than 1024 bytes, but we add some extra space for safety
    sockaddr_in sender_addr{};
    socklen_t sender_len = sizeof(sender_addr);

    ssize_t len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                           (sockaddr*)&sender_addr, &sender_len);
    if (len < 0) {
        perror("recvfrom");
        return;
    }
    buffer[len] = '\0';

    try
    {
        // Deserialize received message
        RouterDeclaration received_declaration = deserialize_router_definition(buffer);
        // Calling add_router_declaration to update the local_lsdb
        add_router_declaration(local_lsdb, received_declaration);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to deserialize router declaration: " << e.what() << "\n";
        return; // Ignore invalid messages
    }

    std::cout << "[UDP] Received from " << inet_ntoa(sender_addr.sin_addr) << ": " << buffer << "\n";

    updateRoutingTable(buffer, local_lsdb);

    computeShortestPaths(local_lsdb, LOCAL_ROUTER_ID);

    updateForwardingTable();

    for (const auto& [destination, nextHop] : forwardingTable) {
        apply_route_to_system(destination, nextHop);
    }

    debug_known_router(local_lsdb);
}




void on_update(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, const std::string& LOCAL_ROUTER_ID, std::vector<std::string>& interfaces) {
    // firstly updating lsdb
    update_lsdb(local_lsdb, LOCAL_ROUTER_ID);

    // Sending all router declarations to all interfaces
    send_all_router_declarations_to_all(local_lsdb, interfaces);
    std::cout << "Periodic update task executed." << std::endl; // Debug output Note: Remove in production
    debug_known_router(local_lsdb);
}


void read_config_file(const std::string& filename, std::vector<std::string>& interfaces) {
    // This function should read the configuration file and populate the interfaces vector
    std::vector<std::string> config_interfaces;
    std::ifstream config_file(filename);
    if (!config_file.is_open())
    {
        throw std::runtime_error("Could not open configuration file:");
    }

    // Assume that user will provide a file with one interface per line
    std::string line;
    while (std::getline(config_file, line)) {
        if (!line.empty() && line[0] != '#') {
            // Test if the line is a valid ip with mask
            if(assert_ip_and_mask(line)) {
                // Take only the ip before the slash
                size_t slash_pos = line.find('/');
                if (slash_pos != std::string::npos) {
                    std::string ip = line.substr(0, slash_pos);
                    interfaces.push_back(ip);
                }
                else 
                {
                    // Debug that config contains one invalide line
                    std::cerr << "Invalid interface format in config file: " << line << "\n";
                }
            } else {
                std::cerr << "Invalid interface format in config file: " << line << "\n";
            }
        }
    }
    
    config_file.close();
    if (interfaces.empty()) {
        throw std::runtime_error("No interfaces found in configuration file.");
    }    
}

void debug_output_ips(const std::vector<std::string>& interfaces) {
    std::cout << "Interfaces with IPs:\n";
    for (const auto& iface : interfaces) {
        std::cout << "- " << iface << "\n";
    }
}

void create_server_declaration(const std::vector<std::string>& interfaces, std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, const std::string& LOCAL_ROUTER_ID) {
    for(const auto& iface : interfaces)
    {
        RouterDeclaration router_declaration = create_router_definition(LOCAL_ROUTER_ID, iface , 10);
        add_router_declaration(local_lsdb, router_declaration);
    }    
}

int main() {
    // Running variables
    std::map<std::string, std::map<std::string, RouterDeclaration>> local_lsdb;
    std::vector<std::string> interfaces;
    std::string LOCAL_ROUTER_ID = get_local_hostname();

    // Read configuration file to get interfaces and debug output
    try {
        read_config_file("config", interfaces);
    } catch (const std::exception& ex) {
        std::cerr << "Error reading configuration file: " << ex.what() << std::endl;
        return 1;
    }
    debug_output_ips(interfaces);

    // Create default lsdb with is own declaration
    create_server_declaration(interfaces, local_lsdb, LOCAL_ROUTER_ID);
    debug_known_router(local_lsdb); // Fror debug purpose Note: Remove in production

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

    fd_set read_fds;
    struct timeval timeout;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        // Timeout de 5 secondes pour appeler onUpdate périodiquement
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int ret = select(sock + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ret < 0) {
            perror("select");
            break;
        } else if (ret == 0) {
            // Timeout expiré, on fait la mise à jour périodique
            on_update(local_lsdb, LOCAL_ROUTER_ID, interfaces);
        } else {
            if (FD_ISSET(sock, &read_fds))
            {
                on_receive(sock, local_lsdb, LOCAL_ROUTER_ID);
            }
        }
    }

    close(sock);
    return 0;
}
