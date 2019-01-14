#include<fstream>
#include<iostream>
#include<string>
#include<sstream>
#include<ctime>
#include<chrono>
using namespace std;

namespace helpers {
    bool isAsciiNumber(char c);
    bool isValidFilePath(const string & filepath);
    string get_current_time();
}
