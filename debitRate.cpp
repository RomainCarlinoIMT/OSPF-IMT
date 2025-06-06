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

// Récupère les noms des interfaces réseau
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

int main() {
    std::vector<std::string> interfaces;
    try {
        interfaces = getNetworkInterfaces();
    } catch (const std::exception& ex) {
        std::cerr << "Erreur lors de la récupération des interfaces : " << ex.what() << std::endl;
        return 1;
    }

    std::regex rateRegex(R"(rate\s+(\d+[KMG]?bit))");

    for (const auto& iface : interfaces) {
        std::string cmd = "tc qdisc show dev " + iface;
        std::string tcOutput;

        try {
            tcOutput = execCommand(cmd.c_str());
        } catch (const std::exception& ex) {
            std::cerr << "Erreur sur l'interface " << iface << " : " << ex.what() << std::endl;
            continue;
        }

        std::smatch match;
        if (std::regex_search(tcOutput, match, rateRegex)) {
            std::cout << "[ " << iface << " ] -> Débit configuré : " << match[1] << std::endl;
        } else {
            std::cout << "[ " << iface << " ] -> Aucun débit (rate) trouvé." << std::endl;
        }
    }

    return 0;
}
