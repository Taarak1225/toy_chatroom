#include "server.h"

Server::Server(){
    serv.sin_family = AF_INET;
    serv.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serv.sin_addr);

    // listenfd = 0;
    // epfd = 0;
}

void Server::Init(){
    std::cout << "INIT SERVER." << std::endl;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0){
        perror("create listenfd error.");
        exit(-1);
    }

    if(bind(listenfd, (sockaddr* )&serv, sizeof(serv)) < 0){
        perror("bind error.");
        exit(-1);
    }

    if(listen(listenfd, LISTENQ) < 0){
        perror("listen error.");
        exit(-1);
    }

    epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0){
        perror("create epoll error.");
        exit(-1);
    }
    epoll_addfd(epfd, listenfd, true);
}

void Server::Close(){
    close(listenfd);
    close(epfd);
}

void Server::Start(){
    Init();

    epoll_event events[EPOLL_SIZE];

    for(; ; ){
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count < 0){
            perror("epoll failure.");
            exit(-1);
        }
        std::cout << "epoll_events_count : " << epoll_events_count << std::endl;

        for(int i = 0; i < epoll_events_count; i++){
            if(events[i].data.fd == listenfd){
                sockaddr_in new_client;
                socklen_t new_client_len = sizeof(new_client);
                int clientfd = accept(listenfd, (sockaddr* )&new_client, &new_client_len);

                std::cout << "new client connection request from : " << inet_ntoa(new_client.sin_addr) << ":"
                          << ntohs(new_client.sin_port) << ", clientfd = " << clientfd << std::endl;
                
                epoll_addfd(epfd, clientfd, true);

                client_info client;
                client.client_host = static_cast<std::string>(inet_ntoa(new_client.sin_addr));
                uint16_t port_number = ntohs(new_client.sin_port);
                char temp[15];
                sprintf(temp, "%x", port_number);
                client.client_port = static_cast<std::string>(temp);
                client.connfd = clientfd;
                time_t now = time(0);
                client.join_time = ctime(&now);
                client.join_time[strlen(client.join_time - 1)] = '\0';
                client.isNickNameSet = false;
                clients_map.insert(std::make_pair(clientfd, client));

                std::cout << "add new clientfd = " << clientfd << " to epoll." << std::endl;
                std::cout << "now, " << clients_map.size() << " users in this chatroom." << std::endl;
            }
            else{
                int ret = BroadcastMessage(events[i].data.fd);
                if(ret < 0){
                    perror("Broadcast return error.");
                    Close();
                    exit(-1);
                }
            }
        }
    }
    Close();
}

int Server::BroadcastMessage(int connfd){
    char buff[BUFFSIZE];
    char message[BUFFSIZE];
    memset(buff, 0, sizeof(buff));
    memset(message, 0, sizeof(message));
    client_info &current_client = clients_map[connfd];

    std::cout << "read message from client#" << connfd << std::endl;

    int len = recv(connfd, buff, BUFFSIZE, 0);

    std::cout << "debug len = " << len << std::endl;                                                                // debug
    
    if(current_client.isNickNameSet == false){
        current_client.isNickNameSet = true;
        current_client.client_name = static_cast<std::string>(buff);
        sprintf(message, "Welcome %s join this chatroom! ", current_client.client_name.c_str());

        for(auto it = clients_map.cbegin(); it != clients_map.cend(); it++){
            if(it->first != connfd){
                if(send(it->first, message, BUFFSIZE, 0) < 0){
                    return -1;
                }
            }
        }
        return len;
    }

    if(len == 0){
        // len == 0 说明该客户端关闭
        close(connfd);
        clients_map.erase(connfd);
        std::cout << "client#" << connfd << " closed." << std::endl;
        std::cout << "now, " << clients_map.size() << " users in this chatroom." << std::endl;

        sprintf(message, "%s left.", current_client.client_name.c_str());
        for(auto it = clients_map.cbegin(); it != clients_map.cend(); it++){
            if(it->first != connfd){
                if(send(it->first, message, BUFFSIZE, 0) < 0){
                    return -1;
                }
            }
        }
        return len;
    }
    else{
        if(clients_map.size() == 1){
            send(connfd, "Only you in this chatroom now.", BUFFSIZE, 0);
            return len;
        }
        if(strncasecmp(buff, "$ show users", strlen("$ show users")) == 0){
            return ShowUserstoClient(connfd);
        }
        else if(buff[0] == '>'){
            // PM功能
            std::string line = static_cast<std::string>(buff);

            size_t start = line.find_first_of(' ');
            size_t end = line.find_last_of(' ');

            std::string pm_name = line.substr(1, start - 1);
            std::string pm_text = line.substr(start + 1);
            sprintf(message, "%s", pm_text.c_str());

            std::cout << "debug PM_name: " << pm_name << "; " << "PM_text: " << pm_text << ";" << std::endl;        // debug
            std::cout << "debug message: " << message << std::endl;                                                 // debug

            for(auto it = clients_map.cbegin(); it != clients_map.cend(); it++){
                std::cout << "compare __" << it->second.client_name << "__ and __" << pm_name << "__...";           // debug
                if(it->second.client_name == pm_name){
                    std::cout << "true..." << std::endl;                                                            // debug
                    std::cout << "[" << clients_map[connfd].client_name << "] send to ["
                              << it->second.client_name << "]..." << std::endl;                                     // debug
                    if(send(it->first, message, BUFFSIZE, 0) < 0){
                        return -1;
                    }
                    break;
                }
            }
            return len;
        }

        // Broadcast功能
        sprintf(message, "[%s] >> %s", current_client.client_name.c_str(), buff);
        std::cout << "Client  " << current_client.client_name << "say>>" << buff << std::endl;

        for(auto it = clients_map.cbegin(); it != clients_map.cend(); it++){
            if(it->first != connfd){
                std::cout << "[" << clients_map[connfd].client_name << "] send to ["
                          << it->second.client_name << "]..." << std::endl;                                         // debug
                if(send(it->first, message, BUFFSIZE, 0) < 0){
                    return -1;
                }
            }
        }
    }
    return len;
}

int Server::ShowUserstoClient(int connfd){
    char message[BUFFSIZE];
    int num = clients_map.size();

    sprintf(message, "\033[31mHere are %d users online now! ", num);
    sprintf(message, "%s\nHost        Port        User_name        Join_time", message);

    for(auto it = clients_map.cbegin(); it != clients_map.cend(); it++){
        sprintf(message, "%s\n%s   %s        %s        %s", message, it->second.client_host.c_str(),
                it->second.client_port.c_str(), it->second.client_name.c_str(), it->second.join_time);
    }
    sprintf(message, "%s\033[0m\n", message);

    if(send(connfd, message, strlen(message), 0) < 0){
        return -1;
    }

    return num;
}