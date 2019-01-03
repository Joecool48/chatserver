#include"chatserver.h"

Server::Server () {
    epollNum = epoll_create(0);
    if (epollNum == -1) {
        perror("epoll_create");
        exit(1);
    }
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
            if (client->getUsername().empty()) {
                // Get the username
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
// Function to cleanup all the clients resources, and show that it disconnected
void Server::removeClient(int clientSockFd) {
    clientMap.erase(clientSockFd);
}
