#ifndef VERIFY_H
#define VERIFY_H

#include<string>
#include"json.hpp"

using namespace nlohmann;

namespace verify {
    bool verify_status_message(const json & j);
    bool verify_create_group_message(const json & j);
    bool verify_create_account(const json & j);
    bool verify_login_request(const json & j);
};













#endif /* VERIFY_H */
