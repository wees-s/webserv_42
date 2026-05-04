#include "../include/SocketServer.hpp"
#include <iostream>

int main() {
    std::cout << "Iniciando o Webserv (Modulo de Infraestrutura)..." << std::endl;
    
    SocketServer server;
    server.run(); // O programa fica preso no event loop aqui
    
    std::cout << "Webserv encerrado com seguranca. FDs limpos." << std::endl;
    return 0;
}