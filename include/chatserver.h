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
using namespace std;

typedef ClientId uint64_t;

class Client {
public:
    Client(ClientId id, int clientSock);
    const string& getUsername();
    void setUsername(const string & newUsername);
private:
    int socketFd;
    ClientId id;
    time_t lastConnectTime;
    time_t lastDisconnectTime;
    string username;
};

#define MAX_EVENTS 30
#define MAX_CLIENTS 3000
#define DEFAULT_LOG_FILE "log.txt"
#define DEFAULT_IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 2222
#define MAX_LISTEN_QUEUE 30
#define EVENT_TIMEOUT 1000 // In us
#define BUFFER_LEN 300
#define CLIENT_DATA_TIMEOUT_USEC 100000
class Server {
 public:
    static ClientId current_id = 0;
    Server();
    void start(string ip, int listenPort);
    void halt();
    ~Server();
    void get_sent_json_data(json & j, Client * client);
 private:
    void get_client_first_connect_data(Client * client);
    void check_events();
    unordered_map<int, Client*> clientMap;
    int serverSock;
    struct sockaddr addr;
    int portNum;
    int epollNum;
    int logfileFd;
    bool loggingEnabled;
    struct epoll_events events[MAX_EVENTS];
}
