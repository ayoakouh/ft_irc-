#include "Server.hpp"

void kick(unsigned int fd, std::vector<std::string> &s, Server &serv)
{
	int target_fd = -1;
	if (s.size() != 2 && s.size() != 3)
	{
		std::cout << "wrong number of parameters.\n";
		return ;
	}
	std::map<std::string, Channel> &channels = serv.getChannels();
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		if (it->first == s[1])
		{
			if (!it->second.check_member(fd))
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