#include "../../include/TrateRequest.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sstream>

TrateRequest::~TrateRequest() {}

TrateRequest::TrateRequest(const ParserRequest& parser_request, int client_fd) : _client_fd(client_fd)
{
    if (parser_request.method == "GET")
        ifGet(parser_request);
    else if (parser_request.method == "POST")
        ifPost(parser_request);
    else if (parser_request.method == "DELETE")
        ifDelete(parser_request);
    else
    {
        sendPage("www/error/405.html", "HTTP/1.1 405 Method Not Allowed");
        std::cerr << "Método não permitido: " << parser_request.method << std::endl;
    }
}

std::string TrateRequest::getContentType(const std::string& file_path)
{
    if (file_path.find(".html") != std::string::npos)
        return "text/html";
    else if (file_path.find(".css") != std::string::npos)
        return "text/css";
    else if (file_path.find(".js") != std::string::npos)
        return "application/javascript";
    else if (file_path.find(".json") != std::string::npos)
        return "application/json";
    else if (file_path.find(".png") != std::string::npos)
        return "image/png";
    else if (file_path.find(".jpeg") != std::string::npos)
        return "image/jpeg";
    else if (file_path.find(".jpg") != std::string::npos)
        return "image/jpg";
    return "text/html";
}

void TrateRequest::sendPage(const std::string& file_path, const std::string& status_header)
{
    int file_fd = open(file_path.c_str(), O_RDONLY);
    if (file_fd < 0)
    {
        std::cerr << "Erro ao abrir arquivo: " << file_path << std::endl;
        return;
    }

    //lê o arquivo
    struct stat file_stat;                  //struct que armazena metadados de arquivo
    fstat(file_fd, &file_stat);             //preenche file_stat com todos os metadados do arquivo aberto
    long file_size = file_stat.st_size;     //pega o dado do tamanho do arquivo de file_stat
    char* file_content = new char[file_size + 1];
    long bytes_read_file = read(file_fd, file_content, file_size);
    file_content[bytes_read_file] = '\0';

    //Monta o header
    std::string header = status_header;
    std::stringstream str_size;
    str_size << file_size;                  //converte o tamanho do arquivo para string
    header += "Content-Type: " + getContentType(file_path);
    header += "\r\nContent-Length: " + str_size.str();
    header += "\r\nConnection: close\r\n\r\n";

    //envia o header
    write(_client_fd, header.c_str(), header.length());
    //envia o conteúdo do arquivo
    write(_client_fd, file_content, bytes_read_file);

    delete[] file_content;
    close(file_fd);
}
