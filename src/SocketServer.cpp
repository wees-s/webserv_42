#include "../include/SocketServer.hpp"
#include "../include/ParserRequest.hpp"
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

void socket_server()
{
    signal(SIGINT, signal_handler);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
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
        return;
    }

    // Listen: Colocar o kernel para ouvir conexões
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Erro no listen." << std::endl;
        close(server_fd);
        return;
    }
    std::cout << "[+] Servidor TCP rodando. Escutando na porta 8080..." << std::endl;

    while (g_running)
    {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (g_running == 0)
                break;
            std::cerr << "Erro ao aceitar cliente." << std::endl;
            continue;
        }
        std::cout << "[!] Um cliente acabou de conectar!" << std::endl;

    // Ler a request do socket
        char buffer[4096];
        std::memset(buffer, 0, sizeof(buffer));
        int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            std::cerr << "Erro ao ler do socket." << std::endl;
            close(client_fd);
            continue;
        }

        ParserRequest request(request_str);

        // teste parser request
        std::string request_str(buffer, bytes_read);
        std::cout << "\n=== REQUEST RECEBIDA (RAW) ===" << std::endl;
        std::cout << request_str << std::endl;

        std::cout << "\n=== REQUEST PARSEADA ===" << std::endl;
        std::cout << "Method: " << request.method << std::endl;
        std::cout << "Path: " << request.path << std::endl;
        std::cout << "Version: " << request.version << std::endl;
        std::cout << "Body: [" << request.body << "]" << std::endl;
        std::cout << "Headers:" << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = request.headers.begin(); it != request.headers.end(); ++it)
            std::cout << "  " << it->first << ": " << it->second << std::endl;

        const char* resposta = "HTTP/1.1 200 OK\r\n\r\nSalve Webserv! A infra ta viva!\n";
        write(client_fd, resposta, std::strlen(resposta));
        //

    // Fechar as portas
        close(client_fd);
        std::cout << "\n[-] Conexao encerrada." << std::endl;
    }

    close(server_fd);
    std::cout << "\n[!] Servidor encerrado." << std::endl;
}

//http://localhost:8080/paginateste
