#include"verify.h"

// A set of methods to return true if all the info is there for a valid header, and false otherwise

bool verify::verify_status_message(const json & j) {
    return j.find(STATUS) != j.end() && j.find(TIMESTAMP) != j.end()
        && j.find(USERNAME) != j.end() && j.find(OTHER_MESSAGE) != j.end();
}

bool verify::verify_create_group_message(const json & j) {
    return j.find(REQUEST) != j.end() &&
        j.find(TIMESTAMP) != j.end() &&
        j.find(GROUPNAME) != j.end() &&
        j.find(USER_LIST) != j.end() &&
        j.find(OTHER_MESSAGE) != j.end();
}
