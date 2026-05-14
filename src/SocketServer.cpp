#include "../include/SocketServer.hpp"
#include "../include/ParserRequest.hpp"
#include "../include/TrateRequest.hpp"
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <csignal>
#include <sstream>
#include <cstdlib>
#include <fstream>

volatile sig_atomic_t g_running = 1;

void signal_handler(int signum) {
    (void)signum;
    g_running = 0;
}

SocketServer::SocketServer(const ParserConf& config) : _config(config) {
    _ports = config.getPorts();
    signal(SIGINT, signal_handler);
}

SocketServer::~SocketServer() {
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
        close(_poll_fds[i].fd);
    }
    _poll_fds.clear();
    _server_fds.clear();
}

void SocketServer::setup() {
    for (size_t i = 0; i < _ports.size(); ++i) {
        int port = _ports[i];
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            std::cerr << "Error creating socket for port " << port << std::endl;
            continue;
        }

        // Permite reusar a porta imediatamente após reiniciar o servidor
        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Torna o socket do servidor não-bloqueante
        fcntl(fd, F_SETFL, O_NONBLOCK);

        struct sockaddr_in address;
        std::memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Error in bind on port " << port << std::endl;
            close(fd);
            continue;
        }

        if (listen(fd, 1024) < 0) {
            std::cerr << "Error in listen on port " << port << std::endl;
            close(fd);
            continue;
        }

        // Regista este FD como um dos nossos servidores
        _server_fds.push_back(fd);

        // Adiciona ao poll() para vigiar
        struct pollfd server_poll_fd;
        server_poll_fd.fd = fd;
        server_poll_fd.events = POLLIN;
        server_poll_fd.revents = 0;
        _poll_fds.push_back(server_poll_fd);

        std::cout << "[+] Server listening on port " << port << " (FD: " << fd << ")" << std::endl;
    }
}

bool SocketServer::isServerSocket(int fd) {
    for (size_t i = 0; i < _server_fds.size(); ++i) {
        if (_server_fds[i] == fd) {
            return true;
        }
    }
    return false;
}

void SocketServer::acceptNewConnection(int server_fd) {
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_address, &client_len);

    if (client_fd < 0) {
        return; // Pode ser EAGAIN/EWOULDBLOCK devido ao O_NONBLOCK
    }

    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _poll_fds.push_back(pfd);

    _client_buffers[client_fd] = "";
    _client_last_activity[client_fd] = time(NULL);

    std::cout << "[!] New client connected on FD: " << client_fd 
              << " (coming from Server FD: " << server_fd << ")" << std::endl;
}

void SocketServer::closeConnection(size_t index) {
    int fd = _poll_fds[index].fd;
    close(fd);
    _client_buffers.erase(fd);
    _client_responses.erase(fd);
    _client_last_activity.erase(fd);
    _poll_fds.erase(_poll_fds.begin() + index);
    std::cout << "[-] Connection closed. FD: " << fd << std::endl;
}

void SocketServer::handleClientData(size_t index) {
    int fd = _poll_fds[index].fd;
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));

    int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    // [FIX] Separado em dois ifs para tratar 0 e negativo individualmente.
    // A régua exige que ambos sejam verificados — checar só <= 0 pode passar, mas checar separado é mais claro.
    if (bytes_read < 0) {
        std::cerr << "[-] recv error on FD " << fd << std::endl;
        closeConnection(index);
        return;
    }
    if (bytes_read == 0) {
        closeConnection(index);
        return;
    }

    _client_last_activity[fd] = time(NULL);
    _client_buffers[fd].append(buffer, bytes_read);

    // Procura o fim do cabeçalho HTTP
    size_t header_end = _client_buffers[fd].find("\r\n\r\n");
    
    if (header_end != std::string::npos) {
        
        // 1. Descobrir o Content-Length
        size_t content_length = 0;
        size_t cl_pos = _client_buffers[fd].find("Content-Length: ");
        
        if (cl_pos != std::string::npos && cl_pos < header_end) {
            size_t value_start = cl_pos + 16;
            size_t value_end = _client_buffers[fd].find("\r", value_start);
            std::string cl_str = _client_buffers[fd].substr(value_start, value_end - value_start);
            content_length = std::atoi(cl_str.c_str());
        }

        // 2. Calcula o tamanho total esperado
        size_t expected_total_size = header_end + 4 + content_length;
        // 3. Se já temos todos os bytes...
        if (_client_buffers[fd].size() >= expected_total_size) {
            std::cout << "[*] Request completed. Repassando para a camada de Aplication..." << std::endl;
            
            // Extrai o pacote HTTP perfeito
            std::string raw_request = _client_buffers[fd].substr(0, expected_total_size);
            
            // --- A PONTE DE INTEGRAÇÃO ---
            ParserRequest parsed_req(raw_request);
            TrateRequest handler(parsed_req, _config);

            if (handler.hasCGI())
            {
                // CGI em andamento — registrar o pipe no poll
                int pipe_fd = handler.getCGIFd();
                pid_t pid   = handler.getCGIPid();

                struct pollfd pfd;
                pfd.fd     = pipe_fd;
                pfd.events = POLLIN;
                pfd.revents = 0;
                _poll_fds.push_back(pfd);

                _cgi_pipe_to_client[pipe_fd] = fd;   // para saber a quem responder
                _cgi_pipe_to_pid[pipe_fd]    = pid;  // para reap sem bloquear
                _cgi_buffers[pipe_fd]        = "";
                // NÃO muda para POLLOUT ainda — vai mudar quando o pipe fechar
            }
            else
            {
                // Fluxo normal (GET estático, POST, DELETE)
                _client_responses[fd] = handler.getResponse();
                _poll_fds[index].events = POLLOUT;
                
                // Limpa o buffer após processar a requisição (para keep-alive)
                _client_buffers[fd].erase(0, expected_total_size);
            }
        } else {
            // Ainda faltam bytes do corpo (POST grande). Continua escutando.
        }
    }
}

void SocketServer::handleClientWrite(size_t index) {
    int fd = _poll_fds[index].fd;
    std::string& response = _client_responses[fd];

    // Tenta enviar o que está na fila. O kernel decide quantos bytes realmente vão.
    ssize_t bytes_sent = send(fd, response.c_str(), response.size(), 0);

    // [FIX] Separado em dois ifs para tratar negativo e 0 individualmente.
    // A régua exige que ambos sejam verificados — send() retornando 0 indica conexão fechada.
    if (bytes_sent < 0) {
        std::cerr << "[-] send error on FD " << fd << std::endl;
        closeConnection(index);
        return;
    }
    if (bytes_sent == 0) {
        closeConnection(index);
        return;
    }

    // Se enviou algo, apaga o trecho enviado do início da nossa string
    response.erase(0, bytes_sent);
    _client_last_activity[fd] = time(NULL);

    // Se a string esvaziou, enviamos tudo! A resposta foi completa.
    if (response.empty()) {
        // Em vez de fechar, voltamos a escutar (POLLIN) neste mesmo FD
        _poll_fds[index].events = POLLIN;
    }
}

// novo método: handleCGIRead(size_t index)
void SocketServer::handleCGIRead(size_t index)
{
    int pipe_fd   = _poll_fds[index].fd;
    int client_fd = _cgi_pipe_to_client[pipe_fd];
    pid_t pid     = _cgi_pipe_to_pid[pipe_fd];

    char buffer[4096];
    ssize_t bytes = read(pipe_fd, buffer, sizeof(buffer));

    if (bytes > 0)
    {
        _cgi_buffers[pipe_fd].append(buffer, bytes);
        return; // ainda tem mais dados
    }

    // bytes == 0 → pipe fechou → CGI terminou
    close(pipe_fd);
    _poll_fds.erase(_poll_fds.begin() + index);

    int status;
    waitpid(pid, &status, WNOHANG); // reap sem bloquear

    // Verifica o status de saída do CGI
    bool cgi_error = false;
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        // CGI retornou erro → 500
        cgi_error = true;
        std::cout << "[CGI] Script returned error code: " << WEXITSTATUS(status) << std::endl;
    } else if (WIFSIGNALED(status)) {
        // CGI foi terminado por sinal → 500
        cgi_error = true;
        std::cout << "[CGI] Script terminated by signal: " << WTERMSIG(status) << std::endl;
    } else if (_cgi_buffers[pipe_fd].empty()) {
        // CGI não imprimiu nada → 500
        cgi_error = true;
        std::cout << "[CGI] Script produced no output" << std::endl;
    }

    std::ostringstream header;
    
    if (cgi_error) {
        // Envia página de erro 500
        std::string error_page = "www/error/500.html";
        std::ifstream file(error_page.c_str());
        std::string content;
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            content = buffer.str();
            file.close();
        } else {
            content = "<html><body><h1>500 Internal Server Error</h1></body></html>";
        }
        
        header << "HTTP/1.1 500 Internal Server Error\r\n";
        header << "Content-Type: text/html\r\n";
        header << "Content-Length: " << content.size() << "\r\n";
        header << "\r\n";
        _client_responses[client_fd] = header.str() + content;
    } else {
        // Sucesso → envia saída do CGI
        std::string output = _cgi_buffers[pipe_fd];
        header << "HTTP/1.1 200 OK\r\n";
        header << "Content-Type: application/json\r\n";
        header << "Content-Length: " << output.size() << "\r\n";
        header << "\r\n";
        _client_responses[client_fd] = header.str() + output;
    }
    
    _cgi_buffers.erase(pipe_fd);
    _cgi_pipe_to_client.erase(pipe_fd);
    _cgi_pipe_to_pid.erase(pipe_fd);

    // Limpa o buffer do cliente após processar CGI (para keep-alive)
    _client_buffers[client_fd].clear();

    // acha o index do client_fd no _poll_fds e muda para POLLOUT
    for (size_t i = 0; i < _poll_fds.size(); ++i)
    {
        if (_poll_fds[i].fd == client_fd)
        {
            _poll_fds[i].events = POLLOUT;
            break;
        }
    }
}

void SocketServer::run() {
    setup();
    if (_server_fds.empty()) return;

    // O EVENT LOOP CENTRAL DA APLICAÇÃO
    while (g_running) {
        // [MODIFICADO]: poll espera no máximo 2 segundos, mesmo sem atividade
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), 2000); 
        
        if (poll_count < 0 && g_running) {
            std::cerr << "Error in poll()." << std::endl;
            break;
        }

        // Primeiro processamos quem enviou ou quer receber dados
        if (poll_count > 0) {
            for (int i = _poll_fds.size() - 1; i >= 0; --i) {
                if (_poll_fds[i].revents == 0)
                    continue;

                if (isServerSocket(_poll_fds[i].fd)) {
                    if (_poll_fds[i].revents & POLLIN)
                        acceptNewConnection(_poll_fds[i].fd);
                } else {
                    // [INTEGRAÇÃO CGI]: Verifica se o FD atual pertence a um pipe de CGI ativo.
                    // Se pertencer, chamamos handleCGIRead para coletar o output do script.
                    // Sem isso, o servidor tentaria ler o pipe como se fosse um cliente novo, quebrando a resposta.
                    if (_cgi_pipe_to_client.count(_poll_fds[i].fd)) {
                        if (_poll_fds[i].revents & POLLIN || _poll_fds[i].revents & POLLHUP)
                            handleCGIRead(i);
                    } else {
                        if (_poll_fds[i].revents & POLLIN)
                            handleClientData(i);
                        
                        // [FIX] Separado do POLLIN — sem else if.
                        // macOS pode retornar POLLIN e POLLOUT juntos no mesmo revents.
                        if (i < (int)_poll_fds.size() && _poll_fds[i].revents & POLLOUT)
                            handleClientWrite(i);
                    }
                }
            }
        }

        // [NOVO]: Independente de ter evento ou não, verifica se alguém expirou
        checkTimeouts(); 
    }
}   

void SocketServer::checkTimeouts() {
    time_t now = time(NULL);
    const double TIMEOUT_SECONDS = 30.0; // Tolerância de 30 segundos

    // Agora iteramos por TODOS os fds (>= 0)
    for (int i = _poll_fds.size() - 1; i >= 0; --i) {
        int fd = _poll_fds[i].fd;
        
        // [NOVO]: Se for um FD de servidor, o Ceifador ignora e passa para o próximo
        if (isServerSocket(fd)) {
            continue; 
        }
        
        // Se o cliente não tem registro ou estourou o tempo, é derrubado
        if (_client_last_activity.count(fd) && difftime(now, _client_last_activity[fd]) > TIMEOUT_SECONDS) {
            std::cout << "[TIMEOUT] Ghost client detected and shut down. FD: " << fd << std::endl;
            
            // Verifica se há CGI em execução para este cliente
            bool has_cgi = false;
            for (std::map<int, int>::const_iterator it = _cgi_pipe_to_client.begin(); it != _cgi_pipe_to_client.end(); ++it) {
                if (it->second == fd) {
                    has_cgi = true;
                    break;
                }
            }
            
            if (has_cgi) {
                // Envia 504 Gateway Timeout
                std::string error_page = "www/error/504.html";
                std::ifstream file(error_page.c_str());
                std::string content;
                if (file.is_open()) {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    content = buffer.str();
                    file.close();
                } else {
                    content = "<html><body><h1>504 Gateway Timeout</h1></body></html>";
                }
                
                std::ostringstream header;
                header << "HTTP/1.1 504 Gateway Timeout\r\n";
                header << "Content-Type: text/html\r\n";
                header << "Content-Length: " << content.size() << "\r\n";
                header << "\r\n";
                
                _client_responses[fd] = header.str() + content;
                
                // Muda para POLLOUT para enviar a resposta de erro
                for (size_t j = 0; j < _poll_fds.size(); ++j) {
                    if (_poll_fds[j].fd == fd) {
                        _poll_fds[j].events = POLLOUT;
                        break;
                    }
                }
                
                std::cout << "[TIMEOUT] Sent 504 Gateway Timeout to client FD: " << fd << std::endl;
            } else {
                closeConnection(i);
            }
        }
    }
}