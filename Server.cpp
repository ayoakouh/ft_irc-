#include "Server.hpp"
#include <sstream>

Server::Server(int port, const std::string& password) : port(port), _password(password), Server_fd(-1)
{
    std::cout<< "intialized the attributes"<<std::endl;
}

std::map<std::string, Channel> &Server::getChannels()
{
    return serv_channel;
}

Server::~Server()
{
    if(Server_fd >= 0)
        close(Server_fd);
    std::cout<<"Destructor is called \n";
}

void Server::CreateServer()
{
    CreateSocket();
    BindSocket();
    StartListen();
    CreatEpoll();
    // AcceptClient();
    run();
}

void Server::CreateSocket()
{
    Server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(Server_fd < 0)
        throw std::runtime_error("Socket failed: ");
    int option_value = 1;
    if(setsockopt(Server_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, 
        sizeof(option_value)) < 0)
        throw std::runtime_error("SetSockopt failed ~!");

    std::cout<<"is working correcte\n";
}

void Server::BindSocket()
{
    struct sockaddr_in ServAddress;

    std::memset(&ServAddress, 0, sizeof(ServAddress));
    ServAddress.sin_family = AF_INET;
    ServAddress.sin_addr.s_addr = INADDR_ANY;
    ServAddress.sin_port = htons(port);
    if(bind(Server_fd, (sockaddr*)&ServAddress, sizeof(ServAddress)) < 0)
    {
        close(Server_fd);
        throw std::runtime_error("bind failed: ");
    }
    std::cout <<"bind is succefuly\n";
}

void Server::StartListen()
{
    if (listen(Server_fd, 128) < 0)
    {
        close(Server_fd);
        throw std::runtime_error("No listning: ");
    }
}

void Server::AcceptClient()
{
    while(1)
    {
        int client_fd = accept(Server_fd, NULL, NULL);
        if(client_fd < 0)
        {
            std::cout << " No accepted \n";
            continue ;
        }
        HandelClient(client_fd);
        std::cout << " is Connected ! \n";
    }
}


std::vector<std::string> Server::parsing_handler(std::string buffer)
{
    std::vector<std::string> Message;
    bool has_trailing = false;
    std::string str_buffer(buffer);
    size_t pos;
    std::string left;
    std::string right;
    std::string line;
    std::stringstream tokens;
    if (!str_buffer.empty() && str_buffer[0] == ':')
    {
        pos = str_buffer.find(' ');
        if (pos != std::string::npos)
            str_buffer = str_buffer.substr(pos + 1);
    }
    size_t pos_two = str_buffer.find(" :");
    if (pos_two != std::string::npos)
    {
        right = str_buffer.substr(pos_two + 2);
        str_buffer = str_buffer.substr(0,pos_two);
        has_trailing = true;
    }
    std::stringstream tmp(str_buffer);
    std::string word;
    while (tmp >> word)
        Message.push_back(word);


    if (has_trailing)
        Message.push_back(right);

    return Message;
}

void Server::command_handeler(int fd, std::vector<std::string> Message)
{
    if (Message.empty())
        return;
    if (Message[0] == "MODE")
    {

    }
    else if (Message[0] == "PRIVMSG")
    {

    }
    else if (Message[0] == "TOPIC")
    {
        
    }
    else if (Message[0] == "JOIN")
    {
        join(fd, Message, *this);
    }
    else if (Message[0] == "INVITE")
    {
        invite(fd, Message, *this);
    }
    else if (Message[0] == "KICK")
    {
        kick(fd, Message, *this);
    }
    else
    {
        std::cout << "Command not found : " << Message[0] << std::endl;
    }
}



void Server::HandelClient(int fd)
{
    char buffer[1024];
    std::vector<std::string> Message;
    memset(buffer, 0, sizeof(buffer));
    int BytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (BytesReceived <= 0)
    {
        RemoveClient(fd);
        std::cout << "here is -=== disconnected\n";
        return;
    }
    buffer[BytesReceived] = '\0';
    Message = parsing_handler(buffer);
    command_handeler(fd, Message);
    std::cout << "Received: [" << buffer << "]\n";
}

void Server::CreatEpoll()
{
    epollFd = epoll_create1(0);
    if (epollFd < 0)
        throw std::runtime_error("Epoll create failed");

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = Server_fd;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, Server_fd, &ev) < 0)
        throw std::runtime_error("epol_ctl ADD server_fd failed");

    std::cout << "Server_fd added to epoll>>>>\n";
}

void Server::AddClientes(int fd)
{
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) < 0)
        throw std::runtime_error("EPOLL CTL FAILED !");
    std::cout << "Client added — fd=" << fd << "\n";
}

void Server::RemoveClient(int fd)
{
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
    clients_map.erase(fd);
    close(fd);
    std::cout << "Client is removed — fd=" << fd << "\n";
}

void Server::run()
{
    struct epoll_event events[MAX_EVENTS];
     std::cout << "IS running with epoll...\n";
    while (1)
    {
        int client_fd;
        int number_fd = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if(number_fd < 0)
            throw std::runtime_error("pollWait is failed ! ..");
        for(int i = 0; i < number_fd; i++)
        {
            int tmpfd = events[i].data.fd;
            if(tmpfd == Server_fd)
            {
                client_fd = accept(Server_fd, NULL, NULL);
                if(client_fd < 0)
                    continue;
				clients_map[client_fd] = Client(client_fd);
                AddClientes(client_fd);
            }
            else
            {
                HandelClient(tmpfd);
            }
        }          
    }
}