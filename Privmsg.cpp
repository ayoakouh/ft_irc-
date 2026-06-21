#include "Server.hpp"
#include <sys/socket.h>
#include <map>
#include <cctype>

void privmsg(int fd, std::vector<std::string> &s, Server &serv)
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
        std::string err_size = ":ft_irc 411 " + clients_map[fd].getNickname() + " :No recipient given (PRIVMSG)\r\n";
        send(fd, err_size.c_str(), err_size.size() , 0);
        return;
    }

    if (s.size() < 3)
    {
        std::string err_size = ":ft_irc 412 " + clients_map[fd].getNickname() +  " :No text to send\r\n";
        send(fd, err_size.c_str(), err_size.size() , 0);
        return;

    }
    std::string ltarget = s[1];
    for (size_t i = 0; i < ltarget.size(); i++)
        ltarget[i] = std::tolower(ltarget[i]);
        

    if (s[1][0] == '#')
    {
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
            std::string err_not_a_member = ":ft_irc 404 " + clients_map[fd].getNickname() + " " + s[1] + " :Cannot send to channel\r\n";
            send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
            return;
        }

        std::string send_M = ":" + clients_map[fd].getNickname() + "!" + clients_map[fd].getUsername()
                  + "@" + "ft_irc" + " PRIVMSG " + s[1] + " :" + s[2] + "\r\n";

        const std::vector<int> &m = ch.get_members();
        for (std::vector<int>::const_iterator it = m.begin(); it != m.end(); ++it)
        {
            if (*it != fd)
                send(*it, send_M.c_str(), send_M.size(), 0);
        }


    }
    else
    {
        Client * tclient = NULL;
        for (std::map<int, Client>::iterator it = clients_map.begin(); it != clients_map.end(); ++it)
        {
            std::string nick = it->second.getNickname();
            for (size_t i = 0; i < nick.size(); i++)
                nick[i] = std::tolower(nick[i]);
            if (nick == ltarget)
            {
                tclient = &it->second;
                break;
            }
        }
        if (tclient == NULL)
        {
            std::string err_nick_name_not_found =":ft_irc 401 " + clients_map[fd].getNickname() + " " + s[1] + " :No such nick/channel\r\n";
            send(fd, err_nick_name_not_found.c_str(), err_nick_name_not_found.size() , 0);
            return;
        }
        std::string send_M = ":" + clients_map[fd].getNickname() + "!" + clients_map[fd].getUsername()
                  + "@" + "ft_irc" + " PRIVMSG " + s[1] + " :" + s[2] + "\r\n";
        send(tclient->getFd(), send_M.c_str(), send_M.size(), 0);
    }

}