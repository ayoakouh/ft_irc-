#include "Client.hpp"
#include "Server.hpp"
#include "Channel.hpp"

std::string ft_lower_input(const std::string &channel)
{
	std::string copy;
	for (size_t i = 0; i < channel.size();i++)
		copy += std::tolower(channel[i]);
	return (copy);
}