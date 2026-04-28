#include "../include/TrateRequest.hpp"
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
        //temporário, deve entregar a página de erro correspondente
        const char* not_allowed = "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/html\r\n\r\n<h1>405 - Method Not Allowed</h1>";
        write(_client_fd, not_allowed, std::strlen(not_allowed));

        //sendPage("/error/405.html", "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/html\r\nContent-Length: ");
        std::cerr << "Método não permitido: " << parser_request.method << std::endl;
    }
}

void TrateRequest::sendPage(const std::string& file_path, const std::string& status_header)
{
    int file_fd = open(file_path.c_str(), O_RDONLY);
    if (file_fd < 0)
    {
        std::cerr << "Erro ao abrir arquivo: " << file_path << std::endl;
        return;
    }

    struct stat file_stat;                  //struct que armazena metadados de arquivo
    fstat(file_fd, &file_stat);             //preenche file_stat com todos os metadados do arquivo aberto
    long file_size = file_stat.st_size;     //pega o dado do tamanho do arquivo de file_stat

    char* file_content = new char[file_size + 1];
    long bytes_read_file = read(file_fd, file_content, file_size);

    std::stringstream str_size;
    str_size << file_size;
    std::string header = status_header;
    header += str_size.str();
    header += "\r\n\r\n";

    //header montado. Write envia o header
    write(_client_fd, header.c_str(), header.length());
    //envia o conteúdo do arquivo
    write(_client_fd, file_content, bytes_read_file);

    delete[] file_content;
    close(file_fd);
}

void TrateRequest::ifGet(const ParserRequest& parser_request)
{
    std::string file_path = "www";
    if (parser_request.path == "/")
        file_path += "/index.html";
    else
        file_path += parser_request.path;

    int file_fd = open(file_path.c_str(), O_RDONLY);
    if (file_fd < 0)
    {
        //temporário, deve entregar a página de erro correspondente
        const char* not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 - File Not Found</h1>";
        write(_client_fd, not_found, std::strlen(not_found));

        //sendPage("/error/404.html", "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: ");
        std::cerr << "Arquivo não encontrado: " << file_path << std::endl;
    }
    else
        sendPage(file_path.c_str(), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ");
}

// tratamento temporário
void TrateRequest::ifPost(const ParserRequest& parser_request)
{
    (void)parser_request;
    const char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>POST received</h1>";
    write(_client_fd, response, std::strlen(response));
}

// tratamento temporário
void TrateRequest::ifDelete(const ParserRequest& parser_request)
{
    (void)parser_request;
    const char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>DELETE received</h1>";
    write(_client_fd, response, std::strlen(response));
}
