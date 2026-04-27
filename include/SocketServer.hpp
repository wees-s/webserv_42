#ifndef SOCKETSERVER_HPP
#define SOCKETSERVER_HPP

class SocketServer
{
	private:
		int server_fd;
		int client_fd;

		void setup();
		void handleConnection();

    public:
        ~SocketServer();
        SocketServer();
        void run();
};

#endif
