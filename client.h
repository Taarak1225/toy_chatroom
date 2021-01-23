#ifndef CLIENT_H
#define CLIENT_H

#include "epoll.h"
#include <iostream>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFSIZE 0xffff // 缓冲区大小

class Client{
private:
    std::string NickName;
    int sockfd;
    int epfd;
    int pipfd[2];
    int pid;

    bool isNickNameSet;
    bool isWorking;

    struct sockaddr_in serv;
    char buff[BUFFSIZE];

public:
    Client();
    void Help();
    void Start();
    void Connect();
    void Close();
};

#endif