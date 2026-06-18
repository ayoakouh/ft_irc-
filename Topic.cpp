#include "Server.hpp"
#include <sys/socket.h>
#include <map>
#include <cctype>


void topic(int fd, std::vector<std::string> &s, Server &serv)
{
    std::map<int, Client> &clients_map = serv.get_clients_map();



    if (!clients_map[fd].IsRegistered())
    {
        std::string err_authen = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err_authen.c_str(), err_authen.size() , 0);
        return;
    }

    if (s.size() < 2)
    {
        std::string err_size = ":ft_irc 461 " + clients_map[fd].getNickname() + " TOPIC :Not enough parameters\r\n";
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
    Channel &ch = serv_channel.find(ltarget)->second;
    if (!ch.check_member(fd))
    {
        std::string err_not_a_member = ":ft_irc 442 " + clients_map[fd].getNickname() + " " + s[1] + " :Not in channel\r\n";
        send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
        return;
    }

    if (s.size() == 2) // view mode
    {
        std::string topic_status = ch.getTopic();
        if (topic_status.empty())
        {
            std::string err_not_a_member = ":ft_irc 331 " + clients_map[fd].getNickname() + " " + s[1] + " :No Topic is Set\r\n";
            send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
            return;
        }
        else
        {
            std::string err_not_a_member = ":ft_irc 332 " + clients_map[fd].getNickname() + " " + s[1] + " :" + topic_status + "\r\n";
            send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
            return;
        }
    }
    if (s.size() >= 3) // set mode
    {
        if (ch.isTopicRestricted() && !ch.check_op(fd))
        {
            std::string err_not_a_member = ":ft_irc 482 " + clients_map[fd].getNickname() + " " + s[1] + " :You Cannot Set Topic\r\n";
            send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
            return;
        }
        ch.setTopic(s[2]);
        std::string send_M = ":" + clients_map[fd].getNickname() 
                            + "!" + clients_map[fd].getUsername()
                            + "@" + "ft_irc"
                            + " TOPIC " + s[1]
                            + " :" + s[2] + "\r\n";

        const std::vector<int> &m = ch.get_members();
        for (std::vector<int>::const_iterator it = m.begin(); it != m.end(); ++it)
        {
            send(*it, send_M.c_str(), send_M.size(), 0);
        }

    }
}

