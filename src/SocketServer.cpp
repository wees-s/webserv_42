#include "../include/SocketServer.hpp"
#include "../include/ParserRequest.hpp"
#include "../include/TrateRequest.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <csignal>
#include <sstream>

volatile sig_atomic_t g_running = 1;

void signal_handler(int signum) {
    (void)signum;
    g_running = 0;
}

SocketServer::SocketServer() : server_fd(-1) {
    signal(SIGINT, signal_handler);
}

SocketServer::~SocketServer() {
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
        if (_poll_fds[i].fd >= 0)
            close(_poll_fds[i].fd);
    }
}

void SocketServer::setup() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Erro ao criar o socket." << std::endl;
        return;
    }
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Erro no bind. A porta 8080 já está em uso?" << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Erro no listen." << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }
    struct pollfd server_poll;
    server_poll.fd = server_fd;
    server_poll.events = POLLIN;
    server_poll.revents = 0;
    _poll_fds.push_back(server_poll);
    std::cout << "[+] Servidor NONBLOCK rodando. Escutando na porta 8080..." << std::endl;
}

void SocketServer::acceptNewConnection() {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) return;
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    struct pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    client_pollfd.revents = 0;
    _poll_fds.push_back(client_pollfd);

    _client_buffers[client_fd] = "";
    _client_last_activity[client_fd] = time(NULL);
    std::cout << "[!] Cliente conectado! FD: " << client_fd << std::endl;
}

void SocketServer::closeConnection(size_t index) {
    int fd = _poll_fds[index].fd;
    close(fd);
    _client_buffers.erase(fd);
    _client_responses.erase(fd);
    _client_last_activity.erase(fd);
    _poll_fds.erase(_poll_fds.begin() + index);
    std::cout << "[-] Conexão fechada. FD: " << fd << std::endl;
}

void SocketServer::handleClientData(size_t index) {
    int fd = _poll_fds[index].fd;
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));

    int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    // Se recv retornar 0, cliente fechou a conexão. Se for < 0, erro.
    if (bytes_read <= 0) {
        closeConnection(index);
        return;
    }

    _client_last_activity[fd] = time(NULL); // Cliente está vivo, atualiza o tempo!
    _client_buffers[fd].append(buffer, bytes_read);

    // Integração temporária com a classe do user1. 
    // Se encontramos o fim do cabeçalho HTTP...
    if (_client_buffers[fd].find("\r\n\r\n") != std::string::npos) {
        std::cout << "[*] Request completo recebido do FD " << fd << ". Preparando resposta..." << std::endl;
        
        // Criamos o corpo do HTML separadamente
        std::string html_body = "<html><body style='background-color: #282a36; color: #50fa7b; font-family: sans-serif;'>"
                                "<h1>Ola! Webserv rodando com poll()!</h1>"
                                "<p>O multiplexador nao-bloqueante esta despachando bytes com sucesso.</p>"
                                "</body></html>";
        
        // Montamos o pacote HTTP calculando o tamanho exato do corpo dinamicamente
        std::ostringstream response_stream;
        response_stream << "HTTP/1.1 200 OK\r\n"
                        << "Content-Type: text/html\r\n"
                        << "Connection: close\r\n"    // Avisa o navegador que vamos fechar a porta
                        << "Content-Length: " << html_body.length() << "\r\n"
                        << "\r\n"                     // Linha em branco obrigatória separando cabeçalho do corpo
                        << html_body;
        
        _client_responses[fd] = response_stream.str();
        _poll_fds[index].events = POLLOUT;
    }
}

void SocketServer::handleClientWrite(size_t index) {
    int fd = _poll_fds[index].fd;
    std::string& response = _client_responses[fd];

    // Tenta enviar o que está na fila. O kernel decide quantos bytes realmente vão.
    ssize_t bytes_sent = send(fd, response.c_str(), response.size(), 0);

    if (bytes_sent < 0) {
        std::cerr << "Erro ao enviar dados para o FD " << fd << std::endl;
        closeConnection(index);
        return;
    }

    // Se enviou algo, apaga o trecho enviado do início da nossa string
    if (bytes_sent > 0) {
        response.erase(0, bytes_sent);
        _client_last_activity[fd] = time(NULL);
    }

    // Se a string esvaziou, enviamos tudo! A resposta foi completa.
    if (response.empty()) {
        std::cout << "[+] Resposta enviada com sucesso para o FD " << fd << std::endl;
        // Na HTTP/1.0 e requisições simples HTTP/1.1, fechamos a conexão após responder
        closeConnection(index);
    }
}

void SocketServer::run() {
    setup();
    if (server_fd < 0) return;

    // O EVENT LOOP CENTRAL DA APLICAÇÃO
    while (g_running) {
        // [MODIFICADO]: poll espera no máximo 2 segundos, mesmo sem atividade
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), 2000); 
        
        if (poll_count < 0 && g_running) {
            std::cerr << "Erro no poll()." << std::endl;
            break;
        }

        // Primeiro processamos quem enviou ou quer receber dados
        if (poll_count > 0) {
            for (int i = _poll_fds.size() - 1; i >= 0; --i) {
                if (_poll_fds[i].revents & POLLIN) {
                    if (_poll_fds[i].fd == server_fd) {
                        acceptNewConnection();
                    } else {
                        handleClientData(i);
                    }
                }
                else if (_poll_fds[i].revents & POLLOUT) {
                    handleClientWrite(i);
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

    // O índice 0 é sempre o socket de listen (server_fd); clientes começam em 1
    for (int i = _poll_fds.size() - 1; i >= 1; --i) {
        int fd = _poll_fds[i].fd;
        
        // Difftime calcula a diferença em segundos entre dois tempos
        if (difftime(now, _client_last_activity[fd]) > TIMEOUT_SECONDS) {
            std::cout << "[TIMEOUT] Cliente fantasma detectado e derrubado. FD: " << fd << std::endl;
            closeConnection(i);
        }
    }
}
