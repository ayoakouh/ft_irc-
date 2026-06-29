#include "Client.hpp"
#include "Server.hpp"

void invite(unsigned int fd, std::vector<std::string> &s, Server &serv)
{ 
	int target_fd = -1;
	std::map<int, Client> &clients_map = serv.get_clients_map();
	std::string err;
	std::string nick = clients_map[fd].getNickname();
	std::string user = clients_map[fd].getUsername();
	std::string host = clients_map[fd].get_host();

    if (!clients_map[fd].IsRegistered())
    {
        err = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err.c_str(), err.size() , 0);
        return;
    }
    if (s.size() != 3) //did the user provide both the nickname and channel name
    {
		err = ":ft_irc 461 " + nick + " INVITE :Not enough parameters\r\n";
        send(fd, err.c_str(), err.size() , 0);
        return ;
    }
	for (std::map<int, Client>::iterator it = clients_map.begin(); it != clients_map.end(); it++)
	{
		if (it->second.getNickname() == s[1])
		{
			target_fd = it->first;
			break;
		}
	}
	if (target_fd == -1)
	{
		err = ":ft_irc 401 " + nick + " " + s[1] + " :No such nick/channel\r\n";
        send(fd, err.c_str(), err.size() , 0);
		return ;
	}
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		if (it->first == s[2]) // does the channel exist?
		{
			if (!it->second.check_member(fd)) //is the user inviting in the channel?
			{
				err = ":ft_irc 442 " + nick + " " + (it->second).get_name() + " :You're not on that channel\r\n";
        		send(fd, err.c_str(), err.size() , 0);
				return ;
			}
			if (it->second.get_invite_only()) // is the channel an invite only?
			{
				if (!it->second.check_op(fd)) // is the inviter an operator?
				{
					err = ":ft_irc 482 " + nick + " " + (it->second).get_name() + " :You're not channel operator\r\n";
        			send(fd, err.c_str(), err.size() , 0);
					return ;
				}
			}
			if (it->second.check_member(target_fd)) // is the target user already in the channel?
			{
				err = ":ft_irc 443 " + nick + " " + s[2] + " " + (it->second).get_name() + " :is already on channel\r\n";
        		send(fd, err.c_str(), err.size() , 0);
				return ;
			}
			it->second.add_invite(target_fd);
			err = ":ft_irc 341 " + nick + " " + s[2] + " " + (it->second).get_name() + "\r\n";
        	send(fd, err.c_str(), err.size() , 0);
			err = ":" + nick + "!" + user + "@" + host + " INVITE " + s[2] + " :" + (it->second).get_name() + "\r\n";
			send(target_fd, err.c_str(), err.size() , 0);

			//send messages to the target and the caller
			return ;
		}
	}
	err = ":ft_irc 403 " + nick + " " + s[2] + " :No such channel\r\n" ;
    send(fd, err.c_str(), err.size() , 0);
	return ;
}