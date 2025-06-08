#include <iostream>
#include <string>
#include "logic.h"

void runIsValidRouterNameTest(const std::string& testName, const std::string& input, bool expectedResult) {
    bool actualResult = isValidRouterName(input);
    std::cout << "Test: " << testName << " (Input: \"" << input << "\") -> ";
    if (actualResult == expectedResult) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED! Expected: " << (expectedResult ? "Valid" : "Invalid")
                  << ", Got: " << (actualResult ? "Valid" : "Invalid") << std::endl;
    }
}

int main() {
    printf("This is the logic.cpp file. The main logic will be implemented in client.cpp.\n");
    printf("UNIT TESTS\n");
    printf("=== Testing isValidRouterName function: ===\n");

    // --- Cas valides ---
    runIsValidRouterNameTest("Valide_Min", "R0", true);
    runIsValidRouterNameTest("Valide_Max", "R999999", true);
    runIsValidRouterNameTest("Valide_PetitNombre", "R123", true);
    runIsValidRouterNameTest("Valide_GrosNombre", "R543210", true);
    runIsValidRouterNameTest("Valide_ZeroPad", "R00000", true); // Les zéros en tête sont valides pour stoi
    runIsValidRouterNameTest("Valide_UnSeulChiffre", "R5", true);

    std::cout << "\n--- Cas Invalides ---" << std::endl;

    // --- Cas invalides (structure) ---
    runIsValidRouterNameTest("Invalide_Vide", "", false);
    runIsValidRouterNameTest("Invalide_PasR", "A123", false);
    runIsValidRouterNameTest("Invalide_JusteR", "R", false); // Pas de partie numérique
    runIsValidRouterNameTest("Invalide_Espace", "R 123", false); // Mon code n'a pas vérifié ça, mais le vôtre oui.
    runIsValidRouterNameTest("Invalide_CaracteresNonNumeriques", "R12A3", false);
    runIsValidRouterNameTest("Invalide_SpecialChar", "R!23", false);

    // --- Cas invalides (plage numérique) ---
    runIsValidRouterNameTest("Invalide_TropGrand", "R1000000", false); // Un de plus que le max
    runIsValidRouterNameTest("Invalide_TropGrand_Beaucoup", "R9999999", false);
    runIsValidRouterNameTest("Invalide_Negatif", "R-1", false); // Not possible avec all_of et isdigit, mais bon à tester
    runIsValidRouterNameTest("Invalide_Negatif_String", "R-123", false); // Sera intercepté par all_of

    std::cout << "--- Validation CIDR ---" << std::endl;

    // Cas valides
    std::cout << "192.168.1.0/24: " << (assert_ip_and_mask("192.168.1.0/24") ? "Valide" : "Invalide") << std::endl;
    std::cout << "10.0.0.0/8: " << (assert_ip_and_mask("10.0.0.0/8") ? "Valide" : "Invalide") << std::endl;
    std::cout << "0.0.0.0/0: " << (assert_ip_and_mask("0.0.0.0/0") ? "Valide" : "Invalide") << std::endl;
    std::cout << "255.255.255.255/32: " << (assert_ip_and_mask("255.255.255.255/32") ? "Valide" : "Invalide") << std::endl;
    std::cout << "172.16.0.0/12: " << (assert_ip_and_mask("172.16.0.0/12") ? "Valide" : "Invalide") << std::endl;

    // Cas invalides
    std::cout << "\n--- Cas Invalides ---" << std::endl;
    std::cout << "InvalidCIDR: " << (assert_ip_and_mask("InvalidCIDR") ? "Valide" : "Invalide") << std::endl;
    std::cout << "192.168.1.0: " << (assert_ip_and_mask("192.168.1.0") ? "Valide" : "Invalide") << std::endl; // Pas de slash
    std::cout << "/24: " << (assert_ip_and_mask("/24") ? "Valide" : "Invalide") << std::endl; // Slash au début
    std::cout << "192.168.1.0/: " << (assert_ip_and_mask("192.168.1.0/") ? "Valide" : "Invalide") << std::endl; // Slash à la fin
    std::cout << "192.168.1.0/24/extra: " << (assert_ip_and_mask("192.168.1.0/24/extra") ? "Valide" : "Invalide") << std::endl; // Plusieurs slashs
    std::cout << "256.0.0.0/8: " << (assert_ip_and_mask("256.0.0.0/8") ? "Valide" : "Invalide") << std::endl; // Octet invalide
    std::cout << "192.168.1/24: " << (assert_ip_and_mask("192.168.1/24") ? "Valide" : "Invalide") << std::endl; // Pas 4 octets
    std::cout << "192.168.1.0/33: " << (assert_ip_and_mask("192.168.1.0/33") ? "Valide" : "Invalide") << std::endl; // Masque trop grand
    std::cout << "192.168.1.0/-1: " << (assert_ip_and_mask("192.168.1.0/-1") ? "Valide" : "Invalide") << std::endl; // Masque négatif
    std::cout << "192.168.1.0/abc: " << (assert_ip_and_mask("192.168.1.0/abc") ? "Valide" : "Invalide") << std::endl; // Masque non numérique
    std::cout << "192.168..1.0/24: " << (assert_ip_and_mask("192.168..1.0/24") ? "Valide" : "Invalide") << std::endl; // Doubles points
    std::cout << "192.168.1.A/24: " << (assert_ip_and_mask("192.168.1.A/24") ? "Valide" : "Invalide") << std::endl; // Caractère non numérique dans l'IP
    std::cout << "1.2.3.4.5/24: " << (assert_ip_and_mask("1.2.3.4.5/24") ? "Valide" : "Invalide") << std::endl; // Trop d'octets

    std::cout << "\n --- Testing serialize and deserialize functions ---" << std::endl;

    RouterDeclaration router = create_router_definition("R1", "10.0.1.1/24", 5);
    if (serialize_router_definition(router) == "{1,R1,10.0.1.1/24,5," + std::to_string(router.timestamp) + "}")
    {
        std::cout << "Serialization test passed!" << std::endl;
    } else {
        std::cout << "Serialization test failed!" << std::endl;
    }
    
    if (router == deserialize_router_definition(serialize_router_definition(router)))
    {
        std::cout << "Deserialization test passed!" << std::endl;
    } else {
        std::cout << "Deserialization test failed!" << std::endl;
    }

    std::cout << "\n--- Testing debug_known_router function ---" << std::endl;
    std::map<std::string, std::map<std::string, RouterDeclaration>> local_lsdb;
    local_lsdb["R1"]["10.0.1.0/24"] = create_router_definition("R1", "10.0.1.0/24", 10);
    local_lsdb["R1"]["10.0.2.0/24"] = create_router_definition("R1", "10.0.2.0/24", 20);
    local_lsdb["R1"]["192.168.1.0/24"] = create_router_definition("R1", "192.168.1.0/24", 5);

    local_lsdb["R2"]["10.0.3.0/24"] = create_router_definition("R2", "10.0.3.0/24", 30);
    local_lsdb["R2"]["10.0.4.0/24"] = create_router_definition("R2", "10.0.4.0/24", 40);

    local_lsdb["R3"]["172.16.0.0/16"] = create_router_definition("R3", "172.16.0.0/16", 15);


    debug_known_router(local_lsdb);

    std::cout << "\n--- Testing add_router_declaration function ---" << std::endl;
    std::map<std::string, std::map<std::string, RouterDeclaration>> lsdb;
    debug_known_router(lsdb);

    std::cout << "=== Testing add one" << std::endl;
    RouterDeclaration new_router = create_router_definition("R1", "10.0.1.1/24", 5);
    add_router_declaration(lsdb, new_router);
    debug_known_router(lsdb);

    std::cout << "=== Testing adding a second" << std::endl;
    RouterDeclaration new_router2 = create_router_definition("R1", "10.0.2.1/24", 10);
    add_router_declaration(lsdb, new_router2);
    debug_known_router(lsdb);

    std::cout << "=== Testing adding a third with same IP but different cost" << std::endl;
    RouterDeclaration new_router3 = create_router_definition("R1", "10.0.1.1/24", 15);
    new_router3.timestamp = new_router2.timestamp + 1000; // Ensure it's newer
    add_router_declaration(lsdb, new_router3);
    debug_known_router(lsdb);

    std::cout << "=== Testing adding a fourth with same IP but older timestamp" << std::endl;
    RouterDeclaration new_router4 = create_router_definition("R1", "10.0.1.1/24", 200);
    new_router4.timestamp = new_router2.timestamp - 1000; // Ensure it's older
    add_router_declaration(lsdb, new_router4);
    debug_known_router(lsdb);

    std::cout << "=== Testing adding a fifth with same IP but same cost and timestamp" << std::endl;
    RouterDeclaration new_router5 = create_router_definition("R1", "10.0.1.1/24",15);
    new_router5.timestamp = new_router2.timestamp; // Ensure it's the same
    add_router_declaration(lsdb, new_router5);
    debug_known_router(lsdb);
    
    std::cout  << "=== Testing adding a route with same info but newer timestamp" << std::endl;
    RouterDeclaration new_router6 = create_router_definition("R1", "10.0.1.1/24", 15);
    new_router6.timestamp = new_router2.timestamp + 1000; // Ensure it's newer
    add_router_declaration(lsdb, new_router6);
    debug_known_router(lsdb);

    std::cout << "=== Testing adding another router with different IP" << std::endl;
    RouterDeclaration new_router7 = create_router_definition("R2", "10.0.2.1/24", 20);
    add_router_declaration(lsdb, new_router7);
    debug_known_router(lsdb);

    std::cout << "\n--- Testing a base scenario with three routers ---" << std::endl;
    std::map<std::string, std::map<std::string, RouterDeclaration>> router_1_lsdb;
    std::map<std::string, std::map<std::string, RouterDeclaration>> router_2_lsdb;
    std::map<std::string, std::map<std::string, RouterDeclaration>> router_3_lsdb;
    
    RouterDeclaration router_1_declaration = create_router_definition("R1", "10.0.1.1/24", 5);
    RouterDeclaration router_2_declaration = create_router_definition("R2", "10.0.1.2/24", 5);
    RouterDeclaration router_2_1_declaration = create_router_definition("R2", "10.0.2.1/24",10);
    RouterDeclaration router_3_declaration = create_router_definition("R3", "10.0.2.2/24", 10);

    // Each router has its own LSDB
    add_router_declaration(router_1_lsdb, router_1_declaration);
    add_router_declaration(router_2_lsdb, router_2_declaration);
    add_router_declaration(router_2_lsdb, router_2_1_declaration);
    add_router_declaration(router_3_lsdb, router_3_declaration);

    // Router 2 send declaration to Router 1
    add_router_declaration(router_1_lsdb, router_2_declaration);
    add_router_declaration(router_1_lsdb, router_2_1_declaration);

    // Router 1 send declaration to Router 2
    add_router_declaration(router_2_lsdb, router_1_declaration);
    // Router 3 send declaration to Router 2
    add_router_declaration(router_2_lsdb, router_3_declaration);

    // Router 2 send declaration to Router 3
    add_router_declaration(router_3_lsdb, router_2_declaration);
    add_router_declaration(router_3_lsdb, router_2_1_declaration);

    std::cout << "=== R1 \n" << std::endl;
    debug_known_router(router_1_lsdb);
    std::cout << "=== R2\n" << std::endl;
    debug_known_router(router_2_lsdb);
    std::cout << "=== R3\n" << std::endl;
    debug_known_router(router_3_lsdb);

    // Router 2 send declaration to Router 1
    add_router_declaration(router_1_lsdb, router_2_declaration); // Same as before, should not change anything
    add_router_declaration(router_1_lsdb, router_2_1_declaration); // Same as before, should not change anything
    add_router_declaration(router_1_lsdb, router_3_declaration); // New declaration from Router 3

    // Router 1 send declaration to Router 2
    add_router_declaration(router_2_lsdb, router_1_declaration); // Same as before, should not change anything

    // Router 3 send declaration to Router 2
    add_router_declaration(router_2_lsdb, router_3_declaration); // Same as before, should not change anything

    // Router 2 send declaration to Router 3
    add_router_declaration(router_3_lsdb, router_2_declaration); // Same as before, should not change anything
    add_router_declaration(router_3_lsdb, router_2_1_declaration); // Same as before, should not change anything
    add_router_declaration(router_3_lsdb, router_1_declaration); // New declaration from Router 1

    std::cout << "=== R1 \n" << std::endl;
    debug_known_router(router_1_lsdb);
    std::cout << "=== R2\n" << std::endl;
    debug_known_router(router_2_lsdb);
    std::cout << "=== R3\n" << std::endl;
    debug_known_router(router_3_lsdb);





    std::cout << "\n--- Ends of unit tests ---" << std::endl;

    printf("\n\n ========== TEMPORARY TEST REMOVE IN PROD !!!\n\n");
    return 0;
}