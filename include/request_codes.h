/*
  This file contains all the codes for the different pieces of data that are exchanged between client and server. Everything goes to the server, and there are no direct client to client interactions. Personal preferences are stored client side.
*/
#include<string>


// User collection strings
const std::string USERNAME = "username";
const std::string USER_ID = "user_id";
const std::string HASHPASSWORD = "hash";
const std::string SALT = "salt";
const std::string GROUPS = "groups"; // List of group ID's the user is currently in
const std::string LAST_LOGIN_TIME = "last_login";
const std::string LAST_LOGOUT_TIME = "last_logout";
const std::string CURRENTLY_LOGGED_IN = "logged_in";

// Group collection strings
const std::string USER_LIST = "user_list"; // List of users in the group by id
const std::string GROUP_ID = "group_id";
const std::string GROUPNAME = "group_name";

// Metadata collection strings
const std::string CURRENT_GROUP_ID = "current_group_id"; // Rolling count of IDs used so far. Simply increments every time a new user or group is added
const std::string CURRENT_USER_ID = "current_user_id";
const std::string DOCNAME = "name";

// Message strings
const std::string REQUEST = "request";

#ifndef REQUEST_CODES_H
#define REQUEST_CODES_H

enum CLIENT_CODES {
                   REQUEST_SALT_VALUE_FROM_SERVER, /* Client asking for server salt attached to username */
                   USER_LOGIN_REQUEST, /* User requesting login with username and hashed password */
                   SEND_MESSAGE, /* User sent a message to a single person, or group */
                   REQUEST_MESSAGE_LOG, /* User requests its previous certain amount of messages from the server */
                   USER_EXIT, /* Nice way to tell server that user exited. If connection drops server also knows client exited */
                   CREATE_ACCOUNT_REQUEST, /* User asks server to create a new user with hash password, salt value, and username and add it to the database */
                   CREATE_GROUP_REQUEST
};

enum SERVER_CODES {
                   SERVER_SEND_SALT, /* Server found the username, and sent the requested salt */
                   USER_LOGIN_INFO_FOUND, /* Found user account with hash and username mentioned. Login was successful */
                   ERR_USER_LOGIN_INFO_NOT_FOUND, /* Server couldn't find that account with the username and hash mentioned in request salt */
                   MESSAGE_LOG_CONTENTS, /* Server found and is returning past certain amount of messages in their log */
                   FORWARD_MESSAGE, /* Server giving message to correct recipient */
                   ERR_USER_MUST_LOGIN_FIRST,
                   ERR_INVALID_REQUEST,
                   ERR_NEED_MORE_LOGIN_INFO,
                   ERR_USER_NAME_TAKEN
};










#endif
