#ifndef TRATEREQUEST_HPP
#define TRATEREQUEST_HPP

#include "ParserRequest.hpp"
#include <string>

class TrateRequest
{
	private:
		int _client_fd;
		void sendPage(const std::string& file_path, const std::string& status_header);

	public:
		~TrateRequest();
		TrateRequest(const ParserRequest& parser_request, int client_fd);

        void ifGet(const ParserRequest& parser_request);
        void ifPost(const ParserRequest& parser_request);
        void ifDelete(const ParserRequest& parser_request);
};

#endif