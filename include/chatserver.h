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
#include"jsondb.h"
#include"log.h"
#include"client.h"
using namespace std;
using namespace nlohmann;
using namespace helpers;

#define DEFAULT_LOG_DIR "/home/joey/chatapp/log"
#define MAX_EVENTS 30
#define MAX_CLIENTS 3000
#define DEFAULT_LOG_FILE "log.txt"
#define DEFAULT_IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 2222
#define MAX_LISTEN_QUEUE 30
#define EVENT_TIMEOUT 1000 // In us
#define BUFFER_LEN 300
class Server {
 public:
    Server();
    void run(char * ip, int listenPort);
    ~Server();
    json get_sent_json_data(Client * client);
 private:
    void get_client_first_connect_data(Client * client);
    void check_events();
    bool isValidPort(int port);
    void removeClient(int key);
    unordered_map<int, Client*> clientMap;
    int serverSock;
    struct sockaddr addr;
    int portNum;
    int epollNum;
    int logfileFd;
    bool loggingEnabled;
    jsondb db;
    struct epoll_event events[MAX_EVENTS];
};
