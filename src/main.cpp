#include "../include/Request.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char **argv)
{
    std::ifstream file;

    file.open(argv[1]);
    if (argc != 2 || !file)
    {
        if (argc != 2)
            std::cerr << "Error input" << std::endl;
        else
            std::cerr << "Error file" << std::endl;
        return (1);
    }
    /*TESTE PARSER REQUEST
    std::string req = "GET /sobre HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\n";  
    Request request(req);


    std::cout << request.method << std::endl;
    std::cout << request.path << std::endl; 
    std::cout << request.version << std::endl; 
    std::cout << request.body << std::endl; 
    for (const auto& header : request.headers)
        std::cout << header.first << ": " << header.second << std::endl;
    */
    return (0);
}
