#include "../../include/TrateRequest.hpp"
#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

/******************************** CGI POST ********************************/

static volatile sig_atomic_t g_cgi_timeout = 0;

static void cgi_timeout_handler(int sig)
{
    (void)sig;
    g_cgi_timeout = 1;
}

void TrateRequest::executeCGIPost(const std::string& script_path, const ParserRequest& parser_request)
{
    int pipefd_stdout[2];
    int pipefd_stdin[2];
    pid_t pid;

    if (pipe(pipefd_stdout) == -1)
    {
        sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
        std::cerr << "[x] Erro ao criar pipe stdout" << std::endl;
        return;
    }

    if (pipe(pipefd_stdin) == -1)
    {
        close(pipefd_stdout[0]);
        close(pipefd_stdout[1]);
        sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
        std::cerr << "[x] Erro ao criar pipe stdin" << std::endl;
        return;
    }

    pid = fork();
    if (pid == -1)
    {
        close(pipefd_stdout[0]);
        close(pipefd_stdout[1]);
        close(pipefd_stdin[0]);
        close(pipefd_stdin[1]);
        sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
        std::cerr << "[x] Erro ao fazer fork" << std::endl;
        return;
    }

    if (pid == 0)
    {
        close(pipefd_stdout[0]);
        close(pipefd_stdin[1]);
        // Escreve no pipe stdout invés da saída padrão
        dup2(pipefd_stdout[1], STDOUT_FILENO);
        close(pipefd_stdout[1]);
        // Lê do pipe stdin invés da entrada padrão
        dup2(pipefd_stdin[0], STDIN_FILENO);
        close(pipefd_stdin[0]);

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
        setenv("QUERY_STRING", "", 1);
        setenv("REQUEST_METHOD", "POST", 1);
        setenv("SCRIPT_FILENAME", script_path.c_str(), 1);
        setenv("CONTENT_TYPE", parser_request.headers.count("Content-Type") ? parser_request.headers.at("Content-Type").c_str() : "", 1);

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

        close(pipefd_stdout[1]);
        close(pipefd_stdin[0]);
        
        // Verificar se é multipart/form-data e desagrupar
        std::string body_to_send = parser_request.body;
        if (parser_request.headers.count("Content-Type") && 
            parser_request.headers.at("Content-Type").find("multipart/form-data") != std::string::npos)
        {
            // Desagrupar multipart/form-data
            // Extrair boundary
            std::string content_type = parser_request.headers.at("Content-Type");
            size_t boundary_pos = content_type.find("boundary=");

            if (boundary_pos != std::string::npos)
            {
                std::string boundary = content_type.substr(boundary_pos + 9);
                // Remover aspas se presentes
                if (boundary[0] == '"')
                    boundary = boundary.substr(1, boundary.length() - 2);
                
                // Desagrupar: remover headers multipart e manter apenas o corpo
                std::string clean_body;
                size_t pos = 0;
                std::string delimiter = "--" + boundary;
                
                while (pos < body_to_send.length())
                {
                    size_t delimiter_pos = body_to_send.find(delimiter, pos);
                    if (delimiter_pos == std::string::npos)
                        break;
                    
                    size_t header_end = body_to_send.find("\r\n\r\n", delimiter_pos);
                    if (header_end == std::string::npos)
                        break;
                    
                    size_t body_start = header_end + 4;
                    size_t next_boundary = body_to_send.find(delimiter, body_start);
                    if (next_boundary == std::string::npos)
                        next_boundary = body_to_send.length();
                    
                    // Remover \r\n antes do próximo boundary
                    std::string part = body_to_send.substr(body_start, next_boundary - body_start);
                    if (part.length() >= 2 && part.substr(part.length() - 2) == "\r\n")
                        part = part.substr(0, part.length() - 2);
                    
                    clean_body += part;
                    pos = next_boundary;
                }

                body_to_send = clean_body;
            }
        }
        
        // Escrever o corpo no stdin do CGI
        if (!body_to_send.empty())
            write(pipefd_stdin[1], body_to_send.c_str(), body_to_send.length());
        close(pipefd_stdin[1]);
        
        // Le o resultado do script feito no processo filho
        while ((bytes_read = read(pipefd_stdout[0], buffer, sizeof(buffer))) > 0)
            output.append(buffer, bytes_read);
        close(pipefd_stdout[0]);

        // espera o processo filho acabar
        waitpid(pid, &status, 0);

        // Verificar se houve timeout (processo filho terminado por SIGALRM)
        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "[x] CGI timeout após 5 segundos" << std::endl;
            return;
        }

        // Verificar se o CGI terminou com sucesso
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "[x] CGI falhou com exit code: " << WEXITSTATUS(status) << std::endl;
            return;
        }

        // Verificar se o output está vazio
        if (output.empty())
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "[x] CGI retornou output vazio" << std::endl;
            return;
        }

        // Envia o output do script para o cliente (o script já inclui o header HTTP)
        std::string response = parser_request.version + " 200 OK\r\n" + output;
        write(_client_fd, response.c_str(), response.length());
        std::cout << "[+] CGI POST executado com sucesso" << std::endl;
    }
}

/******************************** DADOS JSON ********************************/

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

    // Extrair boundary
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos)
    {
        sendPage("www/error/400.html", parser_request.version + " 400 Bad Request");
        return "";
    }
    boundary = "--" + content_type.substr(boundary_pos + 9);
    
    // loop até terminar todos boundary do body
    // preenche json_body com 1 campo de cada vez
    while ((start_boundary = body.find(boundary, start_boundary)) != std::string::npos)
    {
        start_boundary += boundary.length();
        if (body.substr(start_boundary, 2) == "\r\n")
            start_boundary += 2;
        
        // Extrair header
        size_t header_end = body.find("\r\n\r\n", start_boundary);
        if (header_end == std::string::npos)
            break;
        std::string header = body.substr(start_boundary, header_end - start_boundary);
        
        // Extrair nome do campo que está no header
        std::string name;
        size_t name_pos = header.find("name=\"");
        if (name_pos != std::string::npos)
        {
            name_pos += 6;
            size_t name_end = header.find("\"", name_pos);
            if (name_end != std::string::npos)
                name = header.substr(name_pos, name_end - name_pos);
        }

        // Extrair nome do arquivo que está no header (se existir arquivo)
        std::string filename;
        size_t file_pos = header.find("filename=\"");
        if (file_pos != std::string::npos)
        {
            file_pos += 10;
            size_t file_end = header.find("\"", file_pos);
            if (file_end != std::string::npos)
                filename = header.substr(file_pos, file_end - file_pos);
        }
        
        // pular \r\n\r\n para posição do próximo boundary
        start_boundary = header_end + 4;
        
        // Achar onde começa o próximo boundary
        end_boundary = body.find(boundary, start_boundary);
        if (end_boundary == std::string::npos)
            break;
        
        // extrair conteúdo preenchido pelo usuário no campo x
        std::string content = body.substr(start_boundary, end_boundary - start_boundary - 2);
        
        if (!filename.empty())
        {
            // Remover qualquer caminho e deixar só o nome final do arquivo
            size_t last_slash = filename.find_last_of("/\\");
            if (last_slash != std::string::npos)
                filename = filename.substr(last_slash + 1);
            
            // Trocar espaços por _ no nome do arquivo
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

            // Remove "www" prefix
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
        
        // trocar '+' por espaço
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

/******************************** IF POST ********************************/

void TrateRequest::ifPost(const ParserRequest& parser_request)
{
    // Validação de body size
    // Tratamento temporário, o tamanho deve ser configurado via config
    if (parser_request.body.size() > 1024 * 1024) // 1MB
    {
        sendPage("www/error/413.html", parser_request.version + " 413 Payload Too Large");
        std::cerr << "[x] Body com tamanho maior que 1MB" << std::endl;
        return;
    }

    // POST /api/curriculum - salva dados do currículo em arquivo JSON
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
                response += parser_request.headers.at("Referer") + "\r\n\r\n";
            else
                response += "/\r\n\r\n";
            
            write(_client_fd, response.c_str(), response.length());
            std::cout << "[+] Dados do currículo salvos em " << filename << std::endl;
        }
        else
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "[x] Erro ao abrir arquivo para escrita: " << filename << std::endl;
        }
    }
    // POST /cgi-bin/ - endpoint para executar scripts CGI POST
    // curl -X POST http://localhost:8080/cgi-bin/test_infinite_loop.py
    else if (parser_request.path.find("/cgi-bin/") == 0)
    {
        std::string file_path = "www" + parser_request.path;
        executeCGIPost(file_path, parser_request);
    }
    else
    {
        sendPage("www/error/404.html", parser_request.version + " 404 Not Found");
        std::cerr << "[x] Arquivo não encontrado: " << parser_request.path << std::endl;
    }
}
