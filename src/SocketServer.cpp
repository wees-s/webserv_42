#include "../include/SocketServer.hpp"
#include "../include/ParserRequest.hpp"
#include "../include/TrateRequest.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <csignal>

volatile sig_atomic_t g_running = 1;

void signal_handler(int signum)
{
    (void)signum;
    g_running = 0;
}

SocketServer::~SocketServer()
{
    if (server_fd >= 0)
        close(server_fd);
    if (client_fd >= 0)
        close(client_fd);
}

SocketServer::SocketServer() : server_fd(-1), client_fd(-1) {
    signal(SIGINT, signal_handler);
}

void SocketServer::setup()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Erro ao criar o socket." << std::endl;
        return;
    }

    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address)); // Zera a estrutura (hábito seguro)
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;      // Escuta em todas as interfaces de rede locais
    address.sin_port = htons(8080);            // htons converte a porta para o formato da rede

    // Bind: Amarrar o socket à porta 8080
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Erro no bind. A porta 8080 já está em uso?" << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }

    // Listen: Colocar o kernel para ouvir conexões
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Erro no listen." << std::endl;
        close(server_fd);
        server_fd = -1;
        return;
    }
    std::cout << "[+] Servidor TCP rodando. Escutando na porta 8080..." << std::endl;
}

void SocketServer::handleConnection()
{
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        if (g_running == 0)
            return;
        std::cerr << "Erro ao aceitar cliente." << std::endl;
        return;
    }
    std::cout << "[!] Um cliente acabou de conectar!" << std::endl;

    // Ler a request do socket
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));
    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        std::cerr << "Erro ao ler do socket." << std::endl;
        close(client_fd);
        client_fd = -1;
        return;
    }

    std::string req(buffer, bytes_read);
    ParserRequest parser_request(req);
    TrateRequest trate_request(parser_request, client_fd);

    close(client_fd);
    client_fd = -1;
    std::cout << "\n[-] Conexao encerrada." << std::endl;
}

void SocketServer::run()
{
    setup();
    if (server_fd < 0)
        return;

// Futuramente substituir o loop atual while (g_running) 
// que chama accept() sequencialmente por um loop que usa epoll
    while (g_running)
        handleConnection();

    close(server_fd);
    server_fd = -1;
    std::cout << "\n[!] Servidor encerrado." << std::endl;
}

//http://localhost:8080/paginateste
