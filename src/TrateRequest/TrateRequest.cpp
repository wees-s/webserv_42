#include "../../include/TrateRequest.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <errno.h>

TrateRequest::~TrateRequest() {}

TrateRequest::TrateRequest(const ParserRequest& parser_request, const ParserConf& config) : _cgi_fd(-1), _cgi_pid(-1)
{
    std::string root = config.getRoot(parser_request.path);

    // HTTP/1.1: Múltiplos sites no mesmo IP → Host header obrigatório
    if (parser_request.version == "HTTP/1.1" && !parser_request.headers.count("Host"))
    {
        std::string error_page = root + "error/400.html";
        sendPage(error_page, parser_request.version + " 400 Bad Request");
        std::cerr << "[x] Host header ausente (HTTP/1.1 requer)" << std::endl;
        return;
    }

    // Verifica se o path é um redirecionamento do conf
    if (config.hasRedirect(parser_request.path))
    {
        int code = config.getRedirectCode(parser_request.path);
        std::string new_path = config.getRedirectPath(parser_request.path);
        
        std::stringstream ss_code;
        ss_code << code;
        
        _response = parser_request.version + " " + ss_code.str() + " Moved Permanently\r\n";
        _response += "Location: " + new_path + "\r\n";
        _response += "Content-Length: 0\r\n";
        _response += "Connection: close\r\n\r\n";
        return;
    }

    // Validação de método HTTP permitido
    if (!config.isMethodAllowed(parser_request.path, parser_request.method))
    {
        std::string error_page = root + "error/405.html";
        sendPage(error_page, parser_request.version + " 405 Method Not Allowed");
        std::cerr << "[x] Método não permitido: " << parser_request.method << std::endl;
        return;
    }

    if (parser_request.method == "GET")
        ifGet(parser_request, config);
    else if (parser_request.method == "POST")
        ifPost(parser_request, config);
    else if (parser_request.method == "DELETE")
        ifDelete(parser_request, config);
}

const std::string& TrateRequest::getResponse() const { return _response; }

bool TrateRequest::hasCGI()  const { return _cgi_fd != -1; }

int  TrateRequest::getCGIFd() const { return _cgi_fd; }

pid_t TrateRequest::getCGIPid() const { return _cgi_pid; }

std::string TrateRequest::getContentType(const std::string& file_path)
{
	if (file_path.find(".html") != std::string::npos) return "text/html";
	else if (file_path.find(".css") != std::string::npos) return "text/css";
	else if (file_path.find(".json") != std::string::npos) return "application/json";
	else if (file_path.find(".js") != std::string::npos) return "application/javascript";
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
        if (errno == EACCES)
        {
            std::cerr << "[x] Permissão negada: " << file_path << std::endl;
            _response = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
        }
        else
        {
            std::cerr << "[x] Erro ao abrir arquivo: " << file_path << std::endl;
            _response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
        }
        return;
    }

    //lê o arquivo
    struct stat file_stat;                  //struct que armazena metadados de arquivo
    fstat(file_fd, &file_stat);             //preenche file_stat com todos os metadados do arquivo aberto
    long file_size = file_stat.st_size;     //pega o dado do tamanho do arquivo de file_stat
    char* file_content = new char[file_size];
    long bytes_read_file = read(file_fd, file_content, file_size);

    //Monta o header
    std::string header = status_header;
    std::stringstream str_size;
    str_size << file_size;                  //converte o tamanho do arquivo para string
    header += "Content-Type: " + getContentType(file_path) + "\r\n";
    header += "Content-Length: " + str_size.str() + "\r\n";
    header += "Connection: keep-alive\r\n\r\n";

    _response = header + std::string(file_content, bytes_read_file);

    delete[] file_content;
    close(file_fd);
}
