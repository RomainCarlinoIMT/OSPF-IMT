#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <array>
#include <random>

// Execute une commande et récupère la sortie
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

// Liste toutes les interfaces sauf "lo"
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

// Applique la limitation tc avec débit aléatoire
void setRandomRate(const std::string& iface) {
    // Valeurs de débit possibles
    const std::vector<std::string> rates = {"1mbit", "5mbit", "10mbit", "50mbit"};

    // Générateur aléatoire
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, rates.size() - 1);

    std::string rate = rates[dis(gen)];
    std::string burst = "32kbit";
    std::string latency = "400ms";

    try {
        // Supprime la qdisc existante (ignore erreur)
        std::string cmdDel = "sudo tc qdisc del dev " + iface + " root || true";
        system(cmdDel.c_str());

        // Ajoute la nouvelle limitation
        std::string cmdAdd = "sudo tc qdisc add dev " + iface + " root tbf rate " + rate + " burst " + burst + " latency " + latency;
        int ret = system(cmdAdd.c_str());

        if (ret == 0) {
            std::cout << "[OK] Interface " << iface << " limitée à " << rate << std::endl;
        } else {
            std::cerr << "[Erreur] Commande échouée pour " << iface << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[Exception] " << e.what() << " sur interface " << iface << std::endl;
    }
}

int main() {
    std::vector<std::string> interfaces;
    try {
        interfaces = getNetworkInterfaces();
    } catch (const std::exception& e) {
        std::cerr << "Erreur récupération interfaces : " << e.what() << std::endl;
        return 1;
    }

    for (const auto& iface : interfaces) {
        setRandomRate(iface);
    }

    return 0;
}
