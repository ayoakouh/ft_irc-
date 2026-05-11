#include "Server.hpp"

Server::Server(int port, const std::string& password) : port(port), _password(password), Server_fd(-1)
{
    std::cout<< "intialized the attributes"<<std::endl;
}

Server::~Server()
{
    if(Server_fd < 0)
        close(Server_fd);
    std::cout<<"Destructor is called \n";
}

void Server::CreateServer()
{
    CreateSocket();
    BindSocket();
    StartListen();
    AcceptClient();
}
void Server::CreateSocket()
{
    Server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(Server_fd < 0)
        throw std::runtime_error("Socket failed: ");
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
void Server::HandelClient(int fd)
{
    char buffer[1024];

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));

        int BytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);

        if (BytesReceived <= 0)
        {
            std::cout << "Client disconnected\n";
            close(fd);
            break;
        }
        buffer[BytesReceived] = '\0';
        std::cout << "Received: [" << buffer << "]\n";
    }
}
// void Server::HandelClient(int fd)
// {
//     char buffer[1024];
//     int BytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);
//     if(BytesReceived <= 0)
//     {
//         close(fd);
//         return ;
//     }
//     buffer[BytesReceived] = '\0';
//     std::cout<< "Received: " << BytesReceived << " bytes\n";
// }