#include "../../include/TrateRequest.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sstream>

TrateRequest::~TrateRequest() {}

// [CORREÇÃO 1]: O construtor recebe apenas o parser_request. Ele mesmo começa a montar a _response.
TrateRequest::TrateRequest(const ParserRequest& parser_request)
{
    // HTTP/1.1: Múltiplos sites no mesmo IP → Host header obrigatório
    if (parser_request.version == "HTTP/1.1" && !parser_request.headers.count("Host"))
    {
        sendPage("www/error/400.html", parser_request.version + " 400 Bad Request");
        std::cerr << "Host header ausente (HTTP/1.1 requer)" << std::endl;
        return;
    }

    if (parser_request.method == "GET")
        ifGet(parser_request);
    else if (parser_request.method == "POST")
        ifPost(parser_request);
    else if (parser_request.method == "DELETE")
        ifDelete(parser_request);
    else
    {
        sendPage("www/error/405.html", parser_request.version + " 405 Method Not Allowed");
        std::cerr << "Method not allowed: " << parser_request.method << std::endl;
    }
}

// [MÉTODO NOVO]: Para (SocketServer) conseguir pegar a string gigante pronta!
std::string TrateRequest::getResponse() const {
    return _response;
}

std::string TrateRequest::getContentType(const std::string& file_path)
{
    if (file_path.find(".html") != std::string::npos) return "text/html";
    else if (file_path.find(".css") != std::string::npos) return "text/css";
    else if (file_path.find(".js") != std::string::npos) return "application/javascript";
    else if (file_path.find(".json") != std::string::npos) return "application/json";
    else if (file_path.find(".png") != std::string::npos) return "image/png";
    else if (file_path.find(".jpeg") != std::string::npos) return "image/jpeg";
    else if (file_path.find(".jpg") != std::string::npos) return "image/jpg";
    return "text/html";
}

void TrateRequest::sendPage(const std::string& file_path, const std::string& status_header)
{
    int file_fd = open(file_path.c_str(), O_RDONLY);
    if (file_fd < 0)
    {
        std::cerr << "Error opening file: " << file_path << std::endl;
        // Se der erro, preparamos uma string de resposta 404 mínima para não bugar o servidor
        _response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
        return;
    }

    struct stat file_stat;
    fstat(file_fd, &file_stat);
    long file_size = file_stat.st_size;
    
    // [CORREÇÃO 2]: Tiramos o +1 para não corromper imagens e arquivos binários
    char* file_content = new char[file_size];
    long bytes_read_file = read(file_fd, file_content, file_size);

    std::string header = status_header;
    std::stringstream str_size;
    str_size << file_size;
    
    header += "\r\nContent-Type: " + getContentType(file_path);
    header += "\r\nContent-Length: " + str_size.str();
    
    // [CORREÇÃO 3]: Trocamos close por keep-alive para não derrubar sua infraestrutura
    header += "\r\nConnection: keep-alive\r\n\r\n";

    // [CORREÇÃO 4]: ADEUS WRITE()! Nós juntamos Header + Body na variável _response.
    _response = header + std::string(file_content, bytes_read_file);

    delete[] file_content;
    close(file_fd);
}