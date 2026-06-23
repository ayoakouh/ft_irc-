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

void	parsing(std::vector<std::string> &s, std::vector<std::string> &channels_name, std::vector<std::string> &channels_key)
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
        if (std::isspace(s[i]) || s[i] == ',' || s[i] == 7)
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
	int	check = 0;

    if (s.size() < 2) // did user provide a channel?
    {
        std::cerr << "error number of parameters\n";
        return ;
    }
    if (!clients_map[fd].IsRegistered()) // is the client authenticated in the server ?
    {
        std::string err_authen = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err_authen.c_str(), err_authen.size() , 0);
        return;
    }
	if (s[1] == "0")
	{
		handle_case_zero(fd, serv);
		return ;
	}
	parsing(s,channels_name, channels_key);
	//printing the vector
	for (size_t i = 0; i < channels_name.size();i++)
	{
		std::cout << "hello " << channels_name[i] <<"\n";
	}
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (size_t i = 0; i < channels_name.size(); i++) //code in here
	{
		if (channels_name[i].size() <= 1 || channels_name[i].size() > 200) // is the channel name valid?
		{
			std::cerr << "Invalid channel size.\n";
			// return ;
			continue;
		}
		if (check_channel(channels_name[i]))
		{
			std::cerr << "Not valid name.\n";
			continue ;
		}
		for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
		{
			if (it->first == channels_name[i])
			{
				check = 1;
				if (it->second.check_member(fd)) // user already in channel?
				{
					std::cout << "User already in channel.\n";
					break ;
				}
				if (it->second.get_members().size() >= it->second.get_channel_size()) // is the channel already full?
				{
					std::cout << "channel is full.\n";
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
						std::cout << "not invited\n";
						break ;
					}
				}
				if (it->second.check_key()) // does the channel have a key?
				{
					if (s.size() < 3 || i >= channels_key.size() || it->second.get_key() != channels_key[i]) // is the password correct?
					{
						std::cout << "wrong password.\n";
						break ;
					}
				}
				it->second.add(fd);
				break ;
			}
		}
		if (!check)
		{
			std::cerr << "heyhey.\n";
			channels[channels_name[i]] = Channel(channels_name[i]);
			channels[channels_name[i]].add(fd);
			channels[channels_name[i]].become_op(fd);
			if (i < channels_key.size())
			{
				channels[channels_name[i]].set_key(channels_key[i]);
			}
			std::cout << "new channel created with name " << channels_name[i] << "\n";
		}
		check = 0;

	}
	return ;

}