#include"log.h"
#include"jsondb.h"
#include"pass.h"
int main() {
    jsondb mydb("/home/joey/chatapp/testDB");
    mydb.createFile("joey");
    mydb.select("joey");
    json & j = mydb.getSelectData();
    j["username"] = "joey";
    j["salt"] = pass::salt(32);
    j["hash"] = pass::hashPass("123456", j["salt"]);
    mydb.saveSelectData();
    std::string str;
    while (true) {
        std::cin >> str;
        // Get the pass
        // Then check to see if it is the same
        if (pass::passCompare(pass::hashPass(str, j["salt"]), j["hash"])) {
            std::cout << "Right password!" << std::endl;
            break;
        }
        else {
            std::cout << "Wrong password! Try again!" << std::endl;
        }
    }
}
