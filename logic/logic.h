#ifndef LOGIC_H
#define LOGIC_H

#include <string>
#include <map>
#include <vector>

// Defining routerDecllaration struc
struct RouterDeclaration {
    std::string router_name; // Name of the router
    std::string ip_with_mask; // IP address with subnet mask
    int link_cost; // Cost of the link to this router
    long long timestamp; // Timestamp of the declaration

    bool operator==(const RouterDeclaration& other) const 
    {
        return (router_name == other.router_name &&
                ip_with_mask == other.ip_with_mask &&
                link_cost == other.link_cost &&
                timestamp == other.timestamp);
    }
};

bool isValidRouterName(const std::string& router_name);
bool assert_ip_and_mask(const std::string& ip_with_mask) ;
RouterDeclaration create_router_definition(std::string router_name, std::string ip_with_mask, int link_cost);
std::string serialize_router_definition(RouterDeclaration router_declaration);
RouterDeclaration deserialize_router_definition(const std::string& definition);
void debug_known_router(std::map<std::string, std::map<std::string, RouterDeclaration>> local_lsdb);
bool add_router_declaration(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, const RouterDeclaration& new_declaration);
bool cleanup_old_declarations(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, long long threshold_ms);
void update_lsdb(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, const std::string& ROUTER_ID);
std::string get_network_address(const std::string& ip_with_mask);
std::vector<std::string> get_all_subnets(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb);
std::vector<std::string> get_all_routers(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb);
std::vector<std::string> get_all_nodes(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb);
void display_matrix(const std::vector<std::vector<int>>& matrix);
std::vector<std::vector<int>> create_n_by_n_matrix(int n);
void add_router_declaration_to_matrix(RouterDeclaration& declaration, std::vector<std::vector<int>>& matrix, std::vector<std::string>& all_nodes );
void build_matrix_from_lsbd(std::vector<std::vector<int>>& matrix, std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, std::vector<std::string>& all_nodes);
int dijkstraNextHop(const std::vector<std::vector<int>>& adjMatrix, int start, int target);

#endif // LOGIC_H