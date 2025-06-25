#ifndef MSG_H
#define MSG_H

#include <string> // For std::string
#include <map>    // For std::map
#include <vector> // For std::vector
#include "../logic/logic.h" // For RouterDeclaration

int send_message(const std::string& message, const std::string& interface_ip);
bool send_all_router_declarations_to_all(const std::map<std::string, std::map<std::string, RouterDeclaration>>& local_lsdb, const std::vector<std::string>& interfaces);

#endif // MSG_H