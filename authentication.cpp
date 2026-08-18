#include "Server.hpp"

void SendMessage(int fd, const std::string& msg)
{
    send(fd, msg.c_str(), msg.size(), 0);
}

void TryRegister(Client& client)
{
    if(!client.isPassSent())
    {
        SendMessage(client.getFd(), "464 :Password required\r\n");
        return ;
    }
    if(!client.IsRegistered() && client.isAuthenticated() && !client.getNickname().empty() && !client.getUsername().empty())
    {
        client.SetRegistered(true);
        std::string serverName = "irc.server.com";
        std::string nickName = client.getNickname();
        std::string netName = "MyIRCNetwork";
        std::string Version = "1.0";
        std::string user = client.getUsername();
        SendMessage(client.getFd(), ":" + serverName + " 001 " + 
        nickName +  " :Welcome to the " + netName + " Network, "
            + nickName + "!" + user + "@localhost\r\n");
        SendMessage(client.getFd(), ":" + serverName + " 002 " + 
        nickName +  " :Your host is " + serverName + " running Version "
            + Version + "\r\n");
        SendMessage(client.getFd(), ":" + serverName + " 003 " + 
        nickName +  " :This server was created ");
        SendMessage(client.getFd(), ":" + serverName + " 004 " + 
        nickName +  " " + serverName + " " + Version + " o " + "itkol\r\n");
        SendMessage(client.getFd(),
        ":" + serverName + " 005 " + nickName +
        " NICKLEN=30 CHANNELLEN=50 CHANTYPES=# PREFIX=(o)@"
        " :are supported by this server\r\n");
        SendMessage(client.getFd(),
            ":" + serverName + " 422 " + nickName +
            " :MOTD File is missing\r\n");
        std::cout << "Client registered: " << nickName << "\n";    
    }
}




void pass(int fd, std::vector<std::string> &s, Server& serv)
{

    Client& client = serv.GetClient(fd);
    if(client.IsRegistered())
    {
        SendMessage(fd, "462 :You may not reregister\r\n");
        return ;
    }
    if(s.size() < 2)
    {
        SendMessage(fd, "461 PASS :Not enough parameters\r\n");
        return ;
    }
    if(s[1] != serv.GetPassword())
    {
        SendMessage(fd, "464 :Password incorrect\r\n");
        return ;
    }
    client.setPassSent(true);
    client.setAuthenticated(true);
    // TryRegister(client);
}

bool Server::NickIsExist(const std::string& nick)
{
    std::map<int, Client>::iterator it = clients_map.begin();
    for (; it != clients_map.end(); it++)
    {
        if (it->second.getNickname() == nick)
            return true;
    }
    return false;
}

bool isValidNick(const std::string& nick)
{
    if (nick.empty())
        return false;
    if(nick[0] == ':' || nick[0] == '#' || nick[0] == ' ')
        return false;
    for(size_t i = 0; i < nick.size(); i++)
    {
        char c = nick[i];
        if(!(isalnum(c) || c == '[' || c == ']' || c == '{'
            || c == '}' || c == '\\' || c == '|'))
                return false;
    }
    return true;
}

void nick(int fd, std::vector<std::string> &s, Server& serv)
{
    Client& client = serv.GetClient(fd);
    if(s.size() < 2)
    {
        SendMessage(fd, "431 :No nickname given\r\n");
        return ;
    }
    std::string helper = s[1];
    if(!isValidNick(helper))
    {
        SendMessage(fd, "432 :Erroneous nickname\r\n");
        return ;
    }
    if (serv.NickIsExist(helper))
    {
        SendMessage(fd, "433 :Nickname is already in use\r\n");
        return ;
    }
    client.setNickname(helper);
    TryRegister(client);
}

void user(int fd, std::vector<std::string> &s, Server& serv)
{
    Client& client = serv.GetClient(fd);

    if(client.IsRegistered())
    {
        SendMessage(fd, "462 :You may not reregister\r\n");
        return ;
    }
    if(s.size() < 5)
    {
        SendMessage(fd, "461 USER :Not enough parameters\r\n");
        return ;
    }
    if(client.getUsername().empty())
    {
        client.setUsername(s[1]);
    }
    TryRegister(client);
}