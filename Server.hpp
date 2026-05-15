#ifndef SERVER_HPP
#define SERVER_HPP




#include <iostream>
#include<string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <vector>
#include <sys/epoll.h>


#define MAX_EVENTS 64
class Server {
private:
    int port;
    std::string _password;
    int Server_fd;
    int epollFd;
public:
    Server(int port, const std::string& password);
    ~Server();
    void run();
    void CreateServer();
    void CreateSocket();
    void BindSocket();
    void StartListen();
    void AcceptClient();
    void HandelClient(int fd);
    void CreatEpoll();
    void AddClientes(int fd);
    void RemoveClient(int fd);
};
//canonical form


#endif