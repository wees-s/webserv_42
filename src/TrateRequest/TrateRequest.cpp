#include "../../include/TrateRequest.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <sys/wait.h>
#include <signal.h>

volatile sig_atomic_t g_cgi_timeout = 0;

void cgi_timeout_handler(int sig)
{
    (void)sig;
    g_cgi_timeout = 1;
}

TrateRequest::~TrateRequest() {}

TrateRequest::TrateRequest(const ParserRequest& parser_request, int client_fd) : _client_fd(client_fd)
{
    // HTTP/1.1: Múltiplos sites no mesmo IP → Host header obrigatório
    if (parser_request.version == "HTTP/1.1" && !parser_request.headers.count("Host"))
    {
        sendPage("www/error/400.html", parser_request.version + " 400 Bad Request");
        std::cerr << "[x] Host header ausente (HTTP/1.1 requer)" << std::endl;
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
        std::cerr << "[x] Método não permitido: " << parser_request.method << std::endl;
    }
}

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
        std::cerr << "[x] Erro ao abrir arquivo: " << file_path << std::endl;
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

void TrateRequest::executeCGI(const std::string& script_path, const std::string& query_string, const std::string& method, const ParserRequest& parser_request)
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

        // Define variáveis de ambiente
        // Isso simula um ambiente CGI. O script pode ler essas variáveis.
        setenv("QUERY_STRING", query_string.c_str(), 1);
        setenv("REQUEST_METHOD", method.c_str(), 1);
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
        
        // Configurar timeout de 5 segundos para o CGI
        signal(SIGALRM, cgi_timeout_handler);
        g_cgi_timeout = 0;
        alarm(5);
        
        // Le o resultado do script feito no processo filho
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0)
            output.append(buffer, bytes_read);
        close(pipefd[0]);

        // Desativar alarme
        alarm(0);
        signal(SIGALRM, SIG_DFL);

        // espera o processo filho acabar
        waitpid(pid, &status, 0);

        // Verificar se houve timeout
        if (g_cgi_timeout)
        {
            std::cerr << "[x] CGI timeout after 5 seconds" << std::endl;
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            return;
        }

        // Verificar se o CGI terminou com sucesso
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            std::cerr << "[x] CGI failed with exit code: " << WEXITSTATUS(status) << std::endl;
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            return;
        }

        // Verificar se o output está vazio
        if (output.empty())
        {
            std::cerr << "[x] CGI returned empty output" << std::endl;
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            return;
        }

        // Envia o output do script para o cliente (o script já inclui o header HTTP)
        std::string response = parser_request.version + " 200 OK\r\n" + output;
        write(_client_fd, response.c_str(), response.length());
    }
}
