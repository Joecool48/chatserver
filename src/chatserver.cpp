#include"chatserver.h"

Server::Server () {
    epollNum = epoll_create(0);
    current_id = 0;
    if (epollNum == -1) {
        perror("epoll_create");
        exit(1);
    }
}
void Server::start(char *ip, int32_t listenPort) {
    struct epoll_event ev; // Epoll event for epoll_ctl, and events array for epoll_wait
    int nfds = 0; // Number of file descriptors from epoll
    struct addrinfo hints, *result; // Hints and result struct for getaddrinfo before server bind
    int connect_sock = -1; // For accepting sockets
    struct sockaddr addr; // For collecting accepted socket info
    socklen_t addrlen= 0; // Len of connected socket address

    result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int s = getaddrinfo(NULL, ip == NULL ? DEFAULT_IP_ADDRESS : ip, hints, &result);
    if (s != 0) {
        perror("getaddrinfo");
        exit(1);
    }
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        perror("sockete");
        exit(1);
    }

    if (bind(serverSock, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind");
        exit(1);
    }
    // Store the ip address
    addr = result->ai_addr;
    if (isValidPort(listenPort)) {
        portNum = listenPort;
    }
    else {
        portNum = DEFAULT_PORT;
        server_log(SERVER_WARNING, "Port %d is invalid, switching to default port %d", listenPort, DEFAULT_PORT);
    }

    if (listen(serverSock, MAX_LISTEN_QUEUE) != 0) {
        perror("listen");
        exit(1);
    }
    server_log(SERVER_INFO, "Server listening on port %d, address %s", server->portNum, addr.sa_data);

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
    if (nfds == -1) {
        perror("epoll_wait");
        exit(1);
    }
    // Go through all the events
    for (int i = 0; i < nfds; i++) {
        // If the listen socket has data ready and there are too many clients, deny the request
        if (events[i].data.fd == serverSock && clientMap.size() + 1 > MAX_CLIENTS) {
            // Accept, and then close the connection
            connect_sock = accept(serverSock, (struct sockadd *) addr, &addrlen);
            if (connect_sock == -1) {
                perror("accept");
                exit(1);
            }
            close(connect_sock);
        }
        // Otherwise accept, retrieve data, and add to event poll and map
        else if (events[i].data.fd == serverSock) {
            connect_sock = accept(serverSock, (struct sockaddr *) addr, &addrlen);
            if (connect_sock == -1) {
                perror("accept");
                exit(1);
            }
            server_log(SERVER_INFO, "Server connected with socket number %d", connect_sock);
            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = connect_sock;
            // Add the new client to the epoll event listener
            if (epoll_ctl(epollNum, EPOLL_CTL_ADD, connect_sock, &ev) == -1) {
                perror("epoll_ctl: connect_sock");
                exit(1);
            }
            // Add the client to the epoll map
            Client * newClient = new Client(current_id++, connect_sock);
            clientMap[connect_sock] = newClient; // Insert into map to keep track of currently connected clients
        }
        // Else a socket must have sent data
        else {
            Client * client = clientMap.find(events[i].data.fd);
            if (client == NULL) {
                server_log(SERVER_ERROR, "Unable to find socket number %d in clientmap, ignoring client", events[i].data.fd);
                continue;
            }
            // Get the data that the client sent
            json client_message_data = get_sent_json_data(client);
            // Skip cause an error occurred
            if (client_message_data.empty()) continue;
            if (client->username.empty()) {
                
            }
        }
    }
}

json Server::get_sent_json_data(json & j, Client * client) {
    string len = "";
    stringstream jsonToParse;
    char buff[BUFF_LEN];
    int numBytes = recv(client->socketFd, buff, 1);
    if (numBytes == -1) {
        perror("recv");
        exit(1);
    }
    while(numBytes != 1 && isAsciiNumber(buff[0])) {
        len += ("" + buff[0]);
        numBytes = recv(client->sockFd, buff, 1);
        if (numBytes == -1) {
            perror("recv");
            exit(1);
        }
    }
    // If is a newline, then it is a correctly formatted header. Retreive the json contents
    if (buff[0] == '\n') {
        server_log(SERVER_INFO, "Received message of length %s from socket %d", len.c_str(), client->sockFd);
    }
    long len = strtol(len.c_str());
    if (len == 0) {
        server_log(SERVER_ERROR, "Error, malformatted header, error %d.", errno);
        // What to do when malformed header. Dump contents?
    }
    // Get the json contents of the header.
    do {
        int numBytesToGet = (len - BUFF_LEN >= 0) ? BUFF_LEN : len;
        numBytes = recv(client->socketFd, buff, numBytesToGet);
        if (numBytes == -1) {
            perror("recv");
            exit(1);
        }
        // Client must have disconnected due to some abnormal reason
        else if (numBytes == 0) {
            removeClient(client->sockFd);
        }
        len -= numBytes;
        str = buff;
        jsonToParse << str;
    } while (len != 0);
    try {
        json j = json::parse(jsonToParse.str());
        return j;
    }
    catch (exception e) {
        server_log("An exception occurred when parsing the json for socket %d", client->socketFd);
        return json();
    }
}

void Client::Client(ClientId id, int clientSock) {
    socketFd = clientSock;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = CLIENT_DATA_TIMEOUT_USEC;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv));
    this->id = id;
    username = "";
    lastConnectTime = time(NULL); // Mark this as the time they last connected
}
