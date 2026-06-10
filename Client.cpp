//
#include "Client.hpp"

Client::Client(int client_fd)
	: _Registered(false), _passSent(false), fd(client_fd), buffer(), nickname(), username(), authenticated(false)
{
}

int Client::getFd(void) const
{
	return fd;
}

void Client::setFd(int client_fd)
{
	fd = client_fd;
}

std::string &Client::getname(void)
{
	return nickname;
}

const std::string &Client::getBuffer(void) const
{
	return buffer;
}

void Client::setBuffer(const std::string &value)
{
	buffer = value;
}

const std::string &Client::getNickname(void) const
{
	return nickname;
}

void Client::setNickname(const std::string &value)
{
	nickname = value;
}

const std::string &Client::getUsername(void) const
{
	return username;
}

void Client::setUsername(const std::string &value)
{
	username = value;
}

bool Client::isAuthenticated(void) const
{
	return authenticated;
}

void Client::setAuthenticated(bool value)
{
	authenticated = value;
}

bool Client::IsRegistered() const
{
	return _Registered;
}

void Client::SetRegistered(bool value)
{
	_Registered = value;
}

void Client::setPassSent(bool value)
{
	_passSent = value;
}

bool Client::isPassSent() const
{
	return _passSent;
}