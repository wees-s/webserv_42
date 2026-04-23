#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

int main() 
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Erro ao criar o socket." << std::endl;
        return 1;
    }

    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address)); // Zera a estrutura (hábito seguro)
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;      // Escuta em todas as interfaces de rede locais
    address.sin_port = htons(8080);            // htons converte a porta para o formato da rede

    // 3. Bind: Amarrar o socket à porta 8080
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Erro no bind. A porta 8080 já está em uso?" << std::endl;
        return 1;
    }

    // 4. Listen: Colocar o kernel para ouvir conexões
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Erro no listen." << std::endl;
        return 1;
    }
    std::cout << "[+] Servidor TCP rodando. Escutando na porta 8080..." << std::endl;

    // 5. Accept: Fica bloqueado aqui até alguém bater na porta
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        std::cerr << "Erro ao aceitar cliente." << std::endl;
        return 1;
    }
    std::cout << "[!] Um cliente acabou de conectar!" << std::endl;

    // 6. Responder com um mini cabeçalho HTTP e uma mensagem
    const char* resposta = "HTTP/1.1 200 OK\r\n\r\nSalve Webserv! A infra ta viva!\n";
    write(client_fd, resposta, std::strlen(resposta));

    // 7. Fechar as portas
    close(client_fd);
    close(server_fd);
    std::cout << "[-] Conexao encerrada." << std::endl;

    return 0;
}