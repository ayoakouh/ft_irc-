#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <string>
#include <vector>

class Client {

private:
    bool _Registered;
    bool _passSent;

    int fd;
    std::string buffer;
    std::string nickname;
    std::string username;
    bool authenticated;
    std::string host; //i added this one

	public:
        Client(int client_fd = -1);
        int getFd(void) const;
        void setFd(int client_fd);
        std::string &getname(void);
        const std::string &getBuffer(void) const;
        void setBuffer(const std::string &value);
        const std::string &getNickname(void) const;
        void setNickname(const std::string &value);
        const std::string &getUsername(void) const;
        void setUsername(const std::string &value);
        const std::string   &get_host(void);
        void    set_host(const std::string &value);
        bool isAuthenticated(void) const;
        void setAuthenticated(bool value);
        bool IsRegistered() const;
        void SetRegistered(bool value);
        void setPassSent(bool value);
        bool isPassSent() const;
};


//hey;
#endif 