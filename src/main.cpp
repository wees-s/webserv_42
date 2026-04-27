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
            std::cerr << "Error input" << std::endl;
        else
            std::cerr << "Error file" << std::endl;
        return (1);
    }

    if (!socket_server())
        return (1);
    */

    (void)argc;
    (void)argv;

    socket_server();

    return (0);
}
