#include"jsondb.h"

jsondb::jsondb() {
    dbDirectory = DEFAULT_DB_DIR;
}

jsondb::jsondb(const string & dbDir) {
    dbDirectory = dbDir;
}
bool jsondb::createFile(const string & fileName) {
    fstream stream;
    stream.open(dbDirectory + "/" + fileName, ios::out);
    if (!stream) {
        perror("Error creating file for db");
        return false;
    }
    stream.close();
    return true;
}
bool jsondb::select(const string & selectFileName) {
    if (selectStream.is_open()) {
        saveSelectData(); // Save the current data before doing anything
        selectStream.close();
    }
    selectStream.open(dbDirectory + "/" + selectFileName);
    if (!selectStream) {
        cerr << "No valid filename: " << selectFileName << endl;
        return false;;
    }
    stringstream s;
    s << selectStream.rdbuf();
    if (!s.str().empty())
        cachedSelectJson = json::parse(s.str());
    currentSelectedFile = selectFileName;
    return true;
}

bool jsondb::findFile(const string & fileName) {
    const string str(dbDirectory + "/" + fileName);
    return helpers::isValidFilePath(str);
}

json& jsondb::getSelectData() {
    return cachedSelectJson;
}

bool jsondb::saveSelectData() {
    selectStream.close();
    selectStream.open(dbDirectory + "/" + currentSelectedFile);
    if (!selectStream) {
        cerr << "No valid filename " << currentSelectedFile << " to save" << endl;
        return false;
    }
    string str = cachedSelectJson.dump();
    selectStream << str;
    if (!selectStream.good()) {
        cerr << "Error writing to file " << currentSelectedFile << endl;
        return false;
    }
    return true;
}

bool jsondb::deleteSelectData() {
    if (remove ((dbDirectory + "/" + currentSelectedFile).c_str()) != 0) {
        perror("Error deleting file");
        return false;
    }
    return true;
}
