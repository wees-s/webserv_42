#include "../../include/TrateRequest.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <dirent.h>
#include <fstream>
#include <signal.h>
#include <cstdlib>
#include <sys/wait.h>

// Redirecionamentos faltantes:
// 301 Permanent Redirect
// 302 Temporary Redirect
// Deve ser configurável por arquivo de configuração

/******************************** CGI GET ********************************/

static volatile sig_atomic_t g_cgi_timeout = 0;

static void cgi_timeout_handler(int sig)
{
    (void)sig;
    g_cgi_timeout = 1;
}

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

        // Configurar timeout de 5 segundos para o CGI
        signal(SIGALRM, cgi_timeout_handler);
        alarm(5);

        // Executar no diretório correto para acesso a arquivos relativos
        std::string script_dir = script_path.substr(0, script_path.find_last_of("/"));
        if (!script_dir.empty() && chdir(script_dir.c_str()) == -1)
        {
            std::cerr << "[x] Erro ao mudar para diretório: " << script_dir << std::endl;
            exit(1);
        }

        // Define variáveis de ambiente
        // Isso simula um ambiente CGI. O script pode ler essas variáveis.
        setenv("QUERY_STRING", query_string.c_str(), 1);
        setenv("REQUEST_METHOD", "GET", 1);
        setenv("SCRIPT_FILENAME", script_path.c_str(), 1);

        // Extrair apenas o nome do arquivo para execve (caminho relativo ao diretório atual)
        std::string script_name = script_path.substr(script_path.find_last_of("/") + 1);
        char* args[] = {const_cast<char*>(script_name.c_str()), NULL};

        execve(script_name.c_str(), args, environ);
        exit(1);
    }
    else
    {
        char buffer[4096];
        std::string output;
        ssize_t bytes_read;
        int status;

        close(pipefd[1]);
        
        // Le o resultado do script feito no processo filho
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0)
            output.append(buffer, bytes_read);
        close(pipefd[0]);

        // espera o processo filho acabar
        waitpid(pid, &status, 0);

        // Verificar se houve timeout (processo filho terminado por SIGALRM)
        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "[x] CGI GET timeout após 5 segundos" << std::endl;
            return;
        }

        // Verificar se o CGI terminou com sucesso
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "[x] CGI GET falhou com exit code: " << WEXITSTATUS(status) << std::endl;
            return;
        }

        // Verificar se o output está vazio
        if (output.empty())
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "[x] CGI GET retornou output vazio" << std::endl;
            return;
        }

        // Envia o output do script para o cliente (o script já inclui o header HTTP)
        std::string response = parser_request.version + " 200 OK\r\n" + output;
        write(_client_fd, response.c_str(), response.length());
        std::cout << "[+] CGI GET executado com sucesso" << std::endl;
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
    
    std::stringstream ss;
    ss << "www/temp_listing_" << getpid() << ".html";
    std::string temp_file = ss.str();
    
    std::ofstream file(temp_file.c_str());
    if (file.is_open())
    {
        file << listing_html;
        file.close();
        sendPage(temp_file, parser_request.version + " 200 OK");
        std::remove(temp_file.c_str());
    }
    else
        std::cerr << "[x] Erro ao criar arquivo temporário para listagem" << std::endl;
}

/******************************** IF GET ********************************/

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
    
    // API endpoint para carregar dados do currículo
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
        sendPage(filename, parser_request.version + " 200 OK\r\n");
    }
    // API endpoint para retornar PID do usuário
    else if (parser_request.path == "/api/pid")
    {
        std::stringstream ss;
        ss << "{\"pid\":" << getpid() << "}";
        std::string response = parser_request.version + " 200 OK\r\nContent-Type: application/json\r\n\r\n" + ss.str();
        write(_client_fd, response.c_str(), response.length());
    }
    // API endpoint para executar scripts CGI
    // curl -X GET http://localhost:8080/cgi-bin/cgiGet.py
    else if (parser_request.path.find("/cgi-bin/") == 0)
    {
        std::string query_string;
        if (parser_request.headers.count("Query"))
            query_string = parser_request.headers.at("Query");
        else
            query_string = "";
        executeCGIGet(file_path, query_string, parser_request);
    }
    // Cliente pede um diretório em vez de um arquivo
    // curl http://localhost:8080/www
    else if (DIR* dir = opendir(file_path.c_str()))
    {
        // Tentar servir index.html
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
