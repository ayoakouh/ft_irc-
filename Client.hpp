#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <vector>

class Client {
    int fd;
    std::string buffer;
    std::string nickname;
    std::string username;
    bool authentication;
	public:
		std::string &getname(void);//it returns nickname
};


std::vector<Client> Clients;


//hey;
#endif 