#include"pass.h"

char pass::genRandHexChar() {
    unsigned r = rand() % 16;
    if (r >= 0 || r <= 9) {
        return '0' + r;
    }
    else {
        return 'a' + r - 10;
    }
}

std::string pass::salt(unsigned amount) {
    stringstream s;
    for (unsigned i = 0; i < amount; i++) {
        s << getRandHexChar();
    }
    return s.str();
}
std::string pass::hashPass(std::string pass, std::string salt) {
    return sha256(salt + pass);
}

bool pass::passCompare(std::string hash1, std::string hash2) {
    return hash1.compare(hash2) == 0;
}
