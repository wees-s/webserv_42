#include "../include/Request.hpp"

#include <iostream>
//#include <netinet/in.h>

/*std::string prepare_request()
{
    int server_fd;
    int client_fd;
    sockaddr_in addr;

    int bytes;
    char buffer[4096];
    std::string request;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    client_fd = accept(server_fd, NULL, NULL);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    while ((bytes = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) 
    {
        request.append(buffer, bytes);
        if (request.find("\r\n\r\n") != std::string::npos)
            break ;
    }

    return (request);
}*/

int main(int argc, char **argv)
{
    /*std::ifstream file.open(argv[1]);
    if (argc != 2 || !file)
    {
        if (argc != 2)
            std::cerr << "Error input" << std::endl;
        else
            std::cerr << "Error file" << std::endl;
        return (1);
    }*/

    //std::string req = prepare_request();
    std::string req = "GET /sobre HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\n";  
    Request request(req);


    std::cout << request.method << std::endl;
    std::cout << request.path << std::endl; 
    std::cout << request.version << std::endl; 
    std::cout << request.body << std::endl; 
    for (const auto& header : request.headers)
        std::cout << header.first << ": " << header.second << std::endl;
    

    return (0);
}
