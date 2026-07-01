#include "Client.hpp"
#include "Server.hpp"

std::string parsing(std::string &channel)
{
	std::stringstream ss(channel);
	std::string word;
	while (std::getline(ss, word))
	{
		for (size_t i = 0; i < word.size();i++)
			word[i] = std::tolower(word[i]);
	}
	return (word);
}

void ft_error(int check, int fd, std::string &nick, std::string &target_nick, std::string &channel)
{
	std::string err;
	if (check == 1)
	{
		err = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 2)
	{
		err = ":ft_irc 461 " + nick + " INVITE :Not enough parameters\r\n";
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 3)
	{
		err = ":ft_irc 401 " + nick + " " + target_nick + " :No such nick/channel\r\n";
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 4)
	{
		err = ":ft_irc 442 " + nick + " " + channel + " :You're not on that channel\r\n";
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 5)
	{
		err = ":ft_irc 482 " + nick + " " + channel + " :You're not channel operator\r\n";
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 6)
	{
		err = ":ft_irc 443 " + nick + " " + target_nick + " " + channel + " :is already on channel\r\n";
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 7)
	{
		err = ":ft_irc 403 " + nick + " " + channel + " :No such channel\r\n" ;
    	send(fd, err.c_str(), err.size() , 0);
	}
}

void ft_success(int fd, std::string &nick, const std::string &channel_name, std::string &user,
	std::string &host, int target_fd, std::string &target_user)
{
	std::string s;
	s = ":ft_irc 341 " + nick + " " + target_user + " " + channel_name + "\r\n";
    send(fd, s.c_str(), s.size() , 0);
	s = ":" + nick + "!" + user + "@" + host + " INVITE " + target_user + " :" + channel_name + "\r\n";
	send(target_fd, s.c_str(), s.size() , 0);
}

void invite(unsigned int fd, std::vector<std::string> &s, Server &serv)
{ 
	int target_fd = -1;
	std::map<int, Client> &clients_map = serv.get_clients_map();
	std::string to_low;
	std::string nick = clients_map[fd].getNickname();
	std::string user = clients_map[fd].getUsername();
	std::string host = clients_map[fd].get_host();

    if (!clients_map[fd].IsRegistered())
		return (ft_error(1, fd, nick, to_low, to_low));
    if (s.size() != 3) //did the user provide both the nickname and channel name
		return (ft_error(2, fd, nick, to_low, to_low));
	for (std::map<int, Client>::iterator it = clients_map.begin(); it != clients_map.end(); it++)
	{
		if (it->second.getNickname() == s[1])
		{
			target_fd = it->first;
			break;
		}
	}
	if (target_fd == -1)
		return (ft_error(3, fd, nick, s[1], s[2]));
	to_low = parsing(s[2]);
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		if (it->first == to_low) // does the channel exist?
		{
			if (!it->second.check_member(fd)) //is the user inviting in the channel?
				return (ft_error(4, fd, nick, s[1], s[2]));
			if (it->second.get_invite_only()) // is the channel an invite only?
			{
				if (!it->second.check_op(fd)) // is the inviter an operator?
					return (ft_error(5, fd, nick, s[1], s[2]));
			}
			if (it->second.check_member(target_fd)) // is the target user already in the channel?
				return (ft_error(6, fd, nick, s[1], s[2]));
			it->second.add_invite(target_fd);
			return (ft_success(fd, nick, (it->second).get_name(), user, host, target_fd, s[2]));
		}
	}
	return (ft_error(7, fd, nick, s[1], s[2]));
}