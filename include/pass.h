//Helper functions for dealing with encryption and decryption
#include<string>
#include<sstream>
namespace pass {
    std::string salt(unsigned amount);
    bool passCompare(std::string hash1, std::string hash2) // Compares 2 hashed passwords to see if equal;
    std::string hashPass(std::string pass); // Prepends the salt, and hashes using sha256
    char genRandHexChar();
}
