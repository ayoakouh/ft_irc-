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
#include <poll.h>
#include <vector>




class Server {
private:
    int port;
    std::string _password;
    int Server_fd;
    int FdPoll;
    std::vector<struct pollfd> _pollFds;
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
    void CreatRpoll();

};

// std::vector<struct pollfd> _pollFds;

#endif