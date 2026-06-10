#include "Client.hpp"
#include "Server.hpp"

void invite(unsigned int fd, std::vector<std::string> &s, Server &serv)
{ 
	int target_fd = -1;
    if (s.size() != 3)
    {
        std::cerr << "wrong size of parameters, only three are accepted.\n";
        return ;
    }
	std::map<int, Client> &clients_map = serv.get_clients_map();
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
		std::cerr << "the target client fd not found.\n";
		return ;
	}
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		if (it->first == s[2])
		{
			if (!it->second.check_member(fd))
			{
				std::cout << "the user inviting not in channel.\n";
				return ;
			}
			if (it->second.get_invite_only())
			{
				if (!it->second.check_op(fd))
				{
					std::cout << "The user inviting you is not an operator.\n";
					return ;
				}
			}
			if (it->second.check_member(target_fd))
			{
				std::cout << "the user already exists in the channel.\n";
				return ;
			}
			it->second.add_invite(target_fd);
			//send messages to the target and the caller
		}
	}
	std::cout << "this channel " << s[2] << " does not exist on the server.\n";
}