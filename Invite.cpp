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
	for (size_t i = 0; i < serv.clients.size(); i++)
	{
		if (serv.clients[i].getname() == s[1])
		{
			target_fd = serv.clients[i].getfd();
			break;
		}
	}
	if (target_fd == -1)
	{
		std::cerr << "the target client fd not found.\n";
		return ;
	}
	for (std::map<std::string, Channel>::iterator it = serv.serv_channel.begin(); it != serv.serv_channel.end(); it++)
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