#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

void	handle_case_zero(unsigned int fd, Server &serv)
{
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		if (it->second.check_member(fd))
		{
			it->second.pop(fd);	
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
        if (!i && s[i] != '#')
		{
			std::cerr << "Channel name does not start with #\n";
			return (1);
		}
        if (std::isspace(s[i]) || s[i] == ',' || s[i] == 7) //must revise before push
		{
			std::cerr << "Not valid\n";
			return (1);
		}
    }
	return (0);
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
	int	check = 0;
	nick = clients_map[fd].getNickname();
	user = clients_map[fd].getUsername();
	host = clients_map[fd].get_host();

    if (!clients_map[fd].IsRegistered()) // is the client authenticated in the server ?
    {
        err = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err.c_str(), err.size() , 0);
        return;
    }
    if (s.size() < 2) // did user provide a channel? 
    {
		err = ":ft_irc 461 " + nick + " JOIN :Not enough parameters\r\n";
        send(fd, err.c_str(), err.size() , 0);
        return ;
    }
	if (s[1] == "0")
	{
		handle_case_zero(fd, serv);
		return ;
	}
	parsing(s,channels_name, channels_key, channels_origins);
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (size_t i = 0; i < channels_name.size(); i++) //code in here
	{
		if (channels_name[i].size() <= 1 || channels_name[i].size() > 200) // is the channel name valid?
		{
			std::cerr << "Invalid channel size.\n"; //error not valid
			continue;
		}
		if (check_channel(channels_name[i]))
		{
			err = ":ft_irc 476 " + nick + " " + channels_name[i] + " :Bad Channel Mask\r\n";
			send(fd, err.c_str(), err.size() , 0);
			continue ;
		}
		for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
		{
			if (it->first == channels_name[i])
			{
				check = 1;
				if (it->second.check_member(fd)) // user already in channel?
				{
					break ;
				}
				if (it->second.get_members().size() >= it->second.get_channel_size()) // is the channel already full?
				{
					err = ":ft_irc 471 " + nick + " " + it->first + " :Cannot join channel (+l)\r\n";
					send(fd, err.c_str(), err.size() , 0);
					break ;
				}
				if (it->second.get_invite_only()) // is the channel invite only?
				{
					if (it->second.check_invite(fd)) // is the user on the invite list?
					{
						it->second.add(fd);
						it->second.pop_invite(fd);
						break ;
					}
					else
					{
						err = ":ft_irc 473 " + nick + " " + it->first + " :Cannot join channel (+i)\r\n";
						send(fd, err.c_str(), err.size() , 0);
						break ;
					}
				}
				if (it->second.check_key()) // does the channel have a key?
				{
					if (s.size() < 3 || i >= channels_key.size() || it->second.get_key() != channels_key[i]) // is the password correct?
					{
						err = ":ft_irc 475 " + nick + " " + it->first + " :Cannot join channel (+k)\r\n";
						send(fd, err.c_str(), err.size() , 0);
						break ;
					}
				}
				it->second.add(fd);
				if (!it->second.getTopic().empty())
				{
					err =":ft_irc 332 " + nick + " " + it->first + " :" + it->second.getTopic() + "\r\n";
					send(fd, err.c_str(), err.size() , 0);
				}
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
			err = ":" + nick + "!" + user + "@" + host + " JOIN " + channels_origins[i] + "\r\n";
			send(fd, err.c_str(), err.size() , 0);
		}
		check = 0;

	}
	return ;

}