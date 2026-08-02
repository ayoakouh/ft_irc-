#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

void	handle_case_zero(unsigned int fd, Server &serv, std::string &nick, std::string &user, std::string &host)
{
	std::map<std::string, Channel> &channels = serv.getChannels();
	std::string part;
	std::vector<int> mem;
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		if (it->second.check_member(fd))
		{
			if (it->second.check_op(fd))
				it->second.pop_op(fd);
			part = ":" + nick + "!" + user + "@" + host + " PART " + it->first + "\r\n";
			mem = it->second.get_members();
			for (int i = 0; i < mem.size(); i++)
				send(mem[i], part.c_str(), part.size() , 0);
			it->second.pop(fd);
			if (it->second.get_members().empty())
				channels.erase(it);
		}
	}
}

void	parsing(std::vector<std::string> &s, std::vector<std::string> &channels_name, std::vector<std::string> &channels_key, std::vector<std::string> &channels_origins)
{
	std::stringstream ss;
	std::string word;
	for (size_t j = 1; j < 3; j++)
	{
		if (j < s.size())
		{
			ss.str(s[j]);
			if (j == 1)
			{
				while (std::getline(ss, word, ','))
				{
					channels_origins.push_back(word);
					for (size_t i = 0; i < word.size();i++)
						word[i] = std::tolower(word[i]);
					channels_name.push_back(word);
				}
				ss.clear();
			}
			else
			{
				while (std::getline(ss, word, ','))
				{
					channels_key.push_back(word);
				}
			}
		}
	}
}

int	check_channel(std::string &s)
{
	for (size_t i = 0; i < s.size(); i++) // is the channel name valid?
    {
        if (!i && s[i] != '#') //must revise before push
			return (1);
		if  (std::isspace(s[i]) || s[i] == ',' || s[i] == 7)
			return (1);
    }
	return (0);
}

// void ft_send(std::vector<int> &members, std::string &err)
// {
// 	for (size_t i = 0; i < members.size(); i++)
// 	{
// 		send(members[i], err.c_str(), err.size() , 0);
// 	}
// }

void	ft_errors(int check, int fd, std::string &nick, std::string &channel, const std::string &topic)
{
	std::string err;
	if (check == 1)
	{
		err = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 2)
	{
		err = ":ft_irc 461 " + nick + " JOIN :Not enough parameters\r\n";
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 3)
	{
		err = ":ft_irc 476 " + nick + " " + channel + " :Bad Channel Mask\r\n"; 
        send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 4)
	{
		err = ":ft_irc 471 " + nick + " " + channel + " :Cannot join channel (+l)\r\n";
		send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 5)
	{
		err = ":ft_irc 473 " + nick + " " + channel + " :Cannot join channel (+i)\r\n";
		send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 6)
	{
		err = ":ft_irc 475 " + nick + " " + channel + " :Cannot join channel (+k)\r\n";
		send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 7)
	{
		err =":ft_irc 332 " + nick + " " + channel + " :" + topic + "\r\n";
		send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 8)
	{
		err =":ft_irc 331 " + nick + " " + channel + " :No topic is set\r\n";
		send(fd, err.c_str(), err.size() , 0);
	}
	else if (check == 9)
	{
		err = ":ft_irc 366 " + nick + " " + channel + " :End of /NAMES list\r\n";
		send(fd, err.c_str(), err.size() , 0);
	}
}

void	ft_fill_nick(std::string &names, Channel &c)
{
	std::map<std::string, bool> members = c.get_channel_members();
	for (std::map<std::string, bool>::iterator it = members.begin(); it != members.end(); it++)
	{
		if (!names.empty())
			names += ' ';
		if (it->second)
			names += '@' + it->first;
		else
			names += it->first;
	}
}

std::string ft_lower_input(const std::string &channel)
{
	std::string copy;
	for (size_t i = 0; i < channel.size();i++)
		copy[i] = std::tolower(channel[i]);
	return (copy);
}

void join(unsigned int fd, std::vector<std::string> &s, Server &serv)
{
	std::map<int, Client> &clients_map = serv.get_clients_map();
	std::vector<std::string> channels_name;
	std::vector<std::string> channels_key;
	std::vector<std::string> channels_origins;
	std::string err;
	std::string nick;
	std::string user;
	std::string host;
	std::string names;
	int	check = 0;
	nick = clients_map[fd].getNickname();
	user = clients_map[fd].getUsername();
	host = clients_map[fd].get_host();

    if (!clients_map[fd].IsRegistered()) // is the client authenticated in the server ?
		return (ft_errors(1, fd, nick, nick, nick));
    if (s.size() < 2) // did user provide a channel? 
		return (ft_errors(2, fd, nick, nick, nick));
	if (s[1] == "0")
	{
		handle_case_zero(fd, serv, nick, user, host);
		return ;
	}
	parsing(s,channels_name, channels_key, channels_origins);
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (size_t i = 0; i < channels_origins.size(); i++) //code in here
	{
		if (check_channel(channels_origins[i]) || channels_origins[i].size() <= 1 || channels_origins[i].size() > 200) // is the channel name valid?
		{
			ft_errors(3, fd, nick, channels_origins[i], nick);
			continue;
		}
		for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
		{
			if (ft_lower_input(it->first) == channels_origins[i])
			{
				check = 1;//to check if the channel is found
				if (it->second.check_member(fd)) // user already in channel?
				{
					break ;
				}
				if (it->second.get_members().size() >= it->second.get_channel_size()) // is the channel already full?
				{
					ft_errors(4, fd, nick, channels_origins[i], nick);
					break ;
				}
				if (it->second.get_invite_only()) // is the channel invite only?
				{
					if (it->second.check_invite(fd)) // is the user on the invite list?
					{
						it->second.add(fd);
						it->second.pop_invite(fd);
						channels[channels_origins[i]].set_channel_members(nick, false);
						ft_fill_nick(names, it->second);
						err = ":" + nick + "!" + user + "@" + host + " JOIN " + channels_origins[i] + "\r\n";
						ft_send(it->second.get_members(), err);
						if (!it->second.getTopic().empty())
						{
							ft_errors(7, fd, nick, channels_origins[i], it->second.getTopic());
							err = ":ft_irc 333 " + nick + " " + channels_origins[i] + " " + it->second.get_topic_setter() + " " + it->second.get_timestamp() + "\r\n";
							send(fd, err.c_str(), err.size() , 0);
						}
						else
							ft_errors(8, fd, nick, channels_origins[i], it->second.getTopic());
						err = ":ft_irc 353 " + nick + " = " + channels_origins[i] + " :" + names + "\r\n";
						send(fd, err.c_str(), err.size() , 0);
						ft_errors(9, fd, nick, channels_origins[i], it->second.getTopic());
						break ;
					}
					else
					{
						ft_errors(5, fd, nick, channels_origins[i], nick);
						break ;
					}
				}
				if (it->second.check_key()) // does the channel have a key?
				{
					if (s.size() < 3 || i >= channels_key.size() || it->second.get_key() != channels_key[i]) // is the password correct?
					{
						ft_errors(6, fd, nick, channels_origins[i], nick);
						break ;
					}
				}
				it->second.add(fd);
				channels[channels_origins[i]].set_channel_members(nick, false);
				ft_fill_nick(names, it->second);
				err = ":" + nick + "!" + user + "@" + host + " JOIN " + channels_origins[i] + "\r\n";
				ft_send(it->second.get_members(), err);
				if (!it->second.getTopic().empty())
				{
					ft_errors(7, fd, nick, channels_origins[i], it->second.getTopic());
					err = ":ft_irc 333 " + nick + " " + channels_origins[i] + " " + it->second.get_topic_setter() + " " + it->second.get_timestamp() + "\r\n";
					send(fd, err.c_str(), err.size() , 0);
				}
				else
					ft_errors(8, fd, nick, channels_origins[i], it->second.getTopic());
				err = ":ft_irc 353 " + nick + " = " + channels_origins[i] + " :" + names + "\r\n";
				send(fd, err.c_str(), err.size() , 0);
				ft_errors(9, fd, nick, channels_origins[i], it->second.getTopic());
				break ;
			}
		}
		if (!check)
		{
			channels[channels_origins[i]] = Channel(channels_origins[i]);
			channels[channels_origins[i]].add(fd);
			channels[channels_origins[i]].become_op(fd);
			if (i < channels_key.size())
			{
				channels[channels_origins[i]].set_key(channels_key[i]);
			}
			channels[channels_origins[i]].set_channel_members(nick, true);
			err = ":" + nick + "!" + user + "@" + host + " JOIN " + channels_origins[i] + "\r\n";
			send(fd, err.c_str(), err.size() , 0);
			ft_errors(8, fd, nick, channels_origins[i], nick);
			names = '@' + nick;
			err = ":ft_irc 353 " + nick + " = " + channels_origins[i] + " :" + names + "\r\n";
			send(fd, err.c_str(), err.size() , 0);
			ft_errors(9, fd, nick, channels_origins[i], nick);
		}
		check = 0;

	}
	return ;
}