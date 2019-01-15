#include"chatserver.h"

Server::Server () {
    epollNum = epoll_create(0);
    if (epollNum == -1) {
        perror("epoll_create");
        exit(1);
    }
    initDB();
}

void Server::initDB() {
    dbInstance = mongocxx::instance{};
    dbClient = mongocxx::client(mongocxx::uri());
    db = dbClient[CHATAPP_DB_NAME];
    usersCollection = db[CHATAPP_USERS_COLLECTION_NAME];
    groupCollection = db[CHATAPP_GROUP_COLLECTION_NAME];
    metadataCollection = db[CHATAPP_METADATA_COLLECTION_NAME];
    metadataDocument = metadataCollection.find_one(document{} << DOCNAME << CHATAPP_METADATA_DOCUMENT_NAME << finalize);
    // If meta data document doesnt exist, then create it
    if(!metadataDocument) {
        metadataCollection.insert_one(document{} << DOCNAME << CHATAPP_METADAT_DOCUMENT_NAME << GROUP_ID << 0 << finalize);
    }
}

long Server::getNextGroupId() {
    mongocxx::document::view doc = metadataCollection.find_one(document{} << DOCNAME << CHATAPP_METADATA_DOCUMENT_NAME << finalize);
    long id = doc[CURRENT_GROUP_ID];
    metadataCollection.update_one(document{} << DOCNAME << CHATAPP_METADATA_DOCUMENT_NAME << finalize, document{} << "$set" << open_document << CURRENT_GROUP_ID << (id + 1) << close_document << finalize);
    return id;
}

long Server::getNextUserId() {
    mongocxx::document::view doc = metadataCollection.find_one(document{} << DOCNAME << CHATAPP_METADATA_DOCUMENT_NAME << finalize);
    long id = doc[CURRENT_USER_ID];
    metadataCollection.update_one(document{} << DOCNAME << CHATAPP_METADATA_DOCUMENT_NAME << finalize, document{} << "$set" << open_document << CURRENT_USER_ID << (id + 1) << close_document << finalize);
    return id;
}

void Server::createUserDBEntry(const string & user, const string & hashPassword, const string & salt ) {
    // Create new user document
    auto builder = bsoncxx::builder::stream::document{}; // Username should always match doc mane
    long id = getNextUserId();
    bsoncxx::document::value doc = builder
        << USERNAME << user
        << USER_ID << id
        << HASHPASSWORD << hashPassword
        << SALT << salt
        << LAST_LOGIN_TIME << ""
        << LAST_LOGOUT_TIME << ""
        << CURRENTLY_LOGGED_IN << false
        << GROUPS << bsoncxx::builder::stream::open_array << bsoncxx::builder::stream::close_array
        << bsoncxx::builder::stream::finalize;
    usersCollection.insert_one(doc); // Add the user to the usersCollection
    return id;

}

int Server::createGroupDBEntry(const string & groupname, const string & owner_name) {
    auto builder = bsoncxx::builder::string::document{};
    long id = getNextGroupId();
    bsoncxx::document::value doc = builder
        << GROUP_OWNER << owner_name
        << GROUPNAME << groupname
        << GROUP_ID << id
        << USER_LIST << open_array << close_array
        << MESSAGE_LOG << open_array << close_array
        << finalize;
    groupCollection.insert_one(doc);
    return id;
}

int Server::addUserToGroup(long userId, long groupId) {
    // First query to see if the group exists
    bsoncxx::stdx::optional<bsoncxx::document::value> groupRes = groupCollection.find_one(document{} << GROUP_ID << groupId << finalize);
    if (!groupRes) return -1; // Group was not found, so couldn't delete user
    bsoncxx::stdx::optional<bsoncxx::document::value> userRes = userCollection.find_one(document{} << USER_ID << userId << finalize);
    if (!userREs) return -1;
    // Subscribe the user to the group
    groupCollection.update_one(document{} << GROUP_ID << groupId << finalize, document{} << "$push" << open_document << USER_LIST << userId << close_document << finalize);

    userCollection.update_one(document{} << USER_ID << userId << finalize, document{} << "$push" << open_document << GROUPS << groupId << close_document << finalize);
    return 0; // success
    
}

int Server::removeUserFromGroup(long userId, long groupId) {
    bsoncxx::stdx::optional<bsoncxx::document::value> groupRes = groupCollection.find_one(document{} << GROUP_ID << groupId << finalize);
    if (!groupRes) return -1; // Group was not found, so couldn't delete user
    bsoncxx::stdx::optional<bsoncxx::document::value> userRes = userCollection.find_one(document{} << USER_ID << userId << finalize);
    if (!userREs) return -1;
    // Delete the userId from the user list array
    groupCollection.update_one(document{} << GROUP_ID << groupId << finalize, document <<  "$pull" << open_document << USER_LIST << userId << close_document << finalize);

    userCollection.update_one(document{} << USER_ID << userId << finalize, document{} << "$pull" << open_document << GROUPS << groupId << close_document << finalize);
    return 0;


}

void Server::run(char *ip, int listenPort) {
    struct epoll_event ev; // Epoll event for epoll_ctl, and events array for epoll_wait
    int nfds = 0; // Number of file descriptors from epoll
    struct addrinfo hints, *result; // Hints and result struct for getaddrinfo before server bind
    int connect_sock = -1; // For accepting sockets
    struct sockaddr addr; // For collecting accepted socket info
    socklen_t addrlen= 0; // Len of connected socket address
    Log::init_log(Log::Severity::LOG_INFO);
    result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int s = getaddrinfo(NULL, ip == NULL ? DEFAULT_IP_ADDRESS : ip, &hints, &result);
    if (s != 0) {
        perror("getaddrinfo");
        exit(1);
    }
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        perror("socket");
        exit(1);
    }

    if (bind(serverSock, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind");
        exit(1);
    }
    // Store the ip address
    addr = *(result->ai_addr);
    if (isValidPort(listenPort)) {
        portNum = listenPort;
    }
    else {
        portNum = DEFAULT_PORT;
        Log::log_dbg(Log::Severity::LOG_WARNING, "Port " + to_string(listenPort) + "is invalid, switching to default port " + to_string(DEFAULT_PORT));
    }

    if (listen(serverSock, MAX_LISTEN_QUEUE) != 0) {
        perror("listen");
        exit(1);
    }
    Log::log_dbg(Log::Severity::LOG_INFO, "Server listening on port " + to_string(portNum) + ", address " + addr.sa_data);

    // Set up epoll for listener socket
    ev.events = EPOLLIN;
    ev.data.fd = serverSock;

    if (epoll_ctl(epollNum, EPOLL_CTL_ADD, serverSock, &ev) == -1) {
        perror("epoll_ctl: serverSock");
        exit(1);
    }
    // Main server loop
    for (;;) {
        check_events();
    }
}




void Server::check_events() {
// search all the epoll fds
    int nfds = epoll_wait(epollNum, events, MAX_EVENTS, EVENT_TIMEOUT);
    struct sockaddr addr;
    socklen_t addrlen;
    struct epoll_event ev;
    int connect_sock;
    if (nfds == -1) {
        perror("epoll_wait");
        exit(1);
    }
    // Go through all the events
    for (int i = 0; i < nfds; i++) {
        // If the listen socket has data ready and there are too many clients, deny the request
        if (events[i].data.fd == serverSock && clientMap.size() + 1 > MAX_CLIENTS) {
            // Accept, and then close the connection
            connect_sock = accept(serverSock, (struct sockaddr *) &addr, &addrlen);
            if (connect_sock == -1) {
                perror("accept");
                exit(1);
            }
            close(connect_sock);
        }
        // Otherwise accept, retrieve data, and add to event poll and map
        else if (events[i].data.fd == serverSock) {
            connect_sock = accept(serverSock, (struct sockaddr *) &addr, &addrlen);
            if (connect_sock == -1) {
                perror("accept");
                exit(1);
            }
            Log::log_dbg(Log::Severity::LOG_INFO, "Server connected with socket number " + to_string(connect_sock));
            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = connect_sock;
            // Add the new client to the epoll event listener
            if (epoll_ctl(epollNum, EPOLL_CTL_ADD, connect_sock, &ev) == -1) {
                perror("epoll_ctl: connect_sock");
                exit(1);
            }
            // Add the client to the epoll map
            Client * newClient = new Client(connect_sock);
            clientMap[connect_sock] = newClient; // Insert into map to keep track of currently connected clients
        }
        // Else a socket must have sent data
        else {
            // Should always find the data
            if (clientMap.find(events[i].data.fd) == clientMap.end()) {
                Log::log_dbg(Log::Severity::LOG_ERROR, "Unable to find socket number" + to_string(events[i].data.fd) + "in clientmap, ignoring client");
                continue;
            }
            Client * client = clientMap[events[i].data.fd];
            // Get the data that the client sent
            json client_message_data = get_sent_json_data(client);
            // Skip cause an error occurred
            if (client_message_data.empty()) continue;
            // Check all the possible request codes
            if (client_message_data.find(REQUEST) == client_message_data.end()) continue;

            switch (client_message_data[REQUEST]) {
            case REQUEST_SALT_VALUE_FROM_SERVER:
                process_request_salt(client, client_message_data);
                break;
            case USER_LOGIN_REQUEST:
                process_user_login_request(client, client_message_data);
                break;
            case CREATE_ACCOUNT_REQUEST:
                process_user_create_account(client, client_message_data);
                break;
            case CREATE_GROUP_REQUEST:
                process_create_group_request(client, client_message_data);
                break;
            case ADD_USER_TO_GROUP_REQUEST:
                process_add_user_to_group_request(client, client_message_data);
                break;
            case REMOVE_USER_FROM_GROUP_REQUEST:
                process_remove_user_from_group_request(client, client_message_data);
                break;
            case SEND_MESSAGE:
                process_send_message(client, client_message_data);
                break;
            case REQUEST_MESSAGE_LOG:
                process_request_message_log(client, client_message_data);
                break;
            case USER_EXIT:
                process_user_exit(client, client_message_data);
                break;
            default:
                // Tell the client the request isn't recognized
                send_status(client, ERR_INVALID_REQUEST);
            }
        }
    }
}

json Server::get_sent_json_data(Client * client) {
    string len = "";
    stringstream jsonToParse;
    char buff[BUFFER_LEN];
    int numBytes = recv(client->getSocketNum(), buff, 1, 0);
    if (numBytes == -1) {
        perror("recv");
        exit(1);
    }
    while(numBytes != 1 && isAsciiNumber(buff[0])) {
        len += ("" + buff[0]);
        numBytes = recv(client->getSocketNum(), buff, 1, 0);
        if (numBytes == -1) {
            perror("recv");
            exit(1);
        }
    }
    // If is a newline, then it is a correctly formatted header. Retreive the json contents
    if (buff[0] == '\n') {
        Log::log_dbg(Log::Severity::LOG_INFO, "Received message of length" + len + "from socket " + to_string(client->getSocketNum()));
    }
    else {
        Log::log_dbg(Log::Severity::LOG_ERROR, "Improperly formatted header");
    }
    // Convert to decimal and no endptr, so second arg is null
    long chunkLen = strtol(len.c_str(), NULL, 10);
    if (chunkLen == 0) {
        Log::log_dbg(Log::Severity::LOG_ERROR, "Error, malformatted header, error " + to_string(errno));
        // What to do when malformed header. Dump contents?
    }
    // Get the json contents of the header.
    do {
        int numBytesToGet = (chunkLen - BUFFER_LEN - 1 >= 0) ? BUFFER_LEN - 1 : chunkLen;
        numBytes = recv(client->getSocketNum(), buff, numBytesToGet, 0);
        buff[numBytesToGet + 1] = '\0';
        if (numBytes == -1) {
            perror("recv");
            exit(1);
        }
        // Client must have disconnected due to some abnormal reason
        else if (numBytes == 0) {
            removeClient(client->getSocketNum());
        }
        chunkLen -= numBytes;
        string str = buff;
        jsonToParse << str;
    } while (chunkLen != 0);
    try {
        json j = json::parse(jsonToParse.str());
        return j;
    }
    catch (exception e) {
        Log::log_dbg(Log::Severity::LOG_ERROR, "An exception occurred when parsing the json for socket" + to_string(client->getSocketNum()));
        return json();
    }
}


bool Server::isValidPort(int port) {
    return port >= 0 && port <= 65535;
}
// Function to cleanup all the clients resources, and show that it disconnected TODO possible extra cleanup needed
void Server::removeClient(int clientSockFd) {
    clientMap.erase(clientSockFd);
}

void Server::process_request_salt(Client * client, json & clientMessage) {
    // It is trusted that username is a string
    if (clientMessage.find(USERNAME) == clientMessage.end()) {
        send_status(client, ERR_NEED_MORE_LOGIN_INFO);
    }
    // User must exist, and therefore have a salt
    string salt = "";
    bsoncxx::stdx::optional<bsoncxx::document::value> res = usersCollection.find_one(document{} << USERNAME << clientMessage[USERNAME]);
    // Salt exists
    if (res) {
        salt = res[SALT];
    }
    // Otherwise, must create a salt because first time user entered db fov
    else {
        salt = pass::salt(SALT_AMOUNT);
    }
    json j;
    j[REQUEST] = SERVER_SEND_SALT;
    j[SALT] = salt;
    send_json_data(client, j);
}

void process_user_login_request(Client * client, json & clientMessage) {
    if (!verify::verify_login_request(clientMessage)) {
        send_status(client, ERR_NEED_MORE_LOGIN_INFO);
    }
    client->username = clientMessage[USERNAME];
    // Query a document that has the right salt, hash, and username. If one exists, then they login successfully
    bsoncxx::stdx::optional<bsoncxx::document::value> res = usersCollection.find_one(document{} << USERNAME << clientMessage[USERNAME] << SALT << clientMessage[SALT] << HASHPASSWORD << clientMessage[HASHPASSWORD]);
    // Logged in successfully
    if (res) {
        usersCollection.update_one(document{} << USERNAME << clientMessage[USERNAME] << finalize, document{} << "$set" << open_document << CURRENTLY_LOGGED_IN << true << LAST_LOGIN_TIME << helpers::get_current_time() << close_document << finalize);
        send_status(client, USER_LOGIN_INFO_FOUND);
    }
    else {
        send_status(client, ERR_USER_LOGIN_INFO_NOT_FOUND);
    }
}

void Server::process_user_create_account(Client * client, json & clientMessage) {
    if (userExistsInDB(clientMessage[USERNAME])) {
        send_status(client, ERR_USER_NAME_TAKEN);
        return;
    }
    if (!verify::verify_create_account(clientMessage)){
        send_status(client, ERR_NEED_MORE_LOGIN_INFO);
        return;
    }
    // All the data must exist, so we can just create the user entry
    createUserDBEntry(clientMessage[USERNAME], clientMessage[HASHPASSWORD],
                      clientMessage[SALT]);
}

int Server::process_create_group_request(Client * client, json & clientMessage) {
    // Not enough info
    if (!verify::verify_create_group_message(clientMessage)) {
        Log::log_dbg(Log::Severity::LOG_ERROR, "Invalid json request for creating a group");
        send_status(client, ERR_INVALID_JSON_MESSAGE);
        return -1;
    }
    long group_id = createGroupDBEntry(clientMessage[GROUPNAME], clientMessage[USERNAME]);
    vector<long> user_ids = clientMessage[USER_LIST];
    for (auto it = user_ids.begin(); it != user_ids.end(); it++) {
        addUserToGroup(*it, group_id); // Add every user to the group that is listed
    }
    return 0;
}

int Server::process_add_user_to_group_request(Client * client, json & clientMessage) {
    
}

bool Server::userExistsInDB(const string & username) {
    bsoncxx::stdx::optional<bsoncxx::document::value> res = userCollection.find_one(document{} << USERNAME << username << finalize);
    if (res) return true;
    return false;
}

void send_json_data(Client * client, const json & j) {
    string jstring = j.dump();
    stringstream res;
    // construct the object
    res << jstring.length() << '\n' << jstring;
    char buff[BUFFER_LEN];
    int totalBytesSent = 0;
    string sendString = res.str();
    // Send total string in chunks until it all goes through
    while (totalBytesSent < sendString.length()) {
        int numBytes = send(client->socketFd, sendString.c_str() + totalBytesSent, sendString.length() - totalBytesSent >= BUFFER_LEN ? BUFFER_LEN : sendString.length() - totalBytesSent);
        if (numBytes == -1) {
            perror("send");
            exit(EXIT_FAILURE);
        }
        if (numBytes > 0) {
            totalBytesSent += numBytes;
        }
    }
}

void Server::send_status(Client * client, int status) {
    json j;
    j[USERNAME] = client->username;
    j[TIMESTAMP] = helpers::get_current_time();
    j[STATUS] = status;
    string message;
    switch (status) {
    case ERR_USER_MUST_LOGIN_FIRST:
        message = "User cannot do anything without logging in";
    case ERR_INVALID_REQUEST:
        message = "Request number not recognized";
    case ERR_NEED_MORE_LOGIN_INFO:
        message = "Not enough login info provided";
    case ERR_USER_NAME_TAKEN:
        message = "Another user already has this username. Try a different one";
    case ERR_INVALID_JSON_MESSAGE:
        message = "Cannot interpret JSON contents. Please send another message";
    default:
        message = "No additional info";
    }
    j[OTHER_MESSAGE] = message;
    send_json_data(client, j);
}
