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

void ft_errors(int check, int fd, std::string &nick, std::string &channel, std::string user)
{
	std::string err;
	if (check == 1)
	{
		err = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 2)
	{
		err = ":ft_irc 461 " + nick + " KICK :Not enough parameters\r\n";
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 3)
	{
		err = ":ft_irc 401 " + nick + " " + user + " :No such nick/channel\r\n";
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
		err = ":ft_irc 441 " + nick + " " + user + " " + channel + " :They aren't on that channel\r\n";
		send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 7)
	{
		err = ":ft_irc 403 " + nick + " " + channel + " :No such channel\r\n";
    	send(fd, err.c_str(), err.size() , 0);
	}
}

void ft_send(std::vector<int> &members, std::string &err)
{
	for (size_t i = 0; i < members.size(); i++)
	{
		send(members[i], err.c_str(), err.size() , 0);
	}
}


void kick(unsigned int fd, std::vector<std::string> &s, Server &serv)
{
	int target_fd = -1;
	std::vector<std::string> users;
	std::vector<int> targets;
	std::map<int, Client> &clients_map = serv.get_clients_map(); // fill the fds of users
	std::string nick = clients_map[fd].getNickname();
	std::string user = clients_map[fd].getUsername();
	std::string host = clients_map[fd].get_host();
	std::string	reason;
	std::string err;
	int check = 0;

    if (!clients_map[fd].IsRegistered())
    {
        return (ft_errors(1, fd, nick, nick, nick));
    }
	if (s.size() < 3 || s.size() > 4) // number of params are correct
	{
        return (ft_errors(2, fd, nick, nick, nick));
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
        	ft_errors(3, fd, nick, s[1], users[i]);
		}
		target_fd = -1;
	}
	if (s.size() == 4)
	{
		reason = s[3];
		check = 1;
	}
	//if no reason is provided use a default reason, maybe the name of the one getting kicked
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		if (it->first == s[1]) // does the channel exist
		{
			if (!it->second.check_member(fd))
        		return (ft_errors(4, fd, nick, s[1], nick));
			if (!it->second.check_op(fd)) // is the caller an operator of the channel
        		return (ft_errors(5, fd, nick, s[1], nick));
			for (size_t i = 0; i < targets.size(); i++)
			{
				if (targets[i] != -1)
				{
					if (!it->second.check_member(targets[i]))
					{
						ft_errors(6, fd, nick, s[1], users[i]);
						continue ;
					}
					if (it->second.check_op(targets[i])) // check if the target user an operator
						it->second.pop_op(targets[i]);
					if (reason.empty())
						reason = clients_map[targets[i]].getNickname();
					err = ":" + nick + "!" + user + "@" + host + " KICK " + s[1] + " " + clients_map[targets[i]].getNickname() + " :" + reason + "\r\n"; //reason must be filled
					ft_send(it->second.get_members(), err);
					it->second.pop(targets[i]);
					if (!check)
					reason.clear();
				}
			}
			if (it->second.get_members().empty())
				channels.erase(it);
			return ;
		}
	}
    ft_errors(7, fd, nick, s[1], nick);
	return ;
}