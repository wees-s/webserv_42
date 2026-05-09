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
    // [ALERTA] no if delete vc usa o sytem() pra fazer rm, se eu n me engano n é função permitida 
    // e tbm ele entra em conflito com a regra de "passar tudo pelo poll" e "não bloquear o event loop"
    if (parser_request.path == "/api/curriculum")
    {
        std::string json_file = "www/data/curriculum.json";
        std::remove(json_file.c_str());

        std::string uploads_dir = "www/uploads/";
        DIR* dir = opendir(uploads_dir.c_str());
        
        if (dir != NULL)
        {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL)
            {
                std::string file_name = entry->d_name;
                
                // Ignora os diretórios de navegação do sistema "." e ".."
                if (file_name != "." && file_name != "..")
                {
                    std::string full_path = uploads_dir + file_name;
                    std::remove(full_path.c_str()); // Apaga o ficheiro
                }
            }
            closedir(dir);
        }

        _response = parser_request.version + " 204 No Content\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";

        std::cout << "[+] Curriculum deleted and uploads cleaned" << std::endl;
    }
    else
    {
        sendPage("www/error/404.html", parser_request.version + " 404 Not Found");
        std::cerr << "File not found: " << parser_request.path << std::endl;
    }
}
