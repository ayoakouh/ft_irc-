#include "Client.hpp"
#include "Server.hpp"

void  
{
	int target_fd;
	if (s.size() != 2 && s.size() != 3)
	{
		std::cout << "wrong number of parameters.\n";
		return ;
	}
	//if no reason is provided use a default reason, maybe the name of the one getting kicked
	for (std::map<std::string, Channel>::iterator it = serv.serv_channel.begin(); it != serv.serv_channel.end(); it++)
	{
		if (it->first == s[1])
		{
			//check if the nickname is connected to an fd on the server.
	//if no client exist stop here and send error.
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
			//broadcast
			if (it->second.check_op(target_fd))
			{
				it->second.pop_op(target_fd);
			}
			if (it->second.check_member(target_fd))
			{
				it->second.pop(target_fd);
				if (!it->second.get_members())
				{
					serv.serv_channel.erase(it);
				}
			}
		}
	}
	std::cout << "channel does not exist.\n";
	return ;
}