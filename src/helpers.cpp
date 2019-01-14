#include"helpers.h"

bool helpers::isAsciiNumber(char c) {
    return c >= '0' && c <= '9';
}
bool helpers::isValidFilePath(const string & filepath) {
    fstream stream;
    stream.open(filepath);
    if (!stream) {
        return false;
    }
    return true;
}

string helpers::get_current_time() {
    time_t t = time(NULL);
    stringstream s;
    s << put_time(localtime(&t), "%c");
    return s.str();
}
