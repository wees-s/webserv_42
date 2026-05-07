#include "../../include/TrateRequest.hpp"
#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>

std::string createUserDirectory()
{
    std::stringstream ss;
    ss << "www/users/user" << getpid();
    std::string user_dir = ss.str();
    std::string command = "mkdir -p " + user_dir + "/uploads";
    system(command.c_str());

    return user_dir;
}

std::string TrateRequest::postMultipartFormData(const std::string& user_dir, const std::string& content_type, const ParserRequest& parser_request)
{
    std::string body = parser_request.body;
    std::string json_body = "{";
    std::string boundary;
    size_t start_boundary = 0;
    size_t end_boundary = 0;
    bool first = true;

    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos)
    {
        sendPage("www/error/400.html", parser_request.version + " 400 Bad Request");
        return "";
    }
    boundary = "--" + content_type.substr(boundary_pos + 9);
    
    while ((start_boundary = body.find(boundary, start_boundary)) != std::string::npos)
    {
        start_boundary += boundary.length();
        if (body.substr(start_boundary, 2) == "\r\n")
            start_boundary += 2;
        
        size_t header_end = body.find("\r\n\r\n", start_boundary);
        if (header_end == std::string::npos)
            break;
        std::string header = body.substr(start_boundary, header_end - start_boundary);
        
        std::string name;
        size_t name_pos = header.find("name=\"");
        if (name_pos != std::string::npos)
        {
            name_pos += 6;
            size_t name_end = header.find("\"", name_pos);
            if (name_end != std::string::npos)
                name = header.substr(name_pos, name_end - name_pos);
        }

        std::string filename;
        size_t file_pos = header.find("filename=\"");
        if (file_pos != std::string::npos)
        {
            file_pos += 10;
            size_t file_end = header.find("\"", file_pos);
            if (file_end != std::string::npos)
                filename = header.substr(file_pos, file_end - file_pos);
        }
        
        start_boundary = header_end + 4;
        
        end_boundary = body.find(boundary, start_boundary);
        if (end_boundary == std::string::npos)
            break;
        
        std::string content = body.substr(start_boundary, end_boundary - start_boundary - 2);
        
        if (!filename.empty())
        {
            size_t last_slash = filename.find_last_of("/\\");
            if (last_slash != std::string::npos)
                filename = filename.substr(last_slash + 1);
            
            size_t space_pos = 0;
            while ((space_pos = filename.find(' ', space_pos)) != std::string::npos)
                filename.replace(space_pos, 1, "_");
            
            std::string upload_path = user_dir + "/uploads/" + filename;
            std::ofstream file(upload_path.c_str(), std::ios::binary);
            if (file.is_open())
            {
                file.write(content.c_str(), content.length());
                file.close();
            }
            
            if (!first)
                json_body += ",";
            first = false;

            std::string url_path = user_dir.substr(4);
            json_body += "\"photoUrl\":\"" + url_path + "/uploads/" + filename + "\"";
        }
        else
        {
            if (!first)
                json_body += ",";
            first = false;
            json_body += "\"" + name + "\":\"" + content + "\"";
        }
        
        start_boundary = end_boundary;
    }

    json_body += "}";
    return json_body;
}

std::string TrateRequest::postFormData(const ParserRequest& parser_request)
{
    std::string body = parser_request.body;
    std::string json_body = "{";
    std::string key;
    std::string value;
    size_t pos_key = 0;
    size_t pos_value = 0;
    bool first = true;
    
    while ((pos_key = body.find('=')) != std::string::npos)
    {
        if (!first)
            json_body += ",";
        first = false;
        
        key = body.substr(0, pos_key);
        body = body.substr(pos_key + 1);
        
        pos_value = body.find('&');
        if (pos_value != std::string::npos) 
        {
            value = body.substr(0, pos_value);
            body = body.substr(pos_value + 1);
        } 
        else
            value = body;
        
        for (size_t i = 0; i < value.length(); i++) 
        {
            if (value[i] == '+') 
                value[i] = ' ';
        }
        
        json_body += "\"" + key + "\":\"" + value + "\"";
    }

    json_body += "}";
    return json_body;
}

void TrateRequest::ifPost(const ParserRequest& parser_request)
{
    if (parser_request.body.size() > 1024 * 1024) 
    {
        sendPage("www/error/413.html", parser_request.version + " 413 Payload Too Large");
        std::cerr << "Body com tamanho maior que 1MB" << std::endl;
        return;
    }

    if (parser_request.path == "/api/curriculum")
    {
        std::string user_dir = createUserDirectory();
        std::string json_body;
        std::string content_type;

        if (parser_request.headers.count("Content-Type"))
            content_type = parser_request.headers.at("Content-Type");
        else
            content_type = "";
        
        if (content_type.find("multipart/form-data") != std::string::npos)
            json_body = postMultipartFormData(user_dir, content_type, parser_request);
        else
            json_body = postFormData(parser_request);
                
        std::string filename = user_dir + "/curriculum.json";
        std::ofstream file(filename.c_str());
        if (file.is_open())
        {
            file << json_body;
            file.close();
            
            std::string response = parser_request.version + " 302 Found\r\nLocation: ";
            if (parser_request.headers.count("Referer"))
                response += parser_request.headers.at("Referer");
            else
                response += "/";
            
            // Adicionado Keep-Alive e atribuido a variavel _response em vez de usar write()
            response += "\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
            _response = response;
            
            std::cout << "[+] Curriculum data saved to " << filename << std::endl;
        }
        else
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "Error opening file for writing: " << filename << std::endl;
        }
    }
    else
    {
        sendPage("www/error/404.html", parser_request.version + " 404 Not Found");
        std::cerr << "File not found: " << parser_request.path << std::endl;
    }
}