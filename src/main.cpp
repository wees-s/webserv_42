#include "../include/ParserRequest.hpp"
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

    std::string request_str = socket_server();
    if (request_str.empty())
        return (1);

    std::cout << "\n=== REQUEST RECEBIDA (RAW) ===" << std::endl;
    std::cout << request_str << std::endl;

    ParserRequest request(request_str);

    std::cout << "\n=== REQUEST PARSEADA ===" << std::endl;
    std::cout << "Method: " << request.method << std::endl;
    std::cout << "Path: " << request.path << std::endl;
    std::cout << "Version: " << request.version << std::endl;
    std::cout << "Body: [" << request.body << "]" << std::endl;
    std::cout << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = request.headers.begin(); it != request.headers.end(); ++it)
        std::cout << "  " << it->first << ": " << it->second << std::endl;

    return (0);
}
