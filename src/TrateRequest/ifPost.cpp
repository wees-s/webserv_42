#include "../../include/TrateRequest.hpp"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h>
#include <sys/stat.h>

/******************************** CGI POST ********************************/

void TrateRequest::executeCGIPost(const std::string& script_path, const ParserRequest& parser_request, const ParserConf& config)
{
    (void)config;
    int pipefd_stdout[2];
    int pipefd_stdin[2];
    pid_t pid;

    if (pipe(pipefd_stdout) == -1)
    {
        sendErrorPage(500, "500 Internal Server Error", parser_request);
        std::cerr << "[x] Error creating pipe stdout" << std::endl;
        return;
    }

    if (pipe(pipefd_stdin) == -1)
    {
        close(pipefd_stdout[0]);
        close(pipefd_stdout[1]);
        sendErrorPage(500, "500 Internal Server Error", parser_request);
        std::cerr << "[x] Error creating pipe stdin" << std::endl;
        return;
    }

    pid = fork();
    if (pid == -1)
    {
        close(pipefd_stdout[0]);
        close(pipefd_stdout[1]);
        close(pipefd_stdin[0]);
        close(pipefd_stdin[1]);
        sendErrorPage(500, "500 Internal Server Error", parser_request);
        std::cerr << "[x] Error forking process" << std::endl;
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
            std::cerr << "[x] Error changing directory: " << script_dir << std::endl;
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
        if (!body_to_send.empty())
            write(pipefd_stdin[1], body_to_send.c_str(), body_to_send.length());
        close(pipefd_stdin[1]);
        
        // Não lê. Não espera. Só registra e sai.
        _cgi_fd = pipefd_stdout[0];
        _cgi_pid = pid;
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

                // Remove o prefixo "www"
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
            body = body.substr(pos_key + 1);
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

void TrateRequest::ifPost(const ParserRequest& parser_request, const ParserConf& config)
{
    long clientMaxBodySize = config.getClientMaxBodySize(_server);
    std::string root = config.getRoot(_server, parser_request.path);
    
    // Validação de body size
    if (parser_request.body.size() > static_cast<size_t>(clientMaxBodySize))
    {
        sendErrorPage(413, "413 Payload Too Large", parser_request);
        std::cerr << "[x] Body size exceeds configured limit" << std::endl;
        return;
    }

    // API endpoint para salvar dados do currículo em arquivo JSON
    if (parser_request.path == "/api/curriculum")
    {
        std::string dir_uploads = config.getUploadDir(_server, parser_request.path);
        if (dir_uploads.empty())
            dir_uploads = root + "uploads/";
        std::string dir_json = root + "data/";

		DIR* dir = opendir(dir_uploads.c_str());
        if (dir == NULL)
        {
            sendErrorPage(500, "500 Internal Server Error", parser_request);
            std::cerr << "[CGI/Post Error] Configured upload directory does not exist: " << dir_uploads << std::endl;
            return;
        }
        closedir(dir);

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
            
            std::cout << "[+] Curriculum data saved to " << filename << std::endl;
        }
        else
        {
            sendErrorPage(500, "500 Internal Server Error", parser_request);
            std::cerr << "[x] Error opening file for writing: " << filename << std::endl;
        }
    }
    // endpoint para executar scripts CGI POST
    else if (parser_request.path.find("/cgi-bin/") == 0)
    {
		if (parser_request.body.size() > 60000)
        {
            std::cerr << "[CGI/Post Error] Body size (" << parser_request.body.size() << " bytes) is too large for synchronous CGI pipe restriction." << std::endl;
            sendErrorPage(502, "502 Bad Gateway", parser_request);
            return;
        }

        std::string file_path = root + parser_request.path.substr(1);
        size_t dot_pos = file_path.find_last_of('.');
        if (dot_pos != std::string::npos)
        {
            std::string extension = file_path.substr(dot_pos);
            if (!config.isCgiExtension(_server, extension, parser_request.path))
            {
                sendErrorPage(403, "403 Forbidden", parser_request);
                std::cerr << "[x] CGI extension not allowed: " << file_path << std::endl;
                return;
            }
        }
        executeCGIPost(file_path, parser_request, config);
    }
    else
    {
        sendErrorPage(404, "404 Not Found", parser_request);
        std::cerr << "[x] File not found: " << parser_request.path << std::endl;
    }
}
