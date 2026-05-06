#include "../../include/TrateRequest.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <dirent.h>
#include <fstream>

// Redirecionamentos faltantes:
// 301 Permanent Redirect
// 302 Temporary Redirect
// Deve ser configurável por arquivo de configuração

std::string TrateRequest::generateDirectoryListing(const std::string& path, DIR* dir)
{
    std::string listing_html = "<!DOCTYPE html><html><head><title>Directory Listing</title><style>body{font-family:Arial,sans-serif;padding:20px;}h1{color:#333;}ul{list-style:none;padding:0;}li a{color:#0066cc;text-decoration:none;padding:5px;display:block;}li a:hover{background:#f0f0f0;}</style></head><body><h1>Index of " + path + "</h1><ul>";
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name != "." && name != "..")
        {
            listing_html += "<li><a href=\"" + path;
            if (!path.empty() && path[path.length() - 1] != '/')
                listing_html += "/";
            listing_html += name + "\">" + name + "</a></li>";
        }
    }
    
    listing_html += "</ul></body></html>";
    return listing_html;
}

void TrateRequest::sendDirectoryListing(const std::string& path, DIR* dir)
{
    std::string listing_html = generateDirectoryListing(path, dir);
    
    std::stringstream ss;
    ss << "www/temp_listing_" << getpid() << ".html";
    std::string temp_file = ss.str();
    
    std::ofstream file(temp_file.c_str());
    if (file.is_open())
    {
        file << listing_html;
        file.close();
        sendPage(temp_file, "HTTP/1.1 200 OK");
        std::remove(temp_file.c_str());
    }
    else
        std::cerr << "Erro ao criar arquivo temporário para listagem" << std::endl;
}

void TrateRequest::ifGet(const ParserRequest& parser_request)
{
    std::string file_path = "www";
    if (parser_request.path == "/")
        file_path += "/index.html";
    else
    {
        if (parser_request.path[0] != '/')
            file_path += "/";
        file_path += parser_request.path;
    }
    
    // API endpoint para carregar dados do currículo
    if (parser_request.path == "/api/curriculum")
    {
        std::stringstream ss;
        ss << "www/users/user" << getpid();
        std::string user_dir = ss.str();
        std::string filename = user_dir + "/curriculum.json";
        int file_fd = open(filename.c_str(), O_RDONLY);
        if (file_fd < 0)
            filename = "www/default_curriculum.json";
        else
            close(file_fd);
        sendPage(filename, "HTTP/1.1 200 OK\r\n");
    }
    // Cliente pede um diretório em vez de um arquivo
    else if (DIR* dir = opendir(file_path.c_str()))
    {
        // Tentar servir index.html
        std::string index_path = file_path + "/index.html";
        int index_fd = open(index_path.c_str(), O_RDONLY);
        if (index_fd >= 0)
        {
            close(index_fd);
            sendPage(index_path, "HTTP/1.1 200 OK");
        }
        else
            sendDirectoryListing(parser_request.path, dir);

        closedir(dir);
    }
    // GET normal para arquivos estáticos
    else
    {
        int file_fd = open(file_path.c_str(), O_RDONLY);
        if (file_fd < 0)
        {
            sendPage("www/error/404.html", "HTTP/1.1 404 Not Found");
            std::cerr << "Arquivo não encontrado: " << file_path << std::endl;
        }
        else
        {
            close(file_fd);
            sendPage(file_path.c_str(), "HTTP/1.1 200 OK");
        }
    }
}
