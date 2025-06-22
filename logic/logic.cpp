#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm> // For std::all_of
#include <limits>    // For std::numeric_limits
#include <stdexcept> // For std::out_of_range and std::invalid_argument
#include <chrono>  // For getting the current time
#include <sstream> // For std::stringstream
#include <vector>  // For std::vector
#include <map>     // For std::map
#include <set>
#include <bits/stdc++.h>
#include <queue>
#include <limits>


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
                link_cost == other.link_cost); // Timestamp is not included in the comparison, as it is expected to change with each declaration
    }
};

bool isValidRouterName(const std::string& router_name) 
{
    // Check if the input is not empty
    if(router_name.empty()) {
        // printf("Router name cannot be empty.\n");
        return false;
    }

    // Assert first character is R
    if(router_name[0] != 'R') {
        // printf("Router name must start with 'R'.\n");
        return false;
    }

    // Extract the numeric part after R
    std::string numeric_part = router_name.substr(1);

    // Check if the numeric part is not empty
    if(numeric_part.empty()) {
        // printf("Router name must contain a numeric part after R.\n");
        return false;
    }

    // Check if all the numeric_part characters are digits
    if(!std::all_of(numeric_part.begin(), numeric_part.end(), ::isdigit)) {
        // printf("Router name must contain only digits after R.\n");
        return false;
    }

    // Converting the numeric part to an integer
    try
    {
        int router_number = std::stoi(numeric_part); // std::stoi make string to int conversion

        // Check if the router number is within the valid range
        if(router_number >= 0 && router_number <= 999999) {
            return true; // Valid router name
        } else {
            // printf("Router number must be between 0 and 999999.\n");
            return false;
        }
    }
    catch (const std::out_of_range& e)
    {
        // Also this should not happen because limits is very far from 999999
        // But we handle it just in case ¯\_(ツ)_/¯
        // printf("Router number is out of range.\n");
        return false;
    }
    catch (const std::invalid_argument& e)
    {
        // Normally this should not happen since we checked that all characters are digits
        // But we handle it just in case ¯\_(ツ)_/¯
        // printf("Invalid router number format.\n");
        return false;
    }

    return true;
}

bool isValidOctet(const std::string& octet) 
{
    // assert octet is not empty and has a length between 1 and 3
    if (octet.empty() || octet.length() > 3) 
    {
        // printf("Invalid octet: '%s'. It must be between 1 and 3 digits long.\n", octet.c_str());
        return false; // Un octet vide ou trop long
    }

    // assert all characters in the octet are digits (But we already did this in is_valid_IPv4)
    if (!std::all_of(octet.begin(), octet.end(), ::isdigit)) {
        // printf("Invalid octet: '%s'. It must contain only digits.\n", octet.c_str());
        return false;
    }

    // Convert the octet to an integer and check if it is in the valid range
    try
    {
        int octetValue = std::stoi(octet); // Convert string to int
        return (octetValue >= 0 && octetValue <= 255); // Valid range for an IPv4 octet is 0 to 255
    }
    catch (const std::out_of_range& e)
    {
        // printf("Invalid octet: '%s'. It is out of range.\n", octet.c_str());
        return false; // Octet is out of range
    }
    catch (const std::invalid_argument& e)
    {
        // printf("Invalid octet: '%s'. Invalid format.\n", octet.c_str());
        return false; // Invalid format
    }
}

bool is_valid_IPv4(const std::string& ip) 
{
    if (ip.empty()) {
        // // printf("IP address cannot be empty.\n");
        return false;
    }

    std::string currentOctet;
    int dotCount = 0;

    for (char c : ip)
    {
        if (c == '.') 
        {
            dotCount++;
            if (dotCount > 3) 
            { // Trop de points
                return false;
            }
            if (!isValidOctet(currentOctet)) 
            {
                // // printf("Invalid octet: '%s'.\n", currentOctet.c_str());
                return false;
            }
            currentOctet.clear(); // Clear for the next octet
        } 
        else if (::isdigit(c))
        {
            currentOctet += c;
        }
        else
        {
            // printf("Invalid character '%c' in IP address.\n", c);
            return false; // A non-digit character found
        }
    }

    // Validete the last octet
    if(!isValidOctet(currentOctet)) 
    {
        // printf("Invalid last octet: '%s'.\n", currentOctet.c_str());
        return false;
    }

    // Last check we need exactly 3 dots for a valid IPv4 address
    return dotCount == 3;
}

bool assert_ip_and_mask(const std::string& ip_with_mask) 
{
    // Test string is not empty
    if(ip_with_mask.empty()) {
        return false;
    }

    // Check if ther's a slash in the string and also if it is the first character or the last character
    size_t slash_pos = ip_with_mask.find('/');
    if(slash_pos == std::string::npos || slash_pos == 0 || slash_pos == ip_with_mask.length() - 1) {
        // printf("Invalid IP with mask format. Slash must be in the middle.\n");
        return false; // No slash found or slash is at the beginning or end
    }

    // Assert there's only one slash
    if(ip_with_mask.find('/', slash_pos + 1) != std::string::npos) { // search at index + 1 for before to assert
        // printf("Invalid IP with mask format. Only one slash is allowed.\n");
        return false; // More than one slash found
    }

    // Split the string into IP and mask
    std::string ip_part = ip_with_mask.substr(0, slash_pos); // Using the slash position got before
    std::string mask_part = ip_with_mask.substr(slash_pos + 1);

    // Check if the IP part is valid
    if(!is_valid_IPv4(ip_part))
    {
        // printf("Invalid IP address: '%s'.\n", ip_part.c_str());
        return false; // Invalid IP address
    }

    // Assert mask is not empty
    if(mask_part.empty())
    {
        // printf("Mask cannot be empty.\n");
        return false; // Mask is empty
    }

    // Assert mask is only digits
    if(!std::all_of(mask_part.begin(), mask_part.end(), ::isdigit))
    {
        // printf("Mask must contain only digits.\n");
        return false; // Mask contains non-digit characters
    }

    // Testing range of the mask
    try
    {
        int maskBits = std::stoi(mask_part); // Convert mask to integer
        return (maskBits >= 0 && maskBits <= 32); // Valid range for IPv4 subnet mask is 0 to 32
    }
    catch (const std::out_of_range& e)
    {
        // printf("Mask is out of range.\n");
        return false; // Mask is out of range
    }
    catch (const std::invalid_argument& e)
    {
        // printf("Invalid mask format.\n");
        return false; // Invalid mask format
    }
}


RouterDeclaration create_router_definition(std::string router_name, std::string ip_with_mask, int link_cost) 
{
    // Expected fomat: router_name:RXXXXX
    //               : router_ip x.x.x.x/m
    //               : link_cost 0-9999

    std::string definition;

    // Getting the actuall timestamp in miliseonds to string
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // filling the definition struct
    RouterDeclaration routerDeclaration;
    routerDeclaration.router_name = router_name;
    routerDeclaration.ip_with_mask = ip_with_mask;
    routerDeclaration.link_cost = link_cost;
    routerDeclaration.timestamp = timestamp;

    return routerDeclaration;
}

std::string serialize_router_definition(RouterDeclaration router_declaration) 
{
    std::string definition;

    // Getting the actuall timestamp in miliseonds to string
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    definition += "{1," + router_declaration.router_name + "," + router_declaration.ip_with_mask + "," + 
                  std::to_string(router_declaration.link_cost) + "," + std::to_string(router_declaration.timestamp) + "}";
    
    return definition;
}

RouterDeclaration deserialize_router_definition(const std::string& definition) 
{
    RouterDeclaration declaration;

    // Example definition: {1,R123,10.0.1.1/24,5,1700000000000}

    // Assert expression is not empty and starts with '{' and ends with '}'
    if(definition.empty() || definition.front() != '{' || definition.back() != '}')
    {
        throw std::invalid_argument("Invalid router definition format. Missing { or } or empty.");
    }

    std::string content = definition.substr(1, definition.length() - 2); // Remove the { and }

    // Diving the content into segements
    std::stringstream ss(content);
    std::string segment;
    std::vector<std::string> segments;

    while (std::getline(ss, segment, ','))
    {
        segments.push_back(segment);
    }

    if(segments.size() != 5) 
    {
        throw std::invalid_argument("Invalid router definition format. Expected 5 segments.");
    }

    // Convert each segment to the appropriate type and fill the declaration struct
    try
    {
        if (segments[0] != "1") 
        {
            throw std::invalid_argument("Invalid router definition format. First segment must be '1'.");
        }
        
        declaration.router_name = segments[1]; // Router name
        declaration.ip_with_mask = segments[2]; // IP with mask
        declaration.link_cost = std::stoi(segments[3]); // Link cost
        declaration.timestamp = std::stoll(segments[4]); // Timestamp
    }
    catch (const std::invalid_argument& e) 
    {
        throw std::runtime_error("Deserialization error: invalid argument in conversion. " + std::string(e.what()));
    } 
    catch (const std::out_of_range& e) 
    {
        throw std::runtime_error("Deserialization error: numeric value out of range. " + std::string(e.what()));
    } 
    catch (const std::exception& e) 
    {
        throw std::runtime_error("Deserialization error: " + std::string(e.what()));
    }

    return declaration;
}

bool is_router_declaration_newer(RouterDeclaration& existing, RouterDeclaration& new_declaration) 
{
    // Note don't check if this is the same packer type, because other can't fit in RouterDeclaration

    if(existing == new_declaration) 
    {
        return false; // Same declaration, no need to update
    }

    // Assert if all fields are the same except timestamp
    if (existing.router_name != new_declaration.router_name ||
        existing.ip_with_mask != new_declaration.ip_with_mask ||
        existing.link_cost != new_declaration.link_cost)
    {        
        return false; // Not the same router name or IP or link cost
    }

    // If we reach here, it means the router name, IP and link cost are the same, but the timestamp is different
    if (new_declaration.timestamp <= existing.timestamp)
    {
        // printf("New declaration is not newer than the existing one.\n");
        return false; // New declaration is not newer than the existing one
    }

    return true; // New declaration is newer than the existing one
}

void debug_known_router(std::map<std::string, std::map<std::string, RouterDeclaration>> local_lsdb)
{
    std::cout << "Known routers:" << std::endl;
    for (const auto& [router_name, router_data] : local_lsdb) 
    {
        std::cout << "Router: " << router_name << std::endl;
        for (const auto& [ip_mask, declaration] : router_data) 
        {
            std::cout << "  IP/Mask: " << declaration.ip_with_mask 
                      << ", Link Cost: " << declaration.link_cost 
                      << ", Timestamp: " << declaration.timestamp 
                      << std::endl;
        }
    }
    std::cout << "Total routers: " << local_lsdb.size() << std::endl;
}

bool add_router_declaration(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb,
     const RouterDeclaration& new_declaration) 
{
    // Remeber keys of the request
    const std::string& router_name = new_declaration.router_name;
    const std::string& ip_with_mask = new_declaration.ip_with_mask;

    // Check if the router already exists in the local_lsdb
    auto router_it = local_lsdb.find(router_name);

    if(router_it == local_lsdb.end())
    {
        // Case were the router does't exist in the local_lsdb
        // So we add it instantly
        local_lsdb[router_name][ip_with_mask] = new_declaration;
        // Return it directly to spedd up convergence
        return true; // Router added successfully 
    }
    else
    {
        // Case were router is already in the local_lsdb
        // Getting all the declarations for this router
        std::map<std::string, RouterDeclaration>& router_links_map = router_it->second;

        auto link_it = router_links_map.find(ip_with_mask);

        if(link_it == router_links_map.end())
        {
            // Case were the link does't exist for this router
            router_links_map[ip_with_mask] = new_declaration;
            return true; // Router link added successfully
        }
        else
        {
            // Case were the link alrzady exists for this router
            const RouterDeclaration& existing_declaration = link_it->second;

            if(new_declaration.timestamp > existing_declaration.timestamp)
            {
                // Case were the new declaration is newer than the existing one

                router_links_map[ip_with_mask] = new_declaration; // Update the existing declaration
                return true; // Mark the LSDB as updated
            }
        }
    }

    return false; // No update made
}

bool cleanup_old_declarations(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, long long threshold_ms) 
{
    bool cleaned = false;
    long long current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Create the vector to store routers to remove
    std::vector<std::string> routers_to_remove;

    auto router_it = local_lsdb.begin();
    while(router_it != local_lsdb.end())
    {
        std::string router_name = router_it->first;
        std::map<std::string, RouterDeclaration>& router_links_map = router_it->second;

        auto link_it = router_links_map.begin();
        while(link_it != router_links_map.end())
        {
            const RouterDeclaration& declaration = link_it->second;

           long long declaration_age = current_time - declaration.timestamp;

            if(declaration_age > threshold_ms) 
            {
                // If the declaration is older than the threshold, remove it
                link_it = router_links_map.erase(link_it); // Erase returns the next iterator
                cleaned = true; // Mark that we cleaned something
            } 
            else 
            {
                ++link_it; // Move to the next link
            }
        }

        // If the router has no links left, remove the router itself
        if(router_links_map.empty()) 
        {
            routers_to_remove.push_back(router_name); // Store the router name to remove later
            router_it = local_lsdb.erase(router_it); // Erase returns the next iterator
        } 
        ++router_it; // Move to the next router
    }
    
    // Remove routers that have no links left
    for(const std::string& router_name : routers_to_remove) 
    {
        local_lsdb.erase(router_name); // Remove the router from the local_lsdb
        cleaned = true; // Mark that we cleaned something
    }
    return cleaned; // Return true if any declarations were cleaned up
}

void update_lsdb(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, 
                 const std::string& ROUTER_ID) 
{
    // Firstly remove old declarations
    long long threshold_ms = 30000; // 30 seconds threshold for old declarations
    bool cleaned = cleanup_old_declarations(local_lsdb, threshold_ms);
    if(cleaned) 
    {
        std::cout << "Old declarations cleaned up." << std::endl;
    } 
    else 
    {
        std::cout << "No old declarations to clean up." << std::endl;
    }
    // Now parse the message and update the LSDB to update this router own declaration
    auto it_my_declaration = local_lsdb.find(ROUTER_ID); // Assuming that ROUTER_ID is the name of the router
    
    if(it_my_declaration == local_lsdb.end())
    {
        // Extremely rare case, maybe configuration error or Fatal error is comming
        return; // No declaration for this router, nothing to update
    }

    std::map<std::string, RouterDeclaration>& my_declarations = it_my_declaration->second;

    // Generate the new timestamp for the router declaration
    long long new_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    for (auto& pair : my_declarations) 
    {
        printf("Updating declaration for router: %s info %s\n", pair.second.router_name.c_str(), pair.second.ip_with_mask.c_str());
        RouterDeclaration& declaration = pair.second;
        declaration.timestamp = new_timestamp; // Update the timestamp for each declaration
        // Note: We don't update the router_name, ip_with_mask, and link_cost here. This should be done only when the router declaration is received from another router.
    }    
}


// Fonction to convert ip to an uint
// Assuming that passed ip is correct
uint32_t ip_to_uint(const std::string& ip_str)
{
    uint32_t ip_int = 0;
    std::stringstream ss(ip_str);
    std::string octet_str;

    for(int i = 0; i < 4; ++i)
    {
        if(!std::getline(ss, octet_str, '.'))
        {
            throw std::invalid_argument("Invalid IP format in ip_to_uint: " + ip_str);
        }
        int octet = std::stoi(octet_str);
        if (octet < 0 || octet > 255)
        {
            throw std::invalid_argument("Invalid octet value in ip_to_uint: " + octet_str);
        }
        ip_int = (ip_int << 8) | octet;
    }
    return ip_int;
}

// Fonction used to convert uint back to ip string
std::string uint_to_ip(uint32_t ip_int)
{
    std::string ip_str;
    for (int i = 3; i >= 0; i--) // inverted iteration !!!
    {
        ip_str += std::to_string( ( ip_int >> (i * 8) ) & 0xFF );
        if (i > 0)
        {
            ip_str += ".";
        }
    }
    return ip_str;
}


// Fonction used to convert the received pair router ip + mask to the network address
std::string get_network_address(const std::string& ip_with_mask)
{
    // Starting by checking if ip_with_mask is correct
    if(!assert_ip_and_mask(ip_with_mask))
    {
        throw std::invalid_argument("Invalid IP with mask" + ip_with_mask);
    }

    // Above function should avoid those lines to thorw errors
    size_t slash_pos = ip_with_mask.find('/');
    std::string ip_part = ip_with_mask.substr(0, slash_pos);
    std::string mask_bits_str = ip_with_mask.substr(slash_pos + 1);

    int mask_bits = std::stoi(mask_bits_str);
    if (mask_bits < 0 || mask_bits > 32)
    {
        throw std::invalid_argument("Invalid mask bits: " + mask_bits_str);
    }

    // Convert the ip to uint
    uint32_t ip_int = ip_to_uint(ip_part);

    // Creating the binary mask
    uint32_t binary_mask = (mask_bits == 0) ? 0 : (0xFFFFFFFF << (32 - mask_bits));

    // Then thansk to the properties of ip and mask just make an and gate
    uint32_t network_int = ip_int & binary_mask;
    std::string network_ip_str = uint_to_ip(network_int);
    return network_ip_str + "/" + mask_bits_str;
}

std::vector<std::string> get_all_subnets(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb)
{
    std::set<std::string> unique_subnets; // This ensure that subnet is only added one time
    
    // Iterater lsdb
    for (const auto& router_entry : local_lsdb)
    {        
        for (const auto& declaration_entry : router_entry.second)
        {
            try
            {
                std::string network_address = get_network_address(declaration_entry.second.ip_with_mask);
                
                unique_subnets.insert(network_address);
            }
            catch (const std::invalid_argument& e)
            {
                std::cerr << "Warning: Could not get network address for " 
                          << declaration_entry.second.ip_with_mask << ": " << e.what() << std::endl;
            }
        }
    }
    // this bring back to_a_vector
    return std::vector<std::string>(unique_subnets.begin(), unique_subnets.end());
}

std::vector<std::string> get_all_routers(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb)
{
    // Ensure that there's not data duplication
    std::set<std::string> unique_router_names;

    // Iterating the lsdb
    for (const auto& router_entry : local_lsdb)
    {
        // The router id is present as it !
        unique_router_names.insert(router_entry.first);
    }

    // Bring it back to an vector which is easier to manipulate
    return std::vector<std::string>(unique_router_names.begin(), unique_router_names.end());
}

// Function to get all nodes for Dijkstra
std::vector<std::string> get_all_nodes(std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb)
{
    std::vector<std::string> routers = get_all_routers(local_lsdb);
    std::vector<std::string> subnets = get_all_subnets(local_lsdb);

    std::vector<std::string> all_nodes;

    for (const std::string& router_name : routers)
    {
        all_nodes.push_back(router_name);
    }

    for (const std::string& subnet_address : subnets)
    {
        all_nodes.push_back(subnet_address);
    }

    return all_nodes;
}

const int NO_LINK = -1; // Used to infrom that there's not link

// Function to display the cost matrix
void display_matrix(const std::vector<std::vector<int>>& matrix) // Pass by const reference for efficiency
{
    std::cout << "--- Matrix Content ---" << std::endl;
    if (matrix.empty()) {
        std::cout << "Matrix is empty." << std::endl;
        return;
    }

    for(const auto& row : matrix)
    {
        for(const auto& cell : row)
        {
            std::cout << cell << "\t\t";
        }
        std::cout << std::endl;
    }
    std::cout << "----------------------" << std::endl;
}

// Function to create a void n*n matrix (to avoid core_dump)
std::vector<std::vector<int>> create_n_by_n_matrix(int n)
{
    std::vector<std::vector<int>> matrix(n, std::vector<int>(n, -1)); // Whe -1 to inform that's unrechable
    return matrix;
}

// Function to adde one router declaration to matrix
void add_router_declaration_to_matrix(RouterDeclaration& declaration, std::vector<std::vector<int>>& matrix, std::vector<std::string>& all_nodes )
{
    std::string start_node = declaration.router_name;
    std::string end_node = get_network_address(declaration.ip_with_mask);
    int cost = declaration.link_cost; // Note don't forget the COST !!!!

    auto start_it = std::find(all_nodes.begin(), all_nodes.end(), start_node);
    auto end_it = std::find(all_nodes.begin(), all_nodes.end(), end_node);

    // Assert results are correct even if it should happend !
    if (start_it == all_nodes.end() || end_it == all_nodes.end()) {
        std::cerr << "Erreur : nœud non trouvé dans all_nodes" << std::endl;
        return;
    }

    int start_index = std::distance(all_nodes.begin(), start_it);
    int end_index = std::distance(all_nodes.begin(), end_it);

    // We have a not oriented graphe so each link is duplicated in the matrix !
    matrix[start_index][end_index] = cost;
    matrix[end_index][start_index] = cost;
}

void build_matrix_from_lsbd(std::vector<std::vector<int>>& matrix,
    std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb,
    std::vector<std::string>& all_nodes)
{
    for (auto& router_entry : local_lsdb) 
    {
        for (auto& declaration_item : router_entry.second) 
        {
            RouterDeclaration& declaration = declaration_item.second;
            add_router_declaration_to_matrix(declaration, matrix, all_nodes);
        }
    }
}

const int INF = std::numeric_limits<int>::max();

std::pair<int, int> dijkstraNextHop(const std::vector<std::vector<int>>& adjMatrix, int start, int target) {
    int n = adjMatrix.size();
    std::vector<int> dist(n, INF);
    std::vector<int> parent(n, -1);
    std::vector<bool> visited(n, false);

    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<>> pq;

    dist[start] = 0;
    pq.emplace(0, start);

    while (!pq.empty()) {
        auto [currentDist, u] = pq.top();
        pq.pop();

        if (visited[u]) continue;
        visited[u] = true;

        if (u == target) break;

        for (int v = 0; v < n; ++v) {
            int weight = adjMatrix[u][v];
            if (weight != -1 && !visited[v]) {
                if (dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                    parent[v] = u;
                    pq.emplace(dist[v], v);
                }
            }
        }
    }

    // Si on n'a jamais atteint le target
    if (dist[target] == INF)
        return std::make_pair(-1, -1);

    // Reconstruire le chemin de target vers start
    int current = target;
    std::vector<int> path;
    while (current != -1) {
        path.push_back(current);
        current = parent[current];
    }

    std::reverse(path.begin(), path.end());

    // Si le chemin a au moins 2 nœuds, le next hop est le 2e
    if (path.size() >= 3)
        // return path[2];
        return std::make_pair(path[1], path[2]);
    else
        return std::make_pair(-1, -1);; // start == target
}

// Used at the end diskstra because only subnets are used so information is losts
std::string get_router_ip_on_network(std::string& router_name, std::string& network_address, std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb)
{
    // assert if the router exist (even if at this point this is very strange)
    auto it_router = local_lsdb.find(router_name); // Filter with only the route we want to find
    if (it_router == local_lsdb.end()) 
    {
        return "";
    }

    for (const auto& ip_decl_pair : it_router->second) {
        const RouterDeclaration& declaration = ip_decl_pair.second;
        
        // Compute de the address to see if it's the right
        std::string declared_network_address = get_network_address(declaration.ip_with_mask);
        if (declared_network_address == network_address) {
            return declaration.ip_with_mask;
        }
    }

    // This never should happend !!!
    return "";
}


std::vector<std::pair<std::string, std::string>> compute_all_routes(std::string actual_router, std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb)
{
    std::vector<std::string> all_nodes = get_all_nodes(local_lsdb);
    std::vector<std::string> all_subnets = get_all_subnets(local_lsdb);
    std::vector<std::vector<int>> matrix = create_n_by_n_matrix(all_nodes.size());

    std::vector<std::pair<std::string, std::string>> res;

    build_matrix_from_lsbd(matrix, local_lsdb, all_nodes);

    auto it_1 = std::find(all_nodes.begin(), all_nodes.end(), actual_router);
    int routeur_id = std::distance(all_nodes.begin(), it_1);

    

    for(const std::string& destination_subnet : all_subnets)
    {
        auto it_2 = std::find(all_nodes.begin(), all_nodes.end(), destination_subnet);
        int subnet_id = std::distance(all_nodes.begin(), it_2);    

        std::pair<int, int> dijkstra_res = dijkstraNextHop(matrix, routeur_id, subnet_id);

        if(dijkstra_res.first != -1)
        {
            std::string nexthop_subnet = all_nodes[dijkstra_res.first];
            std::string nexthop_routeur_name = all_nodes[dijkstra_res.second];

            std::string next_router_ip = get_router_ip_on_network(nexthop_routeur_name, nexthop_subnet, local_lsdb);
            size_t slash_pos = next_router_ip.find("/");
            next_router_ip = next_router_ip.substr(0, slash_pos);

            std::cout << "To reach : " << destination_subnet << " use " << next_router_ip << std::endl;            
            res.push_back({next_router_ip, destination_subnet});

        }
        else
        {
            std::cout << "To reach : " << destination_subnet << " it's directly connected ! " << std::endl; 
        }        
    }

    return res;
}