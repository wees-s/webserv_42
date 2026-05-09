#ifndef TRATEREQUEST_HPP
#define TRATEREQUEST_HPP

#include "ParserRequest.hpp"
#include <string>
#include <dirent.h>

class TrateRequest
{
	private:
		std::string _response;
		
		// [NOVO] Variáveis para exportar o estado do CGI para o poll()
		bool _is_cgi;
		int _cgi_fd;
		pid_t _cgi_pid;

		void sendPage(const std::string& file_path, const std::string& status_header);
		std::string getContentType(const std::string& file_path);

		std::string generateDirectoryListing(const std::string& path, DIR* dir);
		void sendDirectoryListing(const std::string& path, DIR* dir, const ParserRequest& parser_request);
		void executeCGIGet(const std::string& script_path, const std::string& query_string, const ParserRequest& parser_request);

		void executeCGIPost(const std::string& script_path, const ParserRequest& parser_request);
		std::string postMultipart(const std::string& user_dir, const std::string& content_type, const ParserRequest& parser_request, const std::string& type);
		std::string postFormData(const ParserRequest& parser_request);

		void ifGet(const ParserRequest& parser_request);
		void ifPost(const ParserRequest& parser_request);
		void ifDelete(const ParserRequest& parser_request);

	public:
		~TrateRequest();
		TrateRequest(const ParserRequest& parser_request);
		std::string getResponse() const;

		// [NOVO] Getters para o SocketServer
		bool isCgi() const;
		int getCgiFd() const;
		pid_t getCgiPid() const;
};

#endif



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

TrateRequest::TrateRequest(const ParserRequest& parser_request) : _is_cgi(false), _cgi_fd(-1), _cgi_pid(-1)
{
    if (parser_request.version == "HTTP/1.1" && !parser_request.headers.count("Host"))
    {
        sendPage("www/error/400.html", parser_request.version + " 400 Bad Request");
        std::cerr << "[x] Erro: Host header ausente em request HTTP/1.1" << std::endl;
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
        sendPage("www/error/405.html", "HTTP/1.1 405 Method Not Allowed");
        std::cerr << "Method not allowed: " << parser_request.method << std::endl;
    }
}

std::string TrateRequest::getResponse() const { return _response; }
bool TrateRequest::isCgi() const { return _is_cgi; }
int TrateRequest::getCgiFd() const { return _cgi_fd; }
pid_t TrateRequest::getCgiPid() const { return _cgi_pid; }

// ... resto do arquivo permanece igual


void TrateRequest::executeCGIGet(const std::string& script_path, const std::string& query_string, const ParserRequest& parser_request)
{
	int pipefd[2];
	pid_t pid;

	if (pipe(pipefd) == -1)
	{
		sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
		return;
	}

	pid = fork();
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		sendPage("www/error/500.html", parser_request.version + " 500 Internal Server Error");
		return;
	}

	if (pid == 0)
	{
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);

		std::string script_dir = script_path.substr(0, script_path.find_last_of("/"));
		if (!script_dir.empty() && chdir(script_dir.c_str()) == -1) exit(1);

		std::string env_query = "QUERY_STRING=" + query_string;
		std::string env_method = "REQUEST_METHOD=GET";
		std::string env_script = "SCRIPT_FILENAME=" + script_path;

		char* envp[] = {
			const_cast<char*>(env_query.c_str()),
			const_cast<char*>(env_method.c_str()),
			const_cast<char*>(env_script.c_str()),
			NULL
		};

		std::string script_name = script_path.substr(script_path.find_last_of("/") + 1);
		char* args[] = {const_cast<char*>(script_name.c_str()), NULL};

		execve(script_name.c_str(), args, envp);
		exit(1);
	}
	else
	{
		close(pipefd[1]);

		// [INTEGRAÇÃO SOCKET] Torna o lado de leitura do pipe NÃO-BLOQUEANTE
		fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

		// Exporta os dados para o SocketServer interceptar
		_is_cgi = true;
		_cgi_fd = pipefd[0];
		_cgi_pid = pid;

		// Não lê do pipe aqui. Não faz waitpid aqui. Não preenche _response.
		// Retorna instantaneamente para não parar o loop do servidor.
	}
}

// ... includes
#include <sys/wait.h> // Adicionar este include

class SocketServer
{
	private:
		std::vector<int> _server_fds;
		std::vector<int> _ports;
		std::vector<struct pollfd> _poll_fds;
		std::map<int, std::string> _client_buffers;
		std::map<int, std::string> _client_responses;
		std::map<int, time_t> _client_last_activity;

		// [NOVO] Mapas para rastrear o estado dos CGIs vinculados aos clientes
		std::map<int, int> _cgi_to_client;     // pipe_fd -> client_fd
		std::map<int, pid_t> _cgi_pids;        // pipe_fd -> cgi_pid
		std::map<int, std::string> _cgi_buffers; // pipe_fd -> dados acumulados

		void setup();
		bool isServerSocket(int fd);
		void acceptNewConnection(int server_fd);
		void handleClientData(size_t index);
		void handleClientWrite(size_t index);
		void handleCgiData(size_t index); // [NOVO] Lida com a leitura não-bloqueante do pipe
// ...

void SocketServer::handleClientData(size_t index) {
    int fd = _poll_fds[index].fd;
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));

    int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        closeConnection(index);
        return;
    }

    _client_last_activity[fd] = time(NULL);
    _client_buffers[fd].append(buffer, bytes_read);

    size_t header_end = _client_buffers[fd].find("\r\n\r\n");
    if (header_end != std::string::npos) {
        size_t content_length = 0;
        size_t cl_pos = _client_buffers[fd].find("Content-Length: ");
        if (cl_pos != std::string::npos && cl_pos < header_end) {
            size_t value_start = cl_pos + 16;
            size_t value_end = _client_buffers[fd].find("\r", value_start);
            std::string cl_str = _client_buffers[fd].substr(value_start, value_end - value_start);
            content_length = std::atoi(cl_str.c_str());
        }

        size_t expected_total_size = header_end + 4 + content_length;
        if (_client_buffers[fd].size() >= expected_total_size) {
            std::string raw_request = _client_buffers[fd].substr(0, expected_total_size);
            
            ParserRequest parsed_req(raw_request);
            TrateRequest handler(parsed_req);
            
            // [INTEGRAÇÃO SOCKET] Se for CGI, sequestra o controle
            if (handler.isCgi()) {
                int pipe_fd = handler.getCgiFd();
                _cgi_to_client[pipe_fd] = fd;
                _cgi_pids[pipe_fd] = handler.getCgiPid();
                _cgi_buffers[pipe_fd] = "";

                // Coloca o pipe no poll() para leitura
                struct pollfd pfd;
                pfd.fd = pipe_fd;
                pfd.events = POLLIN;
                pfd.revents = 0;
                _poll_fds.push_back(pfd);

                // Silencia temporariamente o cliente no poll() até o CGI terminar
                _poll_fds[index].events = 0;
                
                // Remove o request processado do buffer
                _client_buffers[fd].erase(0, expected_total_size);
            } else {
                _client_responses[fd] = handler.getResponse();
                _client_buffers[fd].erase(0, expected_total_size); 
                _poll_fds[index].events = POLLOUT;
            }
        }
    }
}

// [NOVO] Leitura assíncrona do CGI e fechamento via waitpid(WNOHANG)
void SocketServer::handleCgiData(size_t index) {
    int pipe_fd = _poll_fds[index].fd;
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));

    // read() assíncrono. Não vai travar porque o pipe é O_NONBLOCK
    int bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        _cgi_buffers[pipe_fd].append(buffer, bytes_read);
    } else if (bytes_read == 0) {
        // EOF. Processo CGI terminou de escrever.
        pid_t pid = _cgi_pids[pipe_fd];
        int status;
        
        // Reap do zumbi sem bloqueio
        waitpid(pid, &status, WNOHANG);

        int client_fd = _cgi_to_client[pipe_fd];
        
        // Formata a resposta HTTP
        std::string response;
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            response = "HTTP/1.1 200 OK\r\n" + _cgi_buffers[pipe_fd];
        } else {
            response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n";
        }

        _client_responses[client_fd] = response;
        
        // Acorda o socket do cliente para transmitir (POLLOUT)
        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].fd == client_fd) {
                _poll_fds[i].events = POLLOUT;
                break;
            }
        }

        // Limpeza dos mapas de estado do CGI
        close(pipe_fd);
        _cgi_to_client.erase(pipe_fd);
        _cgi_pids.erase(pipe_fd);
        _cgi_buffers.erase(pipe_fd);
        _poll_fds.erase(_poll_fds.begin() + index);
    }
}

void SocketServer::run() {
    setup();
    if (_server_fds.empty()) return;

    while (g_running) {
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), 2000); 
        
        if (poll_count < 0 && g_running) break;

        if (poll_count > 0) {
            for (int i = _poll_fds.size() - 1; i >= 0; --i) {
                if (_poll_fds[i].revents & POLLIN) {
                    if (isServerSocket(_poll_fds[i].fd)) {
                        acceptNewConnection(_poll_fds[i].fd);
                    } else if (_cgi_to_client.count(_poll_fds[i].fd)) {
                        // [NOVO] Roteia para o handler assíncrono do CGI
                        handleCgiData(i);
                    } else {
                        handleClientData(i);
                    }
                }
                else if (_poll_fds[i].revents & POLLOUT) {
                    handleClientWrite(i);
                }
            }
        }
        checkTimeouts(); 
    }
}