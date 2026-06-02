#ifndef SERVER_HPP
#define SERVER_HPP



#include <iostream>
#include<string>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <vector>

#include <sys/event.h>
#include <sys/time.h>

#include "Channel.hpp"
#include "Client.hpp"


#define MAX_EVENTS 64
class Server {
private:
    std::map<std::string, Channel> serv_channel;
    std::map<int, Client> clients_map;
    std::map<int, std::string> client_buffers;
    int port;
    const std::string _password;
    int Server_fd;
    int kqueue_fd;
public:
    Server(int port, const std::string& password);
    ~Server();
    std::map<std::string, Channel> &getChannels();
    void run();
    void CreateServer();
    void CreateSocket();
    void BindSocket();
    void StartListen();
    void AcceptClient();
    std::vector<std::string> parsing_handler(std::string buffer);
    void command_handeler(int fd, std::vector<std::string> Message);
    void HandelClient(int fd);
    void CreatEpoll();
    void AddClientes(int fd);
    void RemoveClient(int fd);
    void HandelNonBlocking(int fd);
    void ExtractedMessages(int fd);
};
//canonical form

void join(unsigned int fd, std::vector<std::string> &s, Server &serv);
void invite(unsigned int fd, std::vector<std::string> &s, Server &serv);
void kick(unsigned int fd, std::vector<std::string> &s, Server &serv);


#endif