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
		int server_fd;
		std::vector<struct pollfd> _poll_fds;
		std::map<int, std::string> _client_buffers;
		std::map<int, std::string> _client_responses;
		std::map<int, time_t> _client_last_activity; // Última atividade do cliente
		void setup();
		void acceptNewConnection();
		void handleClientData(size_t index);
		void handleClientWrite(size_t index);
		void closeConnection(size_t index);
		void checkTimeouts();

    public:
        ~SocketServer();
        SocketServer();
        void run();
};

#endif
