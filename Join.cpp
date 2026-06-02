#include "Server.hpp"
#include <cctype>

void join(unsigned int fd, std::vector<std::string> &s, Server &serv)
{
    if (s.size() != 2 && s.size() != 3)
    {
        std::cerr << "error number of parameters\n";
        return ;
    }
    if (s[1].size() < 1 || s[1].size() > 200)
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
	std::map<std::string, Channel>::iterator it = channels.find(s[1]);
	if (it == channels.end())
	{
		Channel new_channel(s[1]);
		std::pair<std::map<std::string, Channel>::iterator, bool> inserted =
			channels.insert(std::make_pair(s[1], new_channel));
		it = inserted.first;
		it->second.become_op(fd);
	}
	if (it->second.check_member(fd))
	{
		std::cout << "User already in channel.\n";
		return ;
	}
	if (it->second.get_channel_size() != -1 && it->second.get_members().size() >= static_cast<size_t>(it->second.get_channel_size()))
	{
		std::cout << "channel is full.\n";
		return ;
	}
	if (it->second.get_invite_only())
	{
		if (!it->second.check_invite(fd))
		{
			std::cout << "not invited\n";
			return ;
		}
		it->second.pop_invite(fd);
	}
	if (s.size() == 3 && it->second.check_key())
	{
		if (it->second.get_key() != s[2])
		{
			std::cout << "wrong password.\n";
			return ;
		}
	}
	it->second.add(fd);
	std::cout << "new channel created with name " << s[1] << "\n";
	return ;

}