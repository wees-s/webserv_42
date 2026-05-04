#ifndef TRATEREQUEST_HPP
#define TRATEREQUEST_HPP

#include "ParserRequest.hpp"
#include <string>
#include <dirent.h>

class TrateRequest
{
	private:
		int _client_fd;
		
		void sendPage(const std::string& file_path, const std::string& status_header);
		std::string getContentType(const std::string& file_path);
		std::string generateDirectoryListing(const std::string& path, DIR* dir);
		void sendDirectoryListing(const std::string& path, DIR* dir);

		void ifGet(const ParserRequest& parser_request);
		void ifPost(const ParserRequest& parser_request);
		void ifDelete(const ParserRequest& parser_request);

	public:
		~TrateRequest();
		TrateRequest(const ParserRequest& parser_request, int client_fd);
};

#endif