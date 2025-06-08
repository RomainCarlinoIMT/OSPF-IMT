#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm> // For std::all_of
#include <limits>    // For std::numeric_limits

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


std::string create_router_definition(std::string router_name, std::string ip_with_mask) 
{
    // Expected fomat: router_name:RXXXXX
    //               : router_ip x.x.x.x/m

   return "wip";
}
