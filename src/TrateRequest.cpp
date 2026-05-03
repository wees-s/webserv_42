#include "../include/TrateRequest.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <cstdlib>

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

/****************************************************************************************************/

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

/****************************************************************************************************/

void TrateRequest::ifGet(const ParserRequest& parser_request)
{
    // API endpoint para carregar dados do currículo
    if (parser_request.path == "/api/curriculum")
    {
        std::string filename = "www/data/curriculum.json";
        int file_fd = open(filename.c_str(), O_RDONLY);
        
        // Arquivo salvo não existe, usa o padrão
        if (file_fd < 0)
        {
            filename = "www/data/default_curriculum.json";
            file_fd = open(filename.c_str(), O_RDONLY);
        }
        
        sendPage(filename, "HTTP/1.1 200 OK\r\n");
    }
    // GET normal para arquivos estáticos
    else
    {
        std::string file_path = "www";
        if (parser_request.path == "/")
            file_path += "/index.html";
        else
            file_path += parser_request.path;

        int file_fd = open(file_path.c_str(), O_RDONLY);
        if (file_fd < 0)
        {
            sendPage("www/error/404.html", "HTTP/1.1 404 Not Found");
            std::cerr << "Arquivo não encontrado: " << file_path << std::endl;
        }
        else
            sendPage(file_path.c_str(), "HTTP/1.1 200 OK");
    }
}

/****************************************************************************************************/

void TrateRequest::ifPost(const ParserRequest& parser_request)
{
    /*(void)parser_request;
    const char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>POST received</h1>";
    write(_client_fd, response, std::strlen(response));*/
    
    // POST /api/curriculum - salva dados do currículo em arquivo JSON
    if (parser_request.path == "/api/curriculum")
    {
        std::string filename = "www/data/curriculum.json";
        std::string json_body = "{";
        
        // Check if multipart/form-data
        std::string content_type = parser_request.headers.count("Content-Type") ? parser_request.headers.at("Content-Type") : "";
        
        if (content_type.find("multipart/form-data") != std::string::npos)
        {
            // Extract boundary
            size_t boundary_pos = content_type.find("boundary=");
            if (boundary_pos == std::string::npos)
            {
                const char* response = "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"status\":\"error\",\"message\":\"No boundary\"}";
                write(_client_fd, response, std::strlen(response));
                return;
            }
            
            std::string boundary = "--" + content_type.substr(boundary_pos + 9);
            std::string body = parser_request.body;
            
            size_t pos = 0;
            bool first = true;
            
            while ((pos = body.find(boundary, pos)) != std::string::npos)
            {
                pos += boundary.length();
                
                // Skip \r\n after boundary
                if (body.substr(pos, 2) == "\r\n") pos += 2;
                
                // Find end of headers (empty line)
                size_t header_end = body.find("\r\n\r\n", pos);
                if (header_end == std::string::npos) break;
                
                // Parse headers to get name and filename
                std::string headers = body.substr(pos, header_end - pos);
                std::string name;
                std::string filename_field;
                
                size_t name_pos = headers.find("name=\"");
                if (name_pos != std::string::npos)
                {
                    name_pos += 6;
                    size_t name_end = headers.find("\"", name_pos);
                    if (name_end != std::string::npos)
                        name = headers.substr(name_pos, name_end - name_pos);
                }
                
                size_t file_pos = headers.find("filename=\"");
                if (file_pos != std::string::npos)
                {
                    file_pos += 10;
                    size_t file_end = headers.find("\"", file_pos);
                    if (file_end != std::string::npos)
                        filename_field = headers.substr(file_pos, file_end - file_pos);
                }
                
                pos = header_end + 4; // Skip \r\n\r\n
                
                // Find next boundary
                size_t next_boundary = body.find(boundary, pos);
                if (next_boundary == std::string::npos) break;
                
                // Content is between pos and next_boundary - 2 (\r\n before boundary)
                std::string content = body.substr(pos, next_boundary - pos - 2);
                
                if (!filename_field.empty())
                {
                    // Save file to disk
                    std::string safe_filename = filename_field;
                    // Simple sanitization: remove path, keep only filename
                    size_t last_slash = safe_filename.find_last_of("/\\");
                    if (last_slash != std::string::npos)
                        safe_filename = safe_filename.substr(last_slash + 1);
                    
                    std::string upload_path = "www/uploads/" + safe_filename;
                    std::ofstream file(upload_path.c_str(), std::ios::binary);
                    if (file.is_open())
                    {
                        file.write(content.c_str(), content.length());
                        file.close();
                        std::cout << "[+] Arquivo salvo: " << upload_path << std::endl;
                    }
                    
                    // Store URL in JSON (use photoUrl for photo field)
                    if (!first) json_body += ",";
                    first = false;
                    std::string json_key = (name == "photo") ? "photoUrl" : name;
                    json_body += "\"" + json_key + "\":\"/uploads/" + safe_filename + "\"";
                }
                else
                {
                    // Regular form field
                    if (!first) json_body += ",";
                    first = false;
                    json_body += "\"" + name + "\":\"" + content + "\"";
                }
                
                pos = next_boundary;
            }
        }
        else
        {
            // Parse form-urlencoded body to JSON
            std::string body = parser_request.body;
            bool first = true;
            
            size_t pos = 0;
            while ((pos = body.find('=')) != std::string::npos) {
                if (!first) json_body += ",";
                first = false;
                
                std::string key = body.substr(0, pos);
                body = body.substr(pos + 1);
                
                size_t amp_pos = body.find('&');
                std::string value;
                if (amp_pos != std::string::npos) {
                    value = body.substr(0, amp_pos);
                    body = body.substr(amp_pos + 1);
                } else {
                    value = body;
                }
                
                // URL decode simple implementation
                for (size_t i = 0; i < value.length(); i++) {
                    if (value[i] == '+') value[i] = ' ';
                }
                
                json_body += "\"" + key + "\":\"" + value + "\"";
            }
        }
        
        json_body += "}";
        
        std::ofstream file(filename.c_str());
        if (file.is_open())
        {
            file << json_body;
            file.close();
            
            // Redirect back to the same page using Referer header
            std::string referer = parser_request.headers.count("Referer") ? parser_request.headers.at("Referer") : "/";
            std::string response = "HTTP/1.1 302 Found\r\nLocation: " + referer + "\r\n\r\n";
            write(_client_fd, response.c_str(), response.length());
            std::cout << "[+] Dados do currículo salvos em " << filename << std::endl;
        }
        else
        {
            const char* response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: application/json\r\n\r\n{\"status\":\"error\"}";
            write(_client_fd, response, std::strlen(response));
            std::cerr << "Erro ao abrir arquivo para escrita: " << filename << std::endl;
        }
    }
    else
    {
        const char* response = "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"status\":\"invalid_endpoint\"}";
        write(_client_fd, response, std::strlen(response));
    }
}

/****************************************************************************************************/

void TrateRequest::ifDelete(const ParserRequest& parser_request)
{
    // DELETE /api/curriculum - deleta o arquivo JSON salvo e limpa uploads
    if (parser_request.path == "/api/curriculum")
    {
        std::string filename = "www/data/curriculum.json";
        
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

        std::string uploads_dir = "www/uploads/";
        std::string command = "rm -f " + uploads_dir + "*";
        system(command.c_str());

        std::cout << "[+] Dados do currículo deletados e uploads limpos" << std::endl;
    }
    else
    {
        sendPage("www/error/404.html", "HTTP/1.1 404 Not Found");
        std::cerr << "Arquivo não encontrado: " << parser_request.path << std::endl;
    }
}
