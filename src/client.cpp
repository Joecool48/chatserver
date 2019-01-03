#include "client.h"

Client::Client(int clientSock) {
    socketFd = clientSock;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = CLIENT_DATA_TIMEOUT_USEC;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv));
    this->id = Client::current_id++;
    username = "";
    lastConnectTime = time(NULL); // Mark this as the time they last connected
}
const string & Client::getUsername() {
    return username;
}
int Client::getSocketNum() {
    return socketFd;
}
void Client::setUsername(const string & str) {
    username = str;
}
void Client::setSocketNum(int sockNum) {
    socketFd = sockNum;
}

ClientId Client::current_id = 0;
