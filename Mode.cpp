#include "Server.hpp"
#include <sys/socket.h>
#include <map>
#include <cctype>



void mode(int fd, std::vector<std::string> &s, Server &serv)
{
    std::map<int, Client> &clients_map = serv.get_clients_map();

    if (!clients_map[fd].isAuthenticated())
    {
        std::string err_authen = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err_authen.c_str(), err_authen.size() , 0);
        return;
    }

    if (s.size() < 2)
    {
        std::string err_size = ":ft_irc 461 " + clients_map[fd].getNickname() + " :Not enough parameters\r\n";
        send(fd, err_size.c_str(), err_size.size() , 0);
        return;
    }
    std::string ltarget = s[1];
    for (size_t i = 0; i < ltarget.size(); i++)
        ltarget[i] = std::tolower(ltarget[i]);

    std::map<std::string, Channel> &serv_channel = serv.getChannels();
    if (serv_channel.find(ltarget) == serv_channel.end())
    {
        std::string err_channel_not_found = ":ft_irc 403 " + clients_map[fd].getNickname() + " " + s[1] + " :No such channel\r\n";
        send(fd, err_channel_not_found.c_str(), err_channel_not_found.size() , 0);
        return;
    }
    Channel &ch = serv_channel[ltarget];
    

    if (!ch.check_member(fd))
    {
        std::string err_not_a_member = ":ft_irc 442 " + clients_map[fd].getNickname() + " " + s[1] + " :You're not on that channel\r\n";
        send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
        return;
    }


    if (!ch.check_op(fd))
    {
        std::string err_not_a_member = ":ft_irc 482 " + clients_map[fd].getNickname() + " " + s[1] + " :You're not on channel operator\r\n";
        send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
        return;
    }
    std::string sign;
    if (s[2][0] == '+' || s[2][0] == '-')
        sign = s[2][0];

    if (s[2][1] == 'i')
    {
        if (sign == "+")
            ch.set_invite_only(true);
        else if (sign == "-")
            ch.set_invite_only(false);
    }
    if (s[2][1] == 't')
    {}
    if (s[2][1] == 'k')
    {}
    if (s[2][1] == 'o')
    {}
    if (s[2][1] == 'l')
    {}
    else
    {
        std::string err_not_a_member = ":ft_irc 472 " + clients_map[fd].getNickname() + " " + s[1] + " :is unknown mode char to me\r\n";
        send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
        return; 
    }


}