#include "Channel.hpp"
#include <algorithm>

Channel::Channel(std::string &channel_name)
	:name(channel_name), invite_only(false), is_key(false), channel_size(-1)
{
	std::cout << "Channel parametrized constructor.\n";//must be removed after
}
Channel::~Channel(void)
{
	std::cout << "Destructor Channel.\n";
}

Channel::Channel(const Channel &obj)
{
	*this = obj;
}

Channel &Channel::operator=(const Channel &obj)
{
	if (this != &obj)
	{
		this->name = obj.name;
		this->members = obj.members;
		this->op = obj.op;
		this->invite_only = obj.invite_only;
		this->inv_list = obj.inv_list;
		this->is_key = obj.is_key;
		this->key = obj.key;
		this->channel_size = obj.channel_size;
	}
	return (*this);
}
void	Channel::add(int fd)
{
	members.push_back(fd);
}
void	Channel::pop(int fd)
{
	std::vector<int>::iterator it = std::find(members.begin(), members.end(), fd);
	if (it != members.end())
	{
		members.erase(it);
	}
}
bool	Channel::check_member(int fd)
{
	std::vector<int>::iterator it = std::find(members.begin(), members.end(), fd);
	if (it != members.end())
	{
		return (true);
	}
	return (false);
}
void	Channel::become_op(int fd)
{
	op.push_back(fd);
}
void	Channel::pop_op(int fd)
{
	std::vector<int>::iterator it = std::find(op.begin(), op.end(), fd);
	if (it != op.end())
	{
		op.erase(it);
	}
}
bool	Channel::check_op(int fd)
{
	std::vector<int>::iterator it = std::find(op.begin(), op.end(), fd);
	if (it != op.end())
	{
		return (true);
	}
	return (false);
}

void	Channel::add_invite(int fd)
{
	inv_list.push_back(fd);
}

void	Channel::pop_invite(int fd)
{
	std::vector<int>::iterator it = std::find(inv_list.begin(), inv_list.end(), fd);
	if (it != inv_list.end())
	{
		inv_list.erase(it);
	}
}
bool	Channel::check_invite(int fd)
{
	std::vector<int>::iterator it = std::find(inv_list.begin(), inv_list.end(), fd);
	if (it != inv_list.end())
	{
		return (true);
	}
	return (false);
}

const std::string &Channel::get_name(void)
{
	return (name);
}
const std::vector<int> &Channel::get_members(void)
{
	return (members);
}
bool Channel::get_invite_only(void)
{
	return (invite_only);
}

bool Channel::check_key(void)
{
	return (is_key);
}

std::string &Channel::get_key(void)
{
	return (key);
}

int Channel::get_channel_size(void)
{
	return (channel_size);
}