#include "../../include/TrateRequest.hpp"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h>

/******************************** CGI POST ********************************/

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
        dup2(pipefd_stdout[1], STDOUT_FILENO);
        close(pipefd_stdout[1]);
        dup2(pipefd_stdin[0], STDIN_FILENO);
        close(pipefd_stdin[0]);

        // Executar no diretório correto para acesso a arquivos relativos
        std::string script_dir = script_path.substr(0, script_path.find_last_of("/"));
        if (!script_dir.empty() && chdir(script_dir.c_str()) == -1)
        {
            std::cerr << "[x] Erro ao mudar para diretório: " << script_dir << std::endl;
            exit(1);
        }

        // Criando envp (array de strings com variáveis de ambiente)
        std::string env_query = "QUERY_STRING=";
        std::string env_method = "REQUEST_METHOD=POST";
        std::string env_script = "SCRIPT_FILENAME=" + script_path;
        std::string env_content = "CONTENT_TYPE=" + (parser_request.headers.count("Content-Type") ? parser_request.headers.at("Content-Type") : "");

        char* envp[] = {
            const_cast<char*>(env_query.c_str()),
            const_cast<char*>(env_method.c_str()),
            const_cast<char*>(env_script.c_str()),
            const_cast<char*>(env_content.c_str()),
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
        char buffer[4096];
        std::string output;
        ssize_t bytes_read;
        int status;

        close(pipefd_stdout[1]);
        close(pipefd_stdin[0]);
        
        // Verificar se é multipart/form-data e parsear campos
        std::string body_to_send = parser_request.body;
        if (parser_request.headers.count("Content-Type") && 
            parser_request.headers.at("Content-Type").find("multipart/form-data") != std::string::npos)
        {
            std::string content_type = parser_request.headers.at("Content-Type");
            body_to_send = postMultipart("", content_type, parser_request, "CGI");
        }
        
        // Escreve o corpo no pipe stdin
        // [ALERTA] - Mesma coisa do ifGET o write e o waitpid bloqueante, precisa registrar os pipes no socket
        // [VIOLAÇÃO ARQUITETURAL 42] Escrever/Ler pipes e aguardar o PID com waitpid bloqueante (sem WNOHANG)
        // fora do poll() paralisa o event loop do servidor. A lógica foi mantida para alterar 
        // minimamente o arquivo, mas a solução correta exige registrar os pipes stdin/stdout no SocketServer.
        if (!body_to_send.empty())
            write(pipefd_stdin[1], body_to_send.c_str(), body_to_send.length());
        close(pipefd_stdin[1]);
        
        // Le o resultado do script feito no processo filho
        // [ALERTA]
        while ((bytes_read = read(pipefd_stdout[0], buffer, sizeof(buffer))) > 0)
            output.append(buffer, bytes_read);
        close(pipefd_stdout[0]);

        // espera o processo filho acabar
        waitpid(pid, &status, 0);

        // Verificar se houve timeout (processo filho terminado por SIGALRM)
        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "[x] CGI POST timeout após 5 segundos" << std::endl;
            return;
        }

        // Verificar se o CGI terminou com sucesso
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "[x] CGI POST falhou com exit code: " << WEXITSTATUS(status) << std::endl;
            return;
        }

        // Verificar se o output está vazio
        if (output.empty())
        {
            sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
            std::cerr << "[x] CGI POST retornou output vazio" << std::endl;
            return;
        }

        _response = parser_request.version + " 200 OK\r\n" + output;
        std::cout << "[+] CGI POST executado com sucesso" << std::endl;
    }
}

/******************************** DADOS JSON ********************************/

std::string TrateRequest::postMultipart(const std::string& dir_uploads, const std::string& content_type, const ParserRequest& parser_request, const std::string& type)
{
    std::string body = parser_request.body;
    std::string parsed_body = "";
    std::string json_body = "{";
    std::string boundary;
    size_t start_boundary = 0;
    size_t end_boundary = 0;
    bool first = true;

    // Extrair boundary
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos)
        return "";
    boundary = "--" + content_type.substr(boundary_pos + 9);
    
    // Loop até terminar todos boundary do body
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
        
        if (type == "FORM")
        {
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
                
                std::string upload_path = dir_uploads + filename;
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
                std::string url_path = dir_uploads.substr(3);
                json_body += "\"photoUrl\":\"" + url_path + filename + "\"";
            }
            else
            {
                if (!first)
                    json_body += ",";
                first = false;
                json_body += "\"" + name + "\":\"" + content + "\"";
            }
        }
        else
        {
            // Formatar para CGI: name=value ou name=filename:content
            if (!filename.empty())
                parsed_body += name + "=file:" + filename + ":" + content + "\n";
            else
                parsed_body += name + "=" + content + "\n";
        }
        
        start_boundary = end_boundary;
    }
    
    if (type == "FORM")
    {
        json_body += "}";
        return json_body;
    }
    return parsed_body;
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
        std::string dir_json = "www/data/";
        std::string dir_uploads = "www/uploads/";
        std::string json_body;
        std::string content_type;

        if (parser_request.headers.count("Content-Type"))
            content_type = parser_request.headers.at("Content-Type");
        else
            content_type = "";
        
        if (content_type.find("multipart/form-data") != std::string::npos)
            json_body = postMultipart(dir_uploads, content_type, parser_request, "FORM");
        else
            json_body = postFormData(parser_request);
                
        std::string filename = dir_json + "curriculum.json";
        std::ofstream file(filename.c_str());
        if (file.is_open())
        {
            file << json_body;
            file.close();
            
            std::string response = parser_request.version + " 302 Found\r\nLocation: ";
            if (parser_request.headers.count("Referer"))
                response += parser_request.headers.at("Referer") + "\r\nConnection: keep-alive\r\n\r\n";
            else
                response += "/\r\nConnection: keep-alive\r\n\r\n";
            _response = response;
            
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
    // curl -X POST -F "bia=123" -F "wes=456" -F "claudio=789" -F "arquivo=@www/cgi-bin/test_file.txt" http://localhost:8080/cgi-bin/test_multipart.py
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
