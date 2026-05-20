#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

void join(unsigned int fd, std::vector<std::string> &s, Server &serv)
{
	int exist = 0;
    if (s.size() != 2 && s.size() != 3)
    {
        std::cerr << "error number of parameters\n";
        return ;
    }
    if (s[1][i].size() < 1 || s[1][i].size() > 200)
        return (std::cerr << "Invalid channel size.\n");
    for (size_t i = 0; i < s[1].size(); i++)
    {
        if (!i && s[1][i] != "#")
            return (std::cerr << "Channel name does not start with #\n");
        if (std::isspace(s[1][i]) || s[1][i] == ',' || s[1][i] == 7)
            return (std::cerr << "Not valid\n");
    }
	for (std::map<std::string, Channel>::iterator it = serv.serv_channel.begin(); it != serv.serv_channel.end(); it++)
	{
		if (it->first == s[1])
		{
			if (it->second.check_member(fd))
			{
				std::cout << "User already in channel.\n";
				return ;
			}
			if (channel_size != -1 && it->second.getmembers().size() == channel_size)
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
			else if (is->second.check_key)
			{
				if (!s[2] || it->second.get_key() != s[2])
				{
					std::cout << "wrong password.\n";
					return ;
				}
			}
			it->second.add(fd);
			return ;
		}
	}
	serv.serv_channel[s[1]] = Channel(s[1]);
	serv.serv_channel[s[1]].add(fd);
	serv.serv_channel[s[1]].become_op(fd);
	std::cout << "new channel created with name " << s[1] << "\n";
	return ;

}