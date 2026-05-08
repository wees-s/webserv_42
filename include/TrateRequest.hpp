#ifndef TRATEREQUEST_HPP
#define TRATEREQUEST_HPP

#include "ParserRequest.hpp"
#include <string>
#include <dirent.h>

class TrateRequest
{
	private:
		std::string _response;
		
		void sendPage(const std::string& file_path, const std::string& status_header);
		std::string getContentType(const std::string& file_path);
		std::string generateDirectoryListing(const std::string& path, DIR* dir);
		void sendDirectoryListing(const std::string& path, DIR* dir, const ParserRequest& parser_request);
		std::string postFormData(const ParserRequest& parser_request);
		std::string postMultipartFormData(const std::string& user_dir, const std::string& content_type, const ParserRequest& parser_request);
		void executeCGI(const std::string& script_path, const std::string& query_string, const ParserRequest& parser_request);

		void ifGet(const ParserRequest& parser_request);
		void ifPost(const ParserRequest& parser_request);
		void ifDelete(const ParserRequest& parser_request);

	public:
		~TrateRequest();
		TrateRequest(const ParserRequest& parser_request);
		std::string getResponse() const;

};

#endif