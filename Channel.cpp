#include "Channel.hpp"
#include <algorithm>

Channel::Channel(void)
{
	std::cout << "Default Constructor\n";
}

Channel::Channel(std::string &channel_name)
	:name(channel_name), invite_only(false), is_key(false), channel_size(-1), topic(""), topic_restricted(false)
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
		this->topic = obj.topic;
		this->topic_restricted = obj.topic_restricted;
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


size_t Channel::get_channel_size(void)
{
	return (channel_size);
}


std::string Channel::getTopic()
{
	return topic;
}

void Channel::setTopic(const std::string &new_topic)
{
	topic = new_topic;
}

bool Channel::isTopicRestricted()
{
	return topic_restricted;
}

void Channel::set_Topic_Restricted(bool status)
{
	topic_restricted = status;
}


void Channel::set_invite_only(bool status_of_invite_only)
{
	invite_only = status_of_invite_only;
}

void Channel::set_key(const std::string &new_key)
{
	key = new_key;
	is_key = true;
}


void Channel::remove_key()
{
	key = "";
	is_key = false;
}

void Channel::set_channel_size(int new_size)
{
	channel_size = new_size;
}  
