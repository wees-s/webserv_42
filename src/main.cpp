#include "../include/SocketServer.hpp"
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
    
    std::vector<int> ports;
    ports.push_back(8080);
    ports.push_back(8081);
    ports.push_back(8082);
    ports.push_back(8083);
    SocketServer server(ports);
    server.run(); // O programa fica preso no event loop aqui
    
    std::cout << "Webserv closed with security. FDs cleaned." << std::endl;
    return 0;
}