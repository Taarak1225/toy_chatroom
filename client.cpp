#include "client.h"

Client::Client(){
    serv.sin_family = AF_INET;
    serv.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serv.sin_addr);

    // sockfd = 0;
    // epfd = 0;
    // pid = 0;

    isNickNameSet = false;
    isWorking = true;
}

void Client::Help(){
    std::cout << "Usage:" << std::endl;
    std::cout << "      <message>               : send message to all online users." << std::endl;
    std::cout << "      > <username> <message>  : send a message to a single user." << std::endl;
    std::cout << "      exit                    : disconnect to the server and leave." << std::endl;
    std::cout << "      clear                   : clear the screen." << std::endl;
    std::cout << "      $ show users            : show informations of all currently online users." << std::endl;
    std::cout << "\033[33mNow choose a nickname to start: \033[0m";
}

void Client::Connect(){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("create socket error.");
        exit(-1);
    }

    if(connect(sockfd, (sockaddr* )&serv, sizeof(serv)) < 0){
        perror("connect error.");
        exit(-1);
    }

    if(pipe(pipfd) < 0){
        perror("create pipe error.");
        exit(-1);
    }

    epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0){
        perror("create epoll error.");
        exit(-1);
    }
    epoll_addfd(epfd, sockfd, true);
    epoll_addfd(epfd, pipfd[0], true);
}

void Client::Close(){
    if(pid){
        close(pipfd[0]);
        close(sockfd);
    }
    else{
        close(pipfd[1]);
    }
}

void Client::Start(){
    Connect();

    pid = fork();
    if(pid < 0){
        perror("fork error.");
        close(sockfd);
        exit(-1);
    }

    // static struct epoll_event events[2];
    struct epoll_event events[2];

    if(pid > 0){
        // 父进程关闭管道写端
        close(pipfd[1]);

        while(isWorking){
            int epoll_events_count = epoll_wait(epfd, events, 2, -1);
            for(int i = 0; i < epoll_events_count; i++){
                memset(buff, 0, sizeof(buff));
                if(events[i].data.fd == sockfd){
                    int recv_count = recv(sockfd, buff, BUFFSIZE, 0);
                    if(recv_count == 0){
                        std::cout << "Server closed connection : " << sockfd << std::endl;
                        close(sockfd);
                        isWorking = false;
                    }
                    else{
                        std::cout << buff << std::endl;
                    }
                }
                else{
                    int read_count = read(events[i].data.fd, buff, BUFFSIZE);
                    if(read_count == 0){
                        isWorking = false;
                    }
                    else{
                        send(sockfd, buff, BUFFSIZE, 0);
                    }
                }
            }
        }
    }
    else{
        // 子进程关闭管道读端
        close(pipfd[0]);

        Help();
        // std::string NickName;

        while(isWorking){
            memset(buff, 0, sizeof(buff));
            fgets(buff, BUFFSIZE, stdin);

            if(isNickNameSet == false){
                isNickNameSet = true;
                NickName = static_cast<std::string>(buff);
            }

            if(strncasecmp(buff, "exit", strlen("exit")) == 0){
                isWorking = false;
            }
            else if(strncasecmp(buff, "clear", strlen("clear")) == 0){
                system("clear");
                continue;
            }

            if(write(pipfd[1], buff, strlen(buff) - 1) < 0){
                perror("child process pipe write error.");
                exit(-1);
            }
        }
    }
    close(0);
}