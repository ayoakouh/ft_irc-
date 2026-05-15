#ifndef CLIENT_HPP
#define CLIENT_HPP




class Client {
    int fd;
    std::string buffer;
    std::string nickname;
    std::string username;
    bool authentication;

}


std::vector<Client> Clients;


//hey;
#endif 