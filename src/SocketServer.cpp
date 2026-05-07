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

SocketServer::SocketServer(const std::vector<int>& ports) : _ports(ports) {
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
    
    // Se recv retornar 0, cliente fechou a conexão. Se for < 0, erro.
    if (bytes_read <= 0) {
        closeConnection(index);
        return;
    }

    _client_last_activity[fd] = time(NULL); // Cliente está vivo, atualiza o tempo!
    _client_buffers[fd].append(buffer, bytes_read);

    /// Integração temporária com a classe do user1. 
    // Se encontramos o fim do cabeçalho HTTP...
    if (_client_buffers[fd].find("\r\n\r\n") != std::string::npos) {
        std::cout << "[*] Complete request received from FD " << fd << ". Preparing response..." << std::endl;
        
        // Criamos o corpo do HTML separadamente
        std::string html_body = "<html><body style='background-color: #282a36; color: #50fa7b; font-family: sans-serif;'>"
                                "<h1>Hello! Webserv running with poll()!</h1>"
                                "<p>The non-blocking multiplexer is dispatching bytes successfully.</p>"
                                "</body></html>";
        
        // Montamos o pacote HTTP calculando o tamanho exato do corpo dinamicamente
        std::ostringstream response_stream;
        response_stream << "HTTP/1.1 200 OK\r\n"
                        << "Content-Type: text/html\r\n"
                        << "Connection: keep-alive\r\n"    // [ALTERADO]: Avisa o navegador para manter a conexão viva
                        << "Content-Length: " << html_body.length() << "\r\n"
                        << "\r\n"                     // Linha em branco obrigatória separando cabeçalho do corpo
                        << html_body;
        
        _client_responses[fd] = response_stream.str();
        
        // [NOVO]: Limpa o buffer de leitura para receber o próximo request neste mesmo FD
        _client_buffers[fd].clear(); 
        
        _poll_fds[index].events = POLLOUT;
    }
}

void SocketServer::handleClientWrite(size_t index) {
    int fd = _poll_fds[index].fd;
    std::string& response = _client_responses[fd];

    // Tenta enviar o que está na fila. O kernel decide quantos bytes realmente vão.
    ssize_t bytes_sent = send(fd, response.c_str(), response.size(), 0);

    if (bytes_sent < 0) {
        std::cerr << "Error sending data to FD " << fd << std::endl;
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
        std::cout << "[+] Response sent successfully to FD " << fd << " (Keeping connection alive)" << std::endl;
        
        // Em vez de fechar, voltamos a escutar (POLLIN) neste mesmo FD
        _poll_fds[index].events = POLLIN;
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
                if (_poll_fds[i].revents & POLLIN) {
                    if (isServerSocket(_poll_fds[i].fd)) {
                        acceptNewConnection(_poll_fds[i].fd);
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
            closeConnection(i);
        }
    }
}
