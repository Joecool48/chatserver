#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<stdbool.h>
#include<time.h>
#include<unordered_map>
#include<errno.h>
using namespace std;

#define CLIENT_DATA_TIMEOUT_USEC 100000

typedef uint64_t ClientId;

class Client {
 public:
    Client(int clientSock);
    const string& getUsername();
    void setUsername(const string & newUsername);
    int getSocketNum();
    void setSocketNum(int sockNum);
 private:
    static ClientId current_id;
    int socketFd;
    ClientId id;
    string username;
};
