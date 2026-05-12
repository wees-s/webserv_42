#include "../include/SocketServer.hpp"
#include "../Conf/ParserConf.hpp"
#include <iostream>
#include <vector>

int main(int argc, char **argv)
{
    /*std::ifstream file;

    file.open(argv[1]);
    if (argc != 2 || !file)
    {
        if (argc != 2)
            std::cerr << "[x] Error input" << std::endl;
        else
            std::cerr << "[x] Error file" << std::endl;
        return (1);
    }
    */
    (void)argc;
    (void)argv;

    std::cout << "Starting the Webserv (Infrastructure Module)..." << std::endl;
    
    ParserConf conf;
    SocketServer server(conf);
    server.run(); // O programa fica preso no event loop aqui
    
    std::cout << "Webserv closed with security. FDs cleaned." << std::endl;
    return 0;
}