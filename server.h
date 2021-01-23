#ifndef SERVER_H
#define SERVER_H

#include "epoll.h"
#include <iostream>
#include <string>
#include <cstring>
#include <map>
// #include <list>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctime>

#define BUFFSIZE 0xffff // 缓冲区大小

struct client_info{
    int connfd;
    char *join_time;
    bool isNickNameSet;

    std::string client_host;
    std::string client_port;
    std::string client_name;
};

class Server{
private:
    int listenfd;
    int epfd;
    char *current_time;
    struct sockaddr_in serv;

    // std::list<int> clients_list;
    std::map<int, client_info> clients_map;

    void remove_client(int connfd);
    int ShowUserstoClient(int connfd);
    int BroadcastMessage(int connfd);
    // int PrivateMessage(int connfd);

public:
    Server();
    void Init();
    void Start();
    void Close();
};

#endif