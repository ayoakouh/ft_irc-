#include "Client.hpp"
#include "Server.hpp"

void invite(unsigned int fd, std::vector<std::string> &s, Server &serv)
{ 
	int target_fd = -1;
	std::map<int, Client> &clients_map = serv.get_clients_map();

    if (!clients_map[fd].isAuthenticated())
    {
        std::string err_authen = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err_authen.c_str(), err_authen.size() , 0);
        return;
    }
    if (s.size() != 3) //did the user provide both the nickname and channel name
    {
        std::cerr << "wrong size of parameters, only three are accepted.\n";
        return ;
    }
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
		if (it->first == s[2]) // does the channel exist?
		{
			if (!it->second.check_member(fd)) //is the user inviting in the channel?
			{
				std::cout << "the user inviting not in channel.\n";
				return ;
			}
			if (it->second.get_invite_only()) // is the channel an invite only?
			{
				if (!it->second.check_op(fd)) // is the inviter an operator?
				{
					std::cout << "The user inviting you is not an operator.\n";
					return ;
				}
			}
			if (it->second.check_member(target_fd)) // is the target user already in the channel?
			{
				std::cout << "the user already exists in the channel.\n";
				return ;
			}
			it->second.add_invite(target_fd);
			//send messages to the target and the caller
			return ;
		}
	}
	std::cout << "this channel " << s[2] << " does not exist on the server.\n";
}