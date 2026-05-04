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
    std::cout << "[!] Cliente conectado! FD: " << client_fd << std::endl;
}

void SocketServer::closeConnection(size_t index) {
    int fd = _poll_fds[index].fd;
    close(fd);
    _client_buffers.erase(fd);
    _client_responses.erase(fd);
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

    _client_buffers[fd].append(buffer, bytes_read);

    // Integração temporária com a classe do user1. 
    // Só envia para o parser se encontrar o fim dos cabeçalhos.
    if (_client_buffers[fd].find("\r\n\r\n") != std::string::npos) {
        std::cout << "[*] Request completo recebido do FD " << fd << ". Preparando resposta..." << std::endl;
        
        // Simulação do que o TrateRequest do Wesley deveria me devolver:
        std::string http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 47\r\n\r\n<h1>Ola! Sou o Webserv rodando com poll()</h1>";
        // ParserRequest parser_request(_client_buffers[fd]);
        // TrateRequest trate_request(parser_request, fd);

       // 1. Colocamos a resposta na fila do cliente
       _client_responses[fd] = http_response;
        
       // 2. Avisamos ao poll() que não queremos mais LER (POLLIN). Agora queremos ESCREVER (POLLOUT).
       _poll_fds[index].events = POLLOUT;     


        // Pós-resposta, encerra a conexão do cliente
        // closeConnection(index);
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
        // poll espera por eventos. Timeout de 1000ms para permitir checagem de SIGINT
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), 1000);
        
        if (poll_count < 0 && g_running) {
            std::cerr << "Erro no poll()." << std::endl;
            break;
        }
        if (poll_count == 0) continue; // Timeout, volta pro loop

        // Varre o vetor de trás para frente para evitar problemas ao usar erase() no vetor
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
}   
