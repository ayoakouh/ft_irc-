#include "Server.hpp"
#include <sstream>

Server::Server(int port, const std::string& password) : port(port), _password(password), Server_fd(-1)
{
    std::cout<< "intialized the attributes"<<std::endl;
}

std::map<std::string, Channel>& Server::getChannels()
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
    HandelNonBlocking(Server_fd);
    int option_value = 1;
    if(setsockopt(Server_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, 
        sizeof(option_value)) < 0)
        throw std::runtime_error("SetSockopt failed ~!");

    std::cout<<"socket is created ;\n";
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

const std::string& Server::GetPassword() const
{
    return _password;
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
    if(Message[0] == "PASS")
    {
        pass(fd, Message, *this);
    }
    else if (Message[0] == "NICK")
    {
        nick(fd, Message, *this);
    }
    else if (Message[0] == "USER")
    {
        user(fd, Message, *this);
    }
    else if (Message[0] == "MODE")
    {}
    else if (Message[0] == "PRIVMSG")
    {}
    else if (Message[0] == "TOPIC")
    {}
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
    while(1)
    {
        int BytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if(BytesReceived > 0)
        {
            buffer[BytesReceived] = '\0';
            client_buffers[fd] += buffer;
        }
        else if(BytesReceived == 0)
        {
            std::cout << "here is" << fd << " -=== disconnected\n";
            RemoveClient(fd);
            return ;
        }
        else 
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break ;
            std::cout<<"recv error fd="<<fd<<std::endl;
            RemoveClient(fd);
            return ;
        }
    }
    ExtractedMessages(fd);
}


void Server::CreatEpoll()
{
    kqueue_fd = kqueue();
    if (kqueue_fd < 0)
        throw std::runtime_error("Epoll create failed");

    struct kevent Kev;
    EV_SET(&Kev, Server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

    if (kevent(kqueue_fd, &Kev, 1, NULL, 0, NULL) < 0)
        throw std::runtime_error("epol_ctl ADD server_fd failed");

    std::cout << "Server_fd added to kqueue>>>>\n";
}

void Server::AddClientes(int fd)
{
    struct kevent Kev;
    EV_SET(&Kev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

    if (kevent(kqueue_fd, &Kev, 1, NULL, 0, NULL) < 0)
        throw std::runtime_error("kevent ADD clinet failed!");
    std::cout << "Client added — fd=" << fd << "\n";
}

void Server::RemoveClient(int fd)
{
    struct kevent Kev;
    EV_SET(&Kev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(kqueue_fd, &Kev, 1, NULL, 0, NULL); 
    clients_map.erase(fd);
    client_buffers.erase(fd);
    close(fd);
    std::cout << "Client is removed — fd=" << fd << "\n";
}

void Server::run()
{
    struct kevent events[MAX_EVENTS];
     std::cout << "IS running with kqueue...\n";
    while (1)
    {
        int client_fd;
        int number_fd = kevent(kqueue_fd, NULL, 0, events,  MAX_EVENTS, NULL);
        if(number_fd < 0)
            throw std::runtime_error("kevent wait is failed ! ..");
        for(int i = 0; i < number_fd; i++)
        {
            int tmpfd = static_cast<int>(events[i].ident);
            if(tmpfd == Server_fd)
            {
                while(1)
                {
                    client_fd = accept(Server_fd, NULL, NULL);
                    if(client_fd < 0)
                    {
                        if(errno == EAGAIN || errno == EWOULDBLOCK)
                            break ;
                        std::cout << "accepte failed !\n";
                        break ;
                    }

                    HandelNonBlocking(client_fd);

                    clients_map[client_fd] = Client(client_fd);
                    AddClientes(client_fd);
                    std::cout << "New client fd = " << client_fd << std::endl;
                }
            }
            else
            {
                HandelClient(tmpfd);
            }
        }          
    }
}

void Server::HandelNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag < 0)
        throw std::runtime_error("Fcntl FGETFL failes !");
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0)
        throw std::runtime_error("Fcntl F_SETFL failes !");
}

void Server::ExtractedMessages(int fd)
{
    size_t pos;
    while ((pos = client_buffers[fd].find("\n")) != std::string::npos)
    {
        std::string line = client_buffers[fd].substr(0, pos);
        client_buffers[fd].erase(0, pos + 1);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (line.empty())
            continue;

        std::cout << "CMD: " << line << std::endl;
        std::vector<std::string> Mesg = parsing_handler(line);
        command_handeler(fd, Mesg);
    }
}
// void Server::ExtractedMessages(int fd)
// {
//     size_t pos;
//     while((pos = client_buffers[fd].find("\r\n")) != std::string::npos)
//     {
//         std::string line;
//         line = client_buffers[fd].substr(0, pos);
//         client_buffers[fd].erase(0, pos + 2);
//         std::cout << line << std::endl;
//         std::vector<std::string> Mesg;
//         Mesg = parsing_handler(line);
//         command_handeler(fd, Mesg);
//     }
// }

Client& Server::GetClient(int fd)
{
    return clients_map[fd];
}