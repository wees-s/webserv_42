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
    // [AVALIAR COM CLAUDIO] lógica totalmente diferente
    if (parser_request.path == "/api/curriculum")
    {
        std::stringstream ss;
        ss << "www/users/user" << getpid();
        std::string user_dir = ss.str();
        std::string filename = user_dir + "/curriculum.json";
        
        remove(filename.c_str());
        std::string command = "rm -rf " + user_dir + "/uploads/*";
        system(command.c_str());
        
        _response = parser_request.version + " 204 No Content\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";

        std::cout << "[+] Dados do currículo deletados e uploads limpos" << std::endl;
    }
    else
    {
        sendPage("www/error/404.html", parser_request.version + " 404 Not Found");
        std::cerr << "[x] Arquivo não encontrado: " << parser_request.path << std::endl;
    }
}
