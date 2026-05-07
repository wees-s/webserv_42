#include "../include/SocketServer.hpp"
#include <iostream>
#include <vector>

int main() {
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