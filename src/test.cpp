#include"log.h"
#include"jsondb.h"
#include"sha256.h"
int main() {
    /*jsondb mydb("/home/joey/chatapp/testDB");
    mydb.createFile("joey");
    mydb.select("joey");
    json & j = mydb.getSelectData();
    for (int i = 0; i < 100; i++) {
        mydb.createFile("joey" + to_string(i));
        mydb.select("joey" + to_string(i));
        json & j = mydb.getSelectData();
        j["username"] = "joey" + to_string(i);
        j["messages"] = {"Hi", "good", "Memes"};
        mydb.saveSelectData();
        }*/
    string hash = sha256("MyBall");
    cout << hash << endl;
}
