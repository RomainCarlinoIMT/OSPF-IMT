#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm> // For std::all_of
#include <limits>    // For std::numeric_limits

bool isValidRouterName(const std::string& router_name) 
{
    // Check if the input is not empty
    if(router_name.empty()) {
        printf("Router name cannot be empty.\n");
        return false;
    }

    // Assert first character is R
    if(router_name[0] != 'R') {
        printf("Router name must start with 'R'.\n");
        return false;
    }

    // Extract the numeric part after R
    std::string numeric_part = router_name.substr(1);

    // Check if the numeric part is not empty
    if(numeric_part.empty()) {
        printf("Router name must contain a numeric part after R.\n");
        return false;
    }

    // Check if all the numeric_part characters are digits
    if(!std::all_of(numeric_part.begin(), numeric_part.end(), ::isdigit)) {
        printf("Router name must contain only digits after R.\n");
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
            printf("Router number must be between 0 and 999999.\n");
            return false;
        }
    }
    catch (const std::out_of_range& e)
    {
        // Also this should not happen because limits is very far from 999999
        // But we handle it just in case ¯\_(ツ)_/¯
        printf("Router number is out of range.\n");
        return false;
    }
    catch (const std::invalid_argument& e)
    {
        // Normally this should not happen since we checked that all characters are digits
        // But we handle it just in case ¯\_(ツ)_/¯
        printf("Invalid router number format.\n");
        return false;
    }

    return true;
}

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


std::string create_router_definition(std::string router_name, std::string router_ip) 
{
    // Expected fomat: router_name:RXXXXX
    //               : router_ip x.x.x.x/m

   return "wip";
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

    std::cout << "\n--- Fin des Tests Unitaires Manuels ---" << std::endl;

    return 0;
}

