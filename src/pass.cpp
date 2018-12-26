#include"pass.h"

char pass::genRandHexChar() {
    unsigned r = rand() % 16;
}

std::string pass::salt(unsigned amount) {
    stringstream s;
    for (unsigned i = 0; i < amount; i++) {
        
    }
}
