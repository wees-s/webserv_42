#include "../../include/TrateRequest.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sstream>

void TrateRequest::ifDelete(const ParserRequest& parser_request)
{
    // DELETE /api/curriculum - deleta o arquivo JSON salvo e limpa uploads
    if (parser_request.path == "/api/curriculum")
    {
        std::stringstream ss;
        ss << "www/users/user" << getpid();
        std::string user_dir = ss.str();
        std::string filename = user_dir + "/curriculum.json";
        
        if (remove(filename.c_str()) == 0)
        {
            const char* response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"deleted\"}";
            write(_client_fd, response, std::strlen(response));
        }
        else
        {
            // Arquivo não existe, mas isso não é erro
            const char* response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"not_found\"}";
            write(_client_fd, response, std::strlen(response));
        }

        std::string command = "rm -rf " + user_dir + "/uploads/*";
        system(command.c_str());

        std::cout << "[+] Dados do currículo deletados e uploads limpos" << std::endl;
    }
    else
    {
        sendPage("www/error/404.html", "HTTP/1.1 404 Not Found");
        std::cerr << "Arquivo não encontrado: " << parser_request.path << std::endl;
    }
}
