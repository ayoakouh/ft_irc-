#include "Server.hpp"
#include<sstream>

void ParsePassword(std::string pass)
{
    if(pass == "")
        throw std::runtime_error("Pass is Empty; ");
    for(size_t i = 0; i < pass.size(); i++)
    {
        if(!isprint(pass[i]))
            throw std::runtime_error("Pass Containe No Printable Character !");
        if(pass[i] == ' ' || pass[i] == '\t')
            throw std::runtime_error("Pass Containe tab && spaces !");
    }
}
long ParsePort(std::string Port)
{
    if(Port.size() == 0)
        throw std::runtime_error("Port is empty;");
    char *end;
    long number = strtol(Port.c_str(), &end, 10);

    if(number < 3 || number > 65356)
        throw std::runtime_error("Port Must be > 0 && < 65356");
    if(*end != '\0')
        throw std::runtime_error("port should be intger !");
    return number;
}
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./server <port> <password>\n";
        return 1;
    }

    try
    {
        int Port = 0;
        signal(SIGPIPE, SIG_IGN);
		std::vector<std::string> s;
        std::string port = argv[1];
        std::string password = argv[2];
        ParsePassword(password);
        Port = ParsePort(port);
        Server server(Port, password);

        server.CreateServer();
        std::cout << "Server is running..." << std::endl;
        while (true)
        {
            pause();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}