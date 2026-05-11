#include "Server.hpp"


#include "Server.hpp"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./server <port> <password>\n";
        return 1;
    }

    try
    {
        int port = std::atoi(argv[1]);
        std::string password = argv[2];

        Server server(port, password);

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