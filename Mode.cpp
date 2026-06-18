#include "Server.hpp"
#include <sys/socket.h>
#include <map>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>


void mode(int fd, std::vector<std::string> &s, Server &serv)
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
    Channel &ch = serv_channel.find(ltarget)->second;
    

    if (!ch.check_member(fd))
    {
        std::string err_not_a_member = ":ft_irc 442 " + clients_map[fd].getNickname() + " " + s[1] + " :You're not on that channel\r\n";
        send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
        return;
    }

    if (s.size() == 2)
    {
        std::string modestr = "+";
        std::string modeargs = "";

        if (ch.get_invite_only())
            modestr += "i";
        if (ch.isTopicRestricted())
            modestr += "t";
        if (!ch.get_key().empty())
        {
            modestr += "k";
            modeargs += " " + ch.get_key();
        }
        if (ch.get_channel_size() > 0)
        {
            modestr += "l";
            std::ostringstream oss;
            oss << ch.get_channel_size();
            modeargs += " " + oss.str();
        }

        std::string r = ":ft_irc 324 " + clients_map[fd].getNickname()
                        + " " + s[1] + " " + modestr + modeargs + "\r\n";
        send(fd, r.c_str(), r.size(), 0);
        return;
        
    }

    if (!ch.check_op(fd))
    {
        std::string err_not_a_member = ":ft_irc 482 " + clients_map[fd].getNickname() + " " + s[1] + " :You're not on channel operator\r\n";
        send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
        return;
    }
    char sign = '+';

    if (s[2].size() < 2 || (s[2][0] != '+' && s[2][0] != '-'))
    {
        std::string err = ":ft_irc 472 " + clients_map[fd].getNickname() + " " + s[2] + " :is unknown mode char to me\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;

    }

    size_t i = 0;
    int arg_idx = 3;
    std::string sign_str;
    while (i < s[2].size())
    {
            if (s[2][i] == '+' || s[2][i] == '-')
            {
                sign = s[2][i];
                sign_str = sign;
                i++;
                continue;
            }
            if (s[2][i] == 'i')
            {
                if (sign == '+')
                    ch.set_invite_only(true);
                else if (sign == '-')
                    ch.set_invite_only(false);

                std::string send_M = ":" + clients_map[fd].getNickname()
                                + "!" + clients_map[fd].getUsername()
                                + "@ft_irc MODE " + s[1] + " " + sign_str + "i\r\n";
                const std::vector<int> &m = ch.get_members();
                for (std::vector<int>::const_iterator it = m.begin(); it != m.end(); ++it)
                    send(*it, send_M.c_str(), send_M.size(), 0);
                i++;
                 continue; //return;
            }
            else if (s[2][i] == 't')
            {
                if (sign == '+')
                    ch.set_Topic_Restricted(true);
                else if (sign == '-')
                    ch.set_Topic_Restricted(false);

                std::string send_M = ":" + clients_map[fd].getNickname()
                                + "!" + clients_map[fd].getUsername()
                                + "@ft_irc MODE " + s[1] + " " + sign_str + "t\r\n";
                const std::vector<int> &m = ch.get_members();
                for (std::vector<int>::const_iterator it = m.begin(); it != m.end(); ++it)
                    send(*it, send_M.c_str(), send_M.size(), 0);
                i++;
                continue; //return;
            }
            else if (s[2][i] == 'k') 
            {
                std::string send_M;
                if (sign == '+')
                {
                    if ((int)s.size() <= arg_idx)
                    {
                        std::string err = ":ft_irc 461 " + clients_map[fd].getNickname() + " MODE :Not enough parameters\r\n";
                        send(fd, err.c_str(), err.size(), 0);
                       return;
                    }
                    ch.set_key(s[arg_idx]);

                    send_M = ":" + clients_map[fd].getNickname()
                            + "!" + clients_map[fd].getUsername()
                            + "@ft_irc MODE " + s[1] + " +k " + s[arg_idx] + "\r\n";
                    arg_idx++;
                }
                else if (sign == '-')
                {
                    ch.remove_key();
                    send_M = ":" + clients_map[fd].getNickname()
                        + "!" + clients_map[fd].getUsername()
                        + "@ft_irc MODE " + s[1] + " -k *\r\n";
                    // arg_idx++;
                }
                const std::vector<int> &m = ch.get_members();
                for (std::vector<int>::const_iterator it = m.begin(); it != m.end(); ++it)
                    send(*it, send_M.c_str(), send_M.size(), 0);
                i++;
                continue; //return;
            }
            else if (s[2][i] == 'o')
            {
                if ((int)s.size() <= arg_idx) // Does tokens[3] exist?
                {
                    std::string err = ":ft_irc 461 " + clients_map[fd].getNickname() + " MODE :Not enough parameters\r\n";
                    send(fd, err.c_str(), err.size(), 0);
                    return;
                }
                std::string lnick = s[arg_idx];
                int tfd = -1;
                for (size_t i = 0; i < lnick.size(); i++)
                    lnick[i] = std::tolower(lnick[i]);
            // Client * tclient = NULL; // Find the target client by nickname
                for (std::map<int, Client>::iterator it = clients_map.begin(); it != clients_map.end(); ++it)
                {
                    std::string nick = it->second.getNickname();
                    for (size_t i = 0; i < nick.size(); i++)
                        nick[i] = std::tolower(nick[i]);
                    if (nick == lnick)
                    {
                    // tclient = &it->second;
                        tfd = it->first;
                        break;
                    }
                }
                if (tfd == -1)
                {
                    std::string err_nick_name_not_found =":ft_irc 401 " + clients_map[fd].getNickname() + " " + s[arg_idx] + " :No such nick/channel\r\n";
                    send(fd, err_nick_name_not_found.c_str(), err_nick_name_not_found.size() , 0);
                    i++;
                    arg_idx++;
                    continue; //return;
                }
                //
                if (!ch.check_member(tfd))
                {
                    std::string err_not_a_member = ":ft_irc 441 " + clients_map[fd].getNickname() + " " + s[1] + " :They aren't on that channel\r\n";
                    send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
                    i++;
                    arg_idx++;
                    continue; //return;
                }

                if (sign == '+')
                    ch.become_op(tfd);
                else if (sign == '-')
                    ch.pop_op(tfd);
                
                std::string send_M = ":" + clients_map[fd].getNickname()
                                + "!" + clients_map[fd].getUsername()
                                + "@ft_irc MODE " + s[1] + " " + sign_str + "o " + s[arg_idx] + "\r\n";
                arg_idx++;
                const std::vector<int> &m = ch.get_members();
                for (std::vector<int>::const_iterator it = m.begin(); it != m.end(); ++it)
                    send(*it, send_M.c_str(), send_M.size(), 0);
                i++;
                continue; //return;
                
            }
            else if (s[2][i] == 'l')
            {
                if (sign == '+')
                {
                    if ((int)s.size() <= arg_idx) // Does tokens[3] exist?
                    {
                        std::string err = ":ft_irc 461 " + clients_map[fd].getNickname() + " MODE :Not enough parameters\r\n";
                        send(fd, err.c_str(), err.size(), 0);
                        return;
                    }

                    // check s[3] is not a valid number If not, send 461 or 472

                    std::string channel_size = s[arg_idx];
                    bool valid = true;
                    for (size_t j = 0; j < channel_size.size(); j++)
                    {
                        if (!isdigit(channel_size[j]))
                        {
                            std::string err = ":ft_irc 472 " + clients_map[fd].getNickname() + " " + std::string(1, channel_size[j]) + " :is unknown mode char to me\r\n";
                            send(fd, err.c_str(), err.size(), 0);
                            valid = false;
                            break;
                        }
                    }
                    if (!valid)
                    {
                        i++;
                        arg_idx++;
                        continue;
                    }

                    //check s[3] is "0" or negative 
                    std::stringstream ss(channel_size);
                    int size_int;
                    ss >> size_int;
                    if (size_int <= 0)
                    {
                        std::string err = ":ft_irc 472 " + clients_map[fd].getNickname() + " " + channel_size + " :Invalid mode parameter\r\n";
                        send(fd, err.c_str(), err.size(), 0);
                        i++;
                        arg_idx++;
                        continue; //return;
                    } 
                    ch.set_channel_size(size_int);
                    std::ostringstream oss;
                    oss << size_int;
                    std::string send_M = ":" + clients_map[fd].getNickname()
                                + "!" + clients_map[fd].getUsername()
                                + "@ft_irc MODE " + s[1]  + " +l " + oss.str() +"\r\n";
                    arg_idx++;
                    const std::vector<int> &m = ch.get_members();
                    for (std::vector<int>::const_iterator it = m.begin(); it != m.end(); ++it)
                        send(*it, send_M.c_str(), send_M.size(), 0);
                }
                else if (sign == '-')
                {
                    ch.set_channel_size(-1);
                    std::string send_M = ":" + clients_map[fd].getNickname()
                                + "!" + clients_map[fd].getUsername()
                                + "@ft_irc MODE " + s[1]  + " -l\r\n";
                    const std::vector<int> &m = ch.get_members();
                    for (std::vector<int>::const_iterator it = m.begin(); it != m.end(); ++it)
                        send(*it, send_M.c_str(), send_M.size(), 0);

                }
                i++;
                continue; //return;
            }
            else
            {
                std::string err_not_a_member = ":ft_irc 472 " + clients_map[fd].getNickname() + " " + std::string(1, s[2][i]) + " :is unknown mode char to me\r\n";
                send(fd, err_not_a_member.c_str(), err_not_a_member.size() , 0);
                i++;
                continue; //return;
            }
    }

}

