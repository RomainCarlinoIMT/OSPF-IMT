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
                  std::to_string(router_declaration.link_cost) + "," + std::to_string(timestamp) + "}";
    
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