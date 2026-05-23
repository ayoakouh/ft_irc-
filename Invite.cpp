#include "Client.hpp"
#include "Server.hpp"

void invite(unsigned int fd, std::vector<std::string> &s, Server &serv)
{ 
	int target_fd;
	int exist = 0;
    if (s.size() != 3)
    {
        std::cerr << "wrong size of parameters, only three are accepted.\n";
        return ;
    }
	//check if the nickname is connected to an fd on the server.
	//if no client exist stop here and send error.
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
	// if (exist)
	// {
	// }
	std::cout << "this channel " << s[2] << " does not exist on the server.\n";

}