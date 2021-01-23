#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"   // 默认服务器IP地址
#define SERVER_PORT 8888        // 服务器端口号
#define EPOLL_SIZE 5000         // 为epoll支持的最大句柄数
#define LISTENQ 1024            // 监听队列长度

static void epoll_addfd(int epfd, int fd, bool setET){
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if(setET){
        event.events |= EPOLLET;
    }

    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

#endif