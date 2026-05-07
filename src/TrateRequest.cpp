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

// 1. Removido o int client_fd do construtor
TrateRequest::TrateRequest(const ParserRequest& parser_request)
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
        std::cerr << "Method not allowed: " << parser_request.method << std::endl;
    }
}

// 2. Adicionado o método que devolve a string pronta para o SocketServer
std::string TrateRequest::getResponse() const {
    return _response;
}

/****************************************************************************************************/

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
        _response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
        return;
    }

    struct stat file_stat;
    fstat(file_fd, &file_stat);
    long file_size = file_stat.st_size;
    
    char* file_content = new char[file_size];
    long bytes_read_file = read(file_fd, file_content, file_size);

    std::string header = status_header;
    std::stringstream str_size;
    str_size << file_size;
    
    header += "\r\nContent-Type: " + getContentType(file_path);
    header += "\r\nContent-Length: " + str_size.str();
    
    // 3. Keep-alive para manter o poll rodando bem
    header += "\r\nConnection: keep-alive\r\n\r\n";

    // 4. Salva a resposta em vez de usar write()
    _response = header + std::string(file_content, bytes_read_file);

    delete[] file_content;
    close(file_fd);
}

/****************************************************************************************************/

void TrateRequest::ifGet(const ParserRequest& parser_request)
{
    if (parser_request.path == "/api/curriculum")
    {
        std::string filename = "www/data/curriculum.json";
        int file_fd = open(filename.c_str(), O_RDONLY);
        
        if (file_fd < 0)
        {
            filename = "www/data/default_curriculum.json";
            file_fd = open(filename.c_str(), O_RDONLY);
        }
        
        sendPage(filename, "HTTP/1.1 200 OK");
    }
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
            std::cerr << "File not found: " << file_path << std::endl;
        }
        else
            sendPage(file_path.c_str(), "HTTP/1.1 200 OK");
    }
}

/****************************************************************************************************/

void TrateRequest::ifPost(const ParserRequest& parser_request)
{
    if (parser_request.path == "/api/curriculum")
    {
        std::string filename = "www/data/curriculum.json";
        std::string json_body = "{";
        
        std::string content_type = parser_request.headers.count("Content-Type") ? parser_request.headers.at("Content-Type") : "";
        
        if (content_type.find("multipart/form-data") != std::string::npos)
        {
            size_t boundary_pos = content_type.find("boundary=");
            if (boundary_pos == std::string::npos)
            {
                // 5. Substituído write por _response
                _response = "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nConnection: keep-alive\r\n\r\n{\"status\":\"error\",\"message\":\"No boundary\"}";
                return;
            }
            
            std::string boundary = "--" + content_type.substr(boundary_pos + 9);
            std::string body = parser_request.body;
            
            size_t pos = 0;
            bool first = true;
            
            while ((pos = body.find(boundary, pos)) != std::string::npos)
            {
                pos += boundary.length();
                if (body.substr(pos, 2) == "\r\n") pos += 2;
                
                size_t header_end = body.find("\r\n\r\n", pos);
                if (header_end == std::string::npos) break;
                
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
                
                pos = header_end + 4;
                
                size_t next_boundary = body.find(boundary, pos);
                if (next_boundary == std::string::npos) break;
                
                std::string content = body.substr(pos, next_boundary - pos - 2);
                
                if (!filename_field.empty())
                {
                    std::string safe_filename = filename_field;
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
                    
                    if (!first) json_body += ",";
                    first = false;
                    std::string json_key = (name == "photo") ? "photoUrl" : name;
                    json_body += "\"" + json_key + "\":\"/uploads/" + safe_filename + "\"";
                }
                else
                {
                    if (!first) json_body += ",";
                    first = false;
                    json_body += "\"" + name + "\":\"" + content + "\"";
                }
                pos = next_boundary;
            }
        }
        else
        {
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
            
            std::string referer = parser_request.headers.count("Referer") ? parser_request.headers.at("Referer") : "/";
            // 6. Substituído write por _response
            _response = "HTTP/1.1 302 Found\r\nLocation: " + referer + "\r\nConnection: keep-alive\r\n\r\n";
            std::cout << "[+] Curriculum data saved to " << filename << std::endl;
        }
        else
        {
            // 7. Substituído write por _response
            _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: application/json\r\nConnection: keep-alive\r\n\r\n{\"status\":\"error\"}";
            std::cerr << "Error opening file for writing: " << filename << std::endl;
        }
    }
    else
    {
        // 8. Substituído write por _response
        _response = "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\nConnection: keep-alive\r\n\r\n{\"status\":\"invalid_endpoint\"}";
    }
}

/****************************************************************************************************/

void TrateRequest::ifDelete(const ParserRequest& parser_request)
{
    if (parser_request.path == "/api/curriculum")
    {
        std::string filename = "www/data/curriculum.json";
        
        if (remove(filename.c_str()) == 0)
        {
            // 9. Substituído write por _response
            _response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: keep-alive\r\n\r\n{\"status\":\"deleted\"}";
        }
        else
        {
            // 10. Substituído write por _response
            _response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: keep-alive\r\n\r\n{\"status\":\"not_found\"}";
        }

        std::string uploads_dir = "www/uploads/";
        std::string command = "rm -f " + uploads_dir + "*";
        system(command.c_str());

        std::cout << "[+] Curriculum deleted and uploads cleaned" << std::endl;
    }
    else
    {
        sendPage("www/error/404.html", "HTTP/1.1 404 Not Found");
        std::cerr << "File not found: " << parser_request.path << std::endl;
    }
}