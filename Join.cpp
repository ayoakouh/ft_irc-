#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

void join(std::vector<std::string> &s, Server &serv)
{
	int key = 0;
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
	if (s[2])
		key = 1;
	for (std::map<std::string, Channel>::iterator it = serv.serv_channel.begin(); it != serv.serv_channel.end(); it++)
	{
		if (it->first == s[1])
		{
			
		}
	}


}