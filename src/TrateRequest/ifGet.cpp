#include "../../include/TrateRequest.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <dirent.h>
#include <fstream>
#include <sys/wait.h>
#include <cstdlib>

extern char **environ; // Necessário para o execve do CGI em C++98

void TrateRequest::executeCGI(const std::string& script_path, const std::string& query_string, const ParserRequest& parser_request)
{
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1)
    {
        sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
        std::cerr << "Error creating pipe" << std::endl;
        return;
    }

    pid = fork();
    if (pid == -1)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
        std::cerr << "Error forking process" << std::endl;
        return;
    }

    if (pid == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        std::string script_dir = script_path.substr(0, script_path.find_last_of("/"));
        if (!script_dir.empty() && chdir(script_dir.c_str()) == -1)
        {
            std::cerr << "Error changing to directory: " << script_dir << std::endl;
            exit(1);
        }

        setenv("QUERY_STRING", query_string.c_str(), 1);
        setenv("REQUEST_METHOD", "GET", 1);
        setenv("SCRIPT_FILENAME", script_path.c_str(), 1);

        char* args[] = {const_cast<char*>(script_path.c_str()), NULL};
        execve(script_path.c_str(), args, environ);
        exit(1);
    }
    else
    {
        char buffer[4096];
        std::string output;
        ssize_t bytes_read;
        int status;

        close(pipefd[1]);
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0)
            output.append(buffer, bytes_read);
        close(pipefd[0]);

        waitpid(pid, &status, 0);

        // [CORREÇÃO]: Substituído write() por _response, e garantimos o Keep-Alive!
        std::stringstream ss_size;
        ss_size << output.length(); // O script Python deve retornar apenas o corpo, ou os headers também. Assumimos corpo.
        
        _response = parser_request.version + " 200 OK\r\nContent-Length: " + ss_size.str() + "\r\nConnection: keep-alive\r\n\r\n" + output;
    }
}

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

void TrateRequest::sendDirectoryListing(const std::string& path, DIR* dir, const ParserRequest& parser_request)
{
    std::string listing_html = generateDirectoryListing(path, dir);
    
    // Em vez de criar um arquivo temporário físico (o que é péssimo para performance), 
    // nós enviamos o HTML gerado diretamente na memória!
    std::stringstream ss_size;
    ss_size << listing_html.length();

    _response = parser_request.version + " 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + ss_size.str() + "\r\nConnection: keep-alive\r\n\r\n" + listing_html;
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
        sendPage(filename, parser_request.version + " 200 OK");
    }
    else if (parser_request.path == "/api/pid")
    {
        std::stringstream ss;
        ss << "{\"pid\":" << getpid() << "}";
        std::string json_body = ss.str();
        
        std::stringstream ss_size;
        ss_size << json_body.length();

        // [CORREÇÃO]: Substituído write() por _response
        _response = parser_request.version + " 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + ss_size.str() + "\r\nConnection: keep-alive\r\n\r\n" + json_body;
    }
    else if (parser_request.path.find("/cgi-bin/") == 0)
    {
        std::string query_string;
        if (parser_request.headers.count("Query"))
            query_string = parser_request.headers.at("Query");
        else
            query_string = "";
        executeCGI(file_path, query_string, parser_request);
    }
    else if (DIR* dir = opendir(file_path.c_str()))
    {
        std::string index_path = file_path + "/index.html";
        int index_fd = open(index_path.c_str(), O_RDONLY);
        if (index_fd >= 0)
        {
            close(index_fd);
            sendPage(index_path, parser_request.version + " 200 OK");
        }
        else
            sendDirectoryListing(parser_request.path, dir, parser_request);

        closedir(dir);
    }
    else
    {
        int file_fd = open(file_path.c_str(), O_RDONLY);
        if (file_fd < 0)
        {
            sendPage("www/error/404.html", parser_request.version + " 404 Not Found");
            std::cerr << "File not found: " << file_path << std::endl;
        }
        else
        {
            close(file_fd);
            sendPage(file_path.c_str(), parser_request.version + " 200 OK");
        }
    }
}