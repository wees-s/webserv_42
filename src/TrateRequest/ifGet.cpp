#include "../../include/TrateRequest.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <dirent.h>
#include <cstdlib>
#include <sys/wait.h>

// Redirecionamentos faltantes:
// 301 Permanent Redirect
// 302 Temporary Redirect
// Deve ser configurável por arquivo de configuração

/******************************** CGI GET ********************************/

void TrateRequest::executeCGIGet(const std::string& script_path, const std::string& query_string, const ParserRequest& parser_request)
{
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1)
    {
        sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
        std::cerr << "[x] Erro ao criar pipe" << std::endl;
        return;
    }

    pid = fork();
    if (pid == -1)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
        std::cerr << "[x] Erro ao fazer fork" << std::endl;
        return;
    }

    if (pid == 0)
    {
        close(pipefd[0]);
        // Escreve no pipe invés da saída padrão
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        // Executar no diretório correto para acesso a arquivos relativos
        std::string script_dir = script_path.substr(0, script_path.find_last_of("/"));
        if (!script_dir.empty() && chdir(script_dir.c_str()) == -1)
        {
            std::cerr << "[x] Erro ao mudar para diretório: " << script_dir << std::endl;
            exit(1);
        }

        // Criando envp
		std::string env_query = "QUERY_STRING=" + query_string;
		std::string env_method = "REQUEST_METHOD=GET";
		std::string env_script = "SCRIPT_FILENAME=" + script_path;

		char* envp[] = {
			const_cast<char*>(env_query.c_str()),
			const_cast<char*>(env_method.c_str()),
			const_cast<char*>(env_script.c_str()),
			NULL
		};

        // Extrair apenas o nome do arquivo para execve (caminho relativo ao diretório atual)
        std::string script_name = script_path.substr(script_path.find_last_of("/") + 1);
        char* args[] = {const_cast<char*>(script_name.c_str()), NULL};

        execve(script_name.c_str(), args, envp);
        exit(1);
    }
    else
    {
        close(pipefd[1]);
        
        // Não lê. Não espera. Só registra e sai.
        _cgi_fd = pipefd[0];
        _cgi_pid = pid;
    }
}

/******************************** DIRECTORY LISTING ********************************/

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

	std::stringstream ss_size;
	ss_size << listing_html.length();

	_response = parser_request.version + " 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
		+ ss_size.str() + "\r\nConnection: keep-alive\r\n\r\n" + listing_html;
}

/******************************** IF GET ********************************/

void TrateRequest::ifGet(const ParserRequest& parser_request, const ParserConf& config)
{
    std::string file_path = "www";
    if (parser_request.path == "/")
        file_path += "/" + _index;
    else
    {
        if (parser_request.path[0] != '/')
            file_path += "/";
        file_path += parser_request.path;
    }
    
    // API endpoint para carregar dados do currículo
    if (parser_request.path == "/api/curriculum")
    {
        std::string filename = "www/data/curriculum.json";
        int file_fd = open(filename.c_str(), O_RDONLY);
        if (file_fd < 0)
            filename = "www/data/default_curriculum.json";
        else
            close(file_fd);
        sendPage(filename, parser_request.version + " 200 OK\r\n");
    }
    // API endpoint para executar scripts CGI
    // curl -X GET http://localhost:8080/cgi-bin/cgiGet.py
    else if (parser_request.path.find("/cgi-bin/") == 0)
    {
        size_t dot_pos = file_path.find_last_of('.');
        if (dot_pos != std::string::npos)
        {
            std::string extension = file_path.substr(dot_pos);
            if (!config.isCgiExtension(extension))
            {
                sendPage("www/error/403.html", parser_request.version + " 403 Forbidden\r\n");
                std::cerr << "[x] Extensão CGI não permitida: " << file_path << std::endl;
                return;
            }
        }
        std::string query_string;
        if (parser_request.headers.count("Query"))
            query_string = parser_request.headers.at("Query");
        else
            query_string = "";
        executeCGIGet(file_path, query_string, parser_request);
    }
    // Cliente pede um diretório em vez de um arquivo
    // curl http://localhost:8080/error
    else if (DIR* dir = opendir(file_path.c_str()))
    {
        std::string index_path = file_path + "/" + _index;
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
    // GET normal para arquivos estáticos
    else
    {
        int file_fd = open(file_path.c_str(), O_RDONLY);
        if (file_fd < 0)
        {
            sendPage("www/error/404.html", parser_request.version + " 404 Not Found");
            std::cerr << "[x] Arquivo não encontrado: " << file_path << std::endl;
        }
        else
        {
            close(file_fd);
            sendPage(file_path.c_str(), parser_request.version + " 200 OK");
        }
    }
}
