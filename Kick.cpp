#include "Client.hpp"
#include "Server.hpp"


void fill_users(std::vector<std::string> &users, std::string &s)
{
	std::stringstream ss(s);
	std::string word;
	while (std::getline(ss, word, ','))
	{
		users.push_back(word);
	}
}


void kick(unsigned int fd, std::vector<std::string> &s, Server &serv)
{
	int target_fd = -1;
	std::vector<std::string> users;
	std::vector<int> targets;
	std::map<int, Client> &clients_map = serv.get_clients_map(); // fill the fds of users
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
	if (s.size() != 2 && s.size() != 3) // number of params are correct
	{
        err = ":ft_irc 461 " + nick + " KICK :Not enough parameters\r\n";
        send(fd, err.c_str(), err.size() , 0);
		return ;
	}
	fill_users(users, s[2]);
	for (size_t i = 0; i < users.size(); i++)
	{
		for (std::map<int, Client>::iterator it = clients_map.begin(); it != clients_map.end(); it++)
		{
			if (it->second.getNickname() == users[i])
			{
				target_fd = it->first;
				break;
			}
		}
		targets.push_back(target_fd);
		if (target_fd == -1)
		{
			err = ":ft_irc 401 " + nick + " " + s[2] + " :No such nick/channel\r\n";
			send(fd, err.c_str(), err.size() , 0);
		}
		target_fd = -1;
	}
	//if no reason is provided use a default reason, maybe the name of the one getting kicked
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		if (it->first == s[1]) // does the channel exist
		{
			if (!it->second.check_member(fd))
			{
				err = ":ft_irc 442 " + nick + " " + it->first + " :You're not on that channel\r\n";
				send(fd, err.c_str(), err.size() , 0);
				return ;
			}
			if (!it->second.check_op(fd)) // is the caller an operator of the channel
			{
				err = ":ft_irc 482 " + nick + " " + it->first + " :You're not channel operator\r\n";
				send(fd, err.c_str(), err.size() , 0);
				return ;
			}
			for (size_t i = 0; i < targets.size(); i++)
			{
				if (targets[i] != -1)
				{
					if (!it->second.check_member(targets[i]))
					{
						err = ":ft_irc 441 " + nick + " " + s[2] + " " + it->first + " :They aren't on that channel\r\n";
						send(fd, err.c_str(), err.size() , 0);
						continue ;
					}
					if (it->second.check_op(targets[i])) // check if the target user an operator
						it->second.pop_op(targets[i]);
					if (it->second.check_member(targets[i])) // check if the target user a member
					{
						it->second.pop(targets[i]);
						err = ":" + nick + "!" + user + "@" + host + " KICK " + s[1] + " " + clients_map[targets[i]].getNickname() + " :" + reason + "\r\n"; //reason must be filled
						send(fd, err.c_str(), err.size() , 0);
						if (it->second.get_members().empty())
						{
							channels.erase(it);
						}
					}
				}
			}
			return ;
		}
	}

    err = ":ft_irc 403 " + nick + " " + s[1] + " :No such channel\r\n";
    send(fd, err.c_str(), err.size() , 0);
	return ;
}