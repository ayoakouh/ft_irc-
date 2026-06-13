#include "Client.hpp"
#include "Server.hpp"

void kick(unsigned int fd, std::vector<std::string> &s, Server &serv)
{
	int target_fd = -1;
	std::map<int, Client> &clients_map = serv.get_clients_map();

    if (!clients_map[fd].isAuthenticated())
    {
        std::string err_authen = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err_authen.c_str(), err_authen.size() , 0);
        return;
    }
	if (s.size() != 2 && s.size() != 3)
	{
		std::cout << "wrong number of parameters.\n";
		return ;
	}
	for (std::map<int, Client>::iterator it = clients_map.begin(); it != clients_map.end(); it++)
	{
		if (it->second.getNickname() == s[2])
		{
			target_fd = it->first;
			break;
		}
	}
	if (target_fd == -1)
	{
		std::cerr << "the target client fd not found.\n";
		return ;
	}
	//if no reason is provided use a default reason, maybe the name of the one getting kicked
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		if (it->first == s[1])
		{
			if (!it->second.check_member(fd) || !it->second.check_member(target_fd))
			{
				std::cout << "the caller is not in the channel.\n";
				return ;
			}
			if (!it->second.check_op(fd))
			{
				std::cout << "the caller is not in the channel.\n";
				return ;
			}
			if (!it->second.check_member(target_fd))
			{
				std::cout << "the user getting kicked out is not in the channel.\n";
				return ;
			}
			if (it->second.check_op(target_fd))
				it->second.pop_op(target_fd);
			if (it->second.check_member(target_fd))
			{
				it->second.pop(target_fd);
				if (it->second.get_members().empty())
				{
					channels.erase(it);
				}
			}
			return ;
		}
	}
	std::cout << "channel does not exist.\n";
}