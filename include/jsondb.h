#include<string>
#include"helpers.h"
#include"json.hpp"
#include<stdio.h>
#include<sstream>
using namespace std;
using nlohmann::json;
class jsondb {
 public:
    jsondb();
    jsondb(const string & dbDir);
    bool select(const string & selectFileName); // Select the next db file
    bool findUser(const string & fileName);
    json& getSelectData();
    bool createFile(const string & fileName);
    bool saveSelectData(); // Returns true if save was successful
    bool deleteSelectData();
 private:
    fstream selectStream;
    const string DEFAULT_DB_DIR = "~/jsondb/";
    string dbDirectory;
    string currentSelectedFile;
    json cachedSelectJson;
};
