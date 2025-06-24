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
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/route/route.h>
#include <netlink/route/nexthop.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>


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

// Fonction pour supprimer une route
void delete_route(const std::string& destination, const std::string& nextHop) {
    struct nl_sock *sock = nl_socket_alloc();
    if (!sock) {
        std::cerr << "Failed to allocate netlink socket" << std::endl;
        return;
    }
    if (nl_connect(sock, NETLINK_ROUTE) < 0) {
        std::cerr << "Failed to connect netlink socket" << std::endl;
        nl_socket_free(sock);
        return;
    }

    struct rtnl_route *route = rtnl_route_alloc();
    if (!route) {
        std::cerr << "Failed to allocate route" << std::endl;
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }

    struct nl_addr *dst_addr = nullptr, *gw_addr = nullptr;

    if (nl_addr_parse(destination.c_str(), AF_INET, &dst_addr) < 0) {
        std::cerr << "Invalid destination address: " << destination << std::endl;
        rtnl_route_put(route);
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }

    if (nl_addr_parse(nextHop.c_str(), AF_INET, &gw_addr) < 0) {
        std::cerr << "Invalid gateway address: " << nextHop << std::endl;
        nl_addr_put(dst_addr);
        rtnl_route_put(route);
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }

    rtnl_route_set_family(route, AF_INET);
    rtnl_route_set_dst(route, dst_addr);

    struct rtnl_nexthop *nh = rtnl_route_nh_alloc();
    if (!nh) {
        std::cerr << "Failed to allocate nexthop" << std::endl;
        nl_addr_put(dst_addr);
        nl_addr_put(gw_addr);
        rtnl_route_put(route);
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }

    rtnl_route_nh_set_gateway(nh, gw_addr);
    rtnl_route_add_nexthop(route, nh);

    int err = rtnl_route_delete(sock, route, 0);
    if (err < 0) {
        if (err == -NLE_OBJ_NOTFOUND) {
            std::cerr << "Route not found: " << destination << std::endl;
        } else {
            std::cerr << "Failed to delete route: " << nl_geterror(err) << std::endl;
        }
    } else {
        std::cout << "Route deleted successfully" << std::endl;
    }

    // Nettoyage
    nl_addr_put(dst_addr);
    nl_addr_put(gw_addr);
    rtnl_route_put(route);
    nl_close(sock);
    nl_socket_free(sock);
}

// Callback pour supprimer les routes indirectes
int route_delete_callback(struct nl_msg *msg, void *arg) {
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct rtnl_route *route = (struct rtnl_route *)nlmsg_data(nlh);

    struct nl_sock *sock = (struct nl_sock *)arg;

    // Vérifier si la route a une gateway (donc pas un voisin direct)
    struct rtnl_nexthop *nh = rtnl_route_nexthop_n(route, 0);
    if (nh != nullptr && rtnl_route_nh_get_gateway(nh) != nullptr) {
        // Supprimer la route car elle a une gateway
        int err = rtnl_route_delete(sock, route, 0);
        if (err < 0) {
            std::cerr << "Erreur suppression: " << nl_geterror(err) << std::endl;
        } else {
            std::cout << "Route supprimée." << std::endl;
        }
    } else {
        std::cout << "Route conservée (voisin direct)." << std::endl;
    }

    return NL_OK;
}

void delete_indirect_routes() {
    struct nl_sock *sock = nl_socket_alloc();
    if (!sock) {
        std::cerr << "Erreur allocation socket" << std::endl;
        return;
    }

    if (nl_connect(sock, NETLINK_ROUTE) < 0) {
        std::cerr << "Erreur connexion socket" << std::endl;
        nl_socket_free(sock);
        return;
    }

    struct rtnl_route *filter = rtnl_route_alloc();
    rtnl_route_set_family(filter, AF_INET);

    struct nl_cache *route_cache;
    if (rtnl_route_alloc_cache(sock, AF_INET, 0, &route_cache) < 0) {
        std::cerr << "Erreur chargement cache route" << std::endl;
        rtnl_route_put(filter);
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }

    std::cout << "🔧 Suppression des routes avec une gateway (routes indirectes)..." << std::endl;

    nl_cache_foreach(route_cache, [](struct nl_object *obj, void *arg) {
        struct rtnl_route *route = (struct rtnl_route *)obj;

        // Vérifier si la route a une gateway
        struct rtnl_nexthop *nh = rtnl_route_nexthop_n(route, 0);
        if (nh != nullptr && rtnl_route_nh_get_gateway(nh) != nullptr) {
            // Supprimer la route car elle a une gateway
            int err = rtnl_route_delete((struct nl_sock *)arg, route, 0);
            if (err < 0) {
                std::cerr << "Erreur suppression: " << nl_geterror(err) << std::endl;
            } else {
                std::cout << "Route supprimée." << std::endl;
            }
        } else {
            std::cout << "Route conservée (voisin direct)." << std::endl;
        }
    }, sock);

    nl_cache_free(route_cache);
    rtnl_route_put(filter);
    nl_close(sock);
    nl_socket_free(sock);

    std::cout << "Suppression terminée." << std::endl;
}

// Fonction pour ajouter une route avec suppression de toutes les routes existantes pour la même destination
void add_route(const std::string& destination, const std::string& nextHop) {
    if (destination.empty()) {
        std::cerr << "Cannot add route: destination address is empty." << std::endl;
        return;
    }
    if (nextHop.empty()) {
        std::cerr << "Cannot add route: next hop address is empty." << std::endl;
        return;
    }

    struct nl_sock *sock = nl_socket_alloc();
    if (!sock) {
        std::cerr << "Failed to allocate netlink socket" << std::endl;
        return;
    }
    if (nl_connect(sock, NETLINK_ROUTE) < 0) {
        std::cerr << "Failed to connect netlink socket" << std::endl;
        nl_socket_free(sock);
        return;
    }

    struct nl_addr *dst_addr = nullptr, *gw_addr = nullptr;

    // These calls are now protected by the checks above
    if (nl_addr_parse(destination.c_str(), AF_INET, &dst_addr) < 0) {
        std::cerr << "Invalid destination address: " << destination << std::endl;
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }

    if (nl_addr_parse(nextHop.c_str(), AF_INET, &gw_addr) < 0) {
        std::cerr << "Invalid gateway address: " << nextHop << std::endl;
        nl_addr_put(dst_addr); // Don't forget to free dst_addr if it was successfully parsed
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }

    // Supprimer les routes existantes pour cette destination (optionnel)
    struct nl_cache *route_cache = nullptr;
    if (rtnl_route_alloc_cache(sock, AF_INET, 0, &route_cache) < 0) {
        std::cerr << "Failed to allocate route cache. Proceeding without deleting existing routes." << std::endl;
        route_cache = nullptr;
    }

    if (route_cache) {
        for (struct nl_object *obj = nl_cache_get_first(route_cache); obj != nullptr; obj = nl_cache_get_next(obj)) {
            struct rtnl_route *existing_route = (struct rtnl_route *)obj;
            struct nl_addr *existing_dst = rtnl_route_get_dst(existing_route);
            if (existing_dst && nl_addr_cmp(existing_dst, dst_addr) == 0) {
                int del_err = rtnl_route_delete(sock, existing_route, 0);
                if (del_err < 0 && del_err != -NLE_OBJ_NOTFOUND) {
                    std::cerr << "Failed to delete existing route: " << nl_geterror(del_err) << std::endl;
                }
            }
        }
    }

    // Trouver l'interface sur le même réseau que la gateway
    struct nl_cache *link_cache = nullptr;
    if (rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache) < 0) {
        std::cerr << "Failed to allocate link cache" << std::endl;
        if (route_cache) nl_cache_free(route_cache);
        nl_addr_put(dst_addr);
        nl_addr_put(gw_addr);
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }

    int ifindex = 0;
    struct nl_object *link_obj = nullptr;
    for (link_obj = nl_cache_get_first(link_cache); link_obj != nullptr; link_obj = nl_cache_get_next(link_obj)) {
        struct rtnl_link *link = (struct rtnl_link *)link_obj;
        int link_ifindex = rtnl_link_get_ifindex(link);

        // Récupérer les adresses IPv4 associées à cette interface
        struct nl_cache *addr_cache = nullptr;
        if (rtnl_addr_alloc_cache(sock, &addr_cache) < 0) continue;

        for (struct nl_object *addr_obj = nl_cache_get_first(addr_cache); addr_obj != nullptr; addr_obj = nl_cache_get_next(addr_obj)) {
            struct rtnl_addr *addr = (struct rtnl_addr *)addr_obj;
            if (rtnl_addr_get_ifindex(addr) != link_ifindex) continue;

            struct nl_addr *local_addr = rtnl_addr_get_local(addr);
            if (!local_addr || nl_addr_get_family(local_addr) != AF_INET) continue;

            // Calculer le masque réseau pour cette adresse
            int prefixlen = rtnl_addr_get_prefixlen(addr);

            // Vérifier si la gateway est dans ce subnet
            if (nl_addr_get_family(gw_addr) != AF_INET) continue;

            // Appliquer le masque sur les adresses pour comparer le réseau
            unsigned char gw_buf[4], local_buf[4];
            memcpy(gw_buf, nl_addr_get_binary_addr(gw_addr), 4);
            memcpy(local_buf, nl_addr_get_binary_addr(local_addr), 4);

            uint32_t gw_ip = ntohl(*((uint32_t*)gw_buf));
            uint32_t local_ip = ntohl(*((uint32_t*)local_buf));
            uint32_t mask = prefixlen == 0 ? 0 : (~0u << (32 - prefixlen));

            if ((gw_ip & mask) == (local_ip & mask)) {
                ifindex = link_ifindex;
                nl_cache_free(addr_cache);
                goto found_interface; // on sort des boucles
            }
        }
        nl_cache_free(addr_cache);
    }

found_interface:
    if (ifindex == 0) {
        std::cerr << "No interface found on the same network as gateway " << nextHop << std::endl;
        nl_cache_free(link_cache);
        if (route_cache) nl_cache_free(route_cache);
        nl_addr_put(dst_addr);
        nl_addr_put(gw_addr);
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }
    nl_cache_free(link_cache);

    // Créer la route et ajouter nexthop avec ifindex et gateway
    struct rtnl_route *route = rtnl_route_alloc();
    if (!route) {
        std::cerr << "Failed to allocate route" << std::endl;
        nl_addr_put(dst_addr);
        nl_addr_put(gw_addr);
        if (route_cache) nl_cache_free(route_cache);
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }
    rtnl_route_set_family(route, AF_INET);
    rtnl_route_set_dst(route, dst_addr);

    struct rtnl_nexthop *nh = rtnl_route_nh_alloc();
    if (!nh) {
        std::cerr << "Failed to allocate nexthop" << std::endl;
        nl_addr_put(dst_addr);
        nl_addr_put(gw_addr);
        rtnl_route_put(route);
        if (route_cache) nl_cache_free(route_cache);
        nl_close(sock);
        nl_socket_free(sock);
        return;
    }
    rtnl_route_nh_set_gateway(nh, gw_addr);
    rtnl_route_nh_set_ifindex(nh, ifindex);
    rtnl_route_add_nexthop(route, nh);

    int err = rtnl_route_add(sock, route, 0);
    if (err < 0) {
        std::cerr << "Failed to add route: " << nl_geterror(err) << std::endl;
    } else {
        std::cout << "Route added successfully" << std::endl;
    }

    // Nettoyage
    nl_addr_put(dst_addr);
    nl_addr_put(gw_addr);
    rtnl_route_put(route);
    if (route_cache) nl_cache_free(route_cache);
    nl_close(sock);
    nl_socket_free(sock);
}



void update_route(const std::string& destination, const std::string& nextHop) {
    delete_route(destination, nextHop); 
    add_route(destination, nextHop);
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

    //updateRoutingTable(buffer, local_lsdb);

    //computeShortestPaths(local_lsdb, LOCAL_ROUTER_ID);

    //updateForwardingTable();

    for (const auto& [destination, nextHop] : forwardingTable) {
        apply_route_to_system(destination, nextHop);
    }

    debug_known_router(local_lsdb);
}



// Corrected on_update function:
void on_update(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, const std::string& LOCAL_ROUTER_ID, std::vector<std::string>& interfaces)
{
    // 1. Update own router's declarations (this includes calling cleanup_old_declarations)
    //    This sets fresh timestamps for YOUR declarations in local_lsdb.
    update_lsdb(local_lsdb, LOCAL_ROUTER_ID);

    // 2. Now send all router declarations (your own will have fresh timestamps)
    send_all_router_declarations_to_all(local_lsdb, interfaces);

    // 3. Re-compute and apply routes based on the potentially updated LSDB
    std::vector<std::pair<std::string, std::string>> routes = compute_all_routes(LOCAL_ROUTER_ID, local_lsdb);
    for(const auto& route : routes)
    {
        // Consider if you need to delete_indirect_routes() here first
        // before adding new routes, to ensure consistency.
        add_route(route.second, route.first);
    }

    std::cout << "Periodic update task executed." << std::endl;
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

void read_config_file_mask(const std::string& filename, std::vector<std::string>& interfaces) {
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
                    interfaces.push_back(line);
                
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
    std::vector<std::string> interfaces_with_mask;

    // Read configuration file to get interfaces and debug output
    try {
        read_config_file("config", interfaces);
    } catch (const std::exception& ex) {
        std::cerr << "Error reading configuration file: " << ex.what() << std::endl;
        return 1;
    }
    debug_output_ips(interfaces);

    // Read configuration file to get interfaces and debug output
    try {
        read_config_file_mask("config", interfaces_with_mask);
    } catch (const std::exception& ex) {
        std::cerr << "Error reading configuration file: " << ex.what() << std::endl;
        return 1;
    }



    // Create default lsdb with is own declaration
    create_server_declaration(interfaces_with_mask, local_lsdb, LOCAL_ROUTER_ID);
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
