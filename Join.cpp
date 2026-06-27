#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

void join(unsigned int fd, std::vector<std::string> &s, Server &serv)
{
	std::map<int, Client> &clients_map = serv.get_clients_map();

    if (!clients_map[fd].isAuthenticated())
    {
        std::string err_authen = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err_authen.c_str(), err_authen.size() , 0);
        return;
    }
    if (s.size() != 2 && s.size() != 3)
    {
        std::cerr << "error number of parameters\n";
        return ;
    }
    if (s[1].size() <= 1 || s[1].size() > 200)
	{
		std::cerr << "Invalid channel size.\n";
		return ;
	}
    for (size_t i = 0; i < s[1].size(); i++)
    {
        if (!i && s[1][i] != '#')
		{
			std::cerr << "Channel name does not start with #\n";
			return ;
		}
        if (std::isspace(s[1][i]) || s[1][i] == ',' || s[1][i] == 7)
		{
			std::cerr << "Not valid\n";
			return ;
		}
    }
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		if (it->first == s[1])
		{
			if (it->second.check_member(fd))
			{
				std::cout << "User already in channel.\n";
				return ;
			}
			if (it->second.get_members().size() >= it->second.get_channel_size())
			{
				std::cout << "channel is full.\n";
				return ;
			}
			if (it->second.get_invite_only())
			{
				if (it->second.check_invite(fd))
				{
					it->second.add(fd);
					it->second.pop_invite(fd);
					return ;
				}
				else
				{
					std::cout << "not invited\n";
					return;
				}
			}
			else if (it->second.check_key())
			{
				if (s.size() < 3 || it->second.get_key() != s[2])
				{
					std::cout << "wrong password.\n";
					return ;
				}
			}
			it->second.add(fd);
			return ;
		}
	}
	channels[s[1]] = Channel(s[1]);
	//channels.insert(std::make_pair(s[1], Channel(s[1])));
	channels[s[1]].add(fd);
	channels[s[1]].become_op(fd);
	std::cout << "new channel created with name " << s[1] << "\n";
	return ;

}