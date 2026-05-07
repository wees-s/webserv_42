#ifndef SOCKETSERVER_HPP
#define SOCKETSERVER_HPP

#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include <ctime>

class SocketServer
{
	private:
		std::vector<int> _server_fds;
		std::vector<int> _ports;
		std::vector<struct pollfd> _poll_fds;
		std::map<int, std::string> _client_buffers;
		std::map<int, std::string> _client_responses;
		std::map<int, time_t> _client_last_activity; // Última atividade do cliente
		void setup();
		bool isServerSocket(int fd);
		void acceptNewConnection(int server_fd);
		void handleClientData(size_t index);
		void handleClientWrite(size_t index);
		void closeConnection(size_t index);
		void checkTimeouts();

    public:
        ~SocketServer();
        SocketServer(const std::vector<int>& ports);
        void run();
};

#endif
