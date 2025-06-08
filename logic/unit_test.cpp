#include <iostream>
#include <string>
#include logic

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