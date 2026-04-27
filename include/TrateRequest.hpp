#ifndef TRATEREQUEST_HPP
#define TRATEREQUEST_HPP

#include "ParserRequest.hpp"

class TrateRequest
{
	private:
		int _client_fd;

	public:
		~TrateRequest();
		TrateRequest(const ParserRequest& parser_request, int client_fd);

        void ifGet(const ParserRequest& parser_request);
        void ifPost(const ParserRequest& parser_request);
        void ifDelete(const ParserRequest& parser_request);
};

#endif