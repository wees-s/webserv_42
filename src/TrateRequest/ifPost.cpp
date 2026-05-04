#include "../../include/TrateRequest.hpp"
#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>

void TrateRequest::ifPost(const ParserRequest& parser_request)
{
    /*(void)parser_request;
    const char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>POST received</h1>";
    write(_client_fd, response, std::strlen(response));*/

    /**************** SOLUÇÃO TEMPORÁRIA ****************/

    // POST /api/curriculum - salva dados do currículo em arquivo JSON
    if (parser_request.path == "/api/curriculum")
    {
        // Create user directory if it doesn't exist
        std::stringstream ss;
        ss << "www/users/user" << getpid();
        std::string user_dir = ss.str();
        std::string command = "mkdir -p " + user_dir + "/uploads";
        system(command.c_str());
        
        std::string filename = user_dir + "/curriculum.json";
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
                    
                    // Remove spaces from filename
                    size_t space_pos = 0;
                    while ((space_pos = safe_filename.find(' ', space_pos)) != std::string::npos)
                        safe_filename.replace(space_pos, 1, "_");
                    
                    std::string upload_path = user_dir + "/uploads/" + safe_filename;
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
                    std::string url_path = user_dir.substr(4); // Remove "www" prefix
                    json_body += "\"" + json_key + "\":\"" + url_path + "/uploads/" + safe_filename + "\"";
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
