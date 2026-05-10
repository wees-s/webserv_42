#include "../../include/TrateRequest.hpp"
#include <iostream>
#include <cstdio>
#include <dirent.h>

void TrateRequest::ifDelete(const ParserRequest& parser_request)
{
    // DELETE /api/curriculum - deleta o arquivo JSON salvo e limpa uploads
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
                    std::remove(full_path.c_str());
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
