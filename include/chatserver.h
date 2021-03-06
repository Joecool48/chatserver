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
#include"json.hpp"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>


#include"log.h"
#include"client.h"
#include"request_codes.h"
using namespace std;
using namespace nlohmann;
using namespace helpers;

#define CHATAPP_DB_NAME "chatappDB"
#define CHATAPP_USERS_COLLECTION_NAME "users"
#define CHATAPP_METADATA_COLLECTION_NAME "chatappMetadata"
#define CHATAPP_METADATA_DOCUMENT_NAME "appdata"
#define CHATAPP_GROUP_COLLECTION_NAME "groups"


#define DEFAULT_LOG_DIR "/home/joey/chatapp/log"
#define MAX_EVENTS 30
#define MAX_CLIENTS 3000
#define DEFAULT_LOG_FILE "log.txt"
#define DEFAULT_IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 2222
#define MAX_LISTEN_QUEUE 30
#define EVENT_TIMEOUT 1000 // In us
#define BUFFER_LEN 300

#define SALT_AMOUNT 128
class Server {
 public:
    Server();
    void run(char * ip, int listenPort);
    ~Server();
    json get_sent_json_data(Client * client);
    void send_json_data(Client * client, const json & j);
 private:
    void get_client_first_connect_data(Client * client);
    void check_events();
    bool isValidPort(int port);
    void removeClient(int key);

    // Request processing functions
    void process_request_salt(Client * client, json & clientMessage);
    void process_user_login_request(Client * client, json & clientMessage);
    int process_send_message(Client * client, json & clientMessage);
    int process_request_message_log(Client * client, json & clientMessage);
    void process_user_exit(Client * client, json & clientMessage);
    void process_user_create_account(Client * client, json & clientMessage);
    int process_create_group_request(Client * client, json & clientMessage);
    int process_add_user_to_group_request(Client * client, json & clientMessage);
    int process_remove_user_from_group_request(Client * client, json & clientMessage);
    // Database vars and functions
    void initDB();
    long getNextGroupId();
    long getNextUserId();
    int userExistsInDB(const string & username);
    mongocxx::instance dbInstance;
    mongocxx::client dbClient;
    mongocxx::database db;
    mongocxx::collection usersCollection;
    mongocxx::collection metadataCollection;
    mongocxx::document metadataDocument;
    void send_status(Client * client, int status); // For server communicating updates to client
    int createUserDBEntry(const string & username, const string & hashPassword, const string & salt); // returns the user id
    int createGroupDBEntry(const string & groupname, const string & ownername); // Returns the group id
    int addUserToGroup(long userId, long groupId);
    int removeUserFromGroup(long userId, long groupId);
    unordered_map<int, Client*> clientMap;
    int serverSock;
    struct sockaddr addr;
    int portNum;
    int epollNum;
    int logfileFd;
    bool loggingEnabled;
    struct epoll_event events[MAX_EVENTS];
};
