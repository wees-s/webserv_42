#include "../include/SocketServer.hpp"
#include <iostream>
#include <fstream>

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

    SocketServer server;
    server.run();

    return (0);
}
