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
        
        remove(filename.c_str());
        
        // [CORREÇÃO]: Substituído write() por _response. 
        // Adicionado Keep-Alive e Content-Length: 0 (já que o 204 No Content não tem corpo)
        _response = parser_request.version + " 204 No Content\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";

        std::string command = "rm -rf " + user_dir + "/uploads/*";
        system(command.c_str());

        std::cout << "[+] Curriculum deleted and uploads cleaned" << std::endl;
    }
    else
    {
        // O sendPage já foi adaptado no TrateRequest.cpp para usar _response, então esta chamada está segura!
        sendPage("www/error/404.html", parser_request.version + " 404 Not Found");
        std::cerr << "File not found: " << parser_request.path << std::endl;
    }
}
