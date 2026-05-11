#ifndef TRATEREQUEST_HPP
#define TRATEREQUEST_HPP

#include "ParserRequest.hpp"
#include <string>
#include <dirent.h>
#include <sys/types.h>

class TrateRequest
{
	private:
		std::string _response;

		int    _cgi_fd;   // -1 se não tem CGI pendente
		pid_t  _cgi_pid;
		
		std::string getContentType(const std::string& file_path);
		void sendPage(const std::string& file_path, const std::string& status_header);
		
		// ifGet helpers
		void executeCGIGet(const std::string& script_path, const std::string& query_string, const ParserRequest& parser_request);
		std::string generateDirectoryListing(const std::string& path, DIR* dir);
		void sendDirectoryListing(const std::string& path, DIR* dir, const ParserRequest& parser_request);
		
		// ifPost helpers
		void executeCGIPost(const std::string& script_path, const ParserRequest& parser_request);
		std::string postMultipart(const std::string& user_dir, const std::string& content_type, const ParserRequest& parser_request, const std::string& type);
		std::string postFormData(const ParserRequest& parser_request);

		void ifGet(const ParserRequest& parser_request);
		void ifPost(const ParserRequest& parser_request);
		void ifDelete(const ParserRequest& parser_request);

	public:
		~TrateRequest();
		TrateRequest(const ParserRequest& parser_request);
		const std::string& getResponse() const;
		bool hasCGI() const;
		int  getCGIFd() const;
		pid_t getCGIPid() const;
};

#endif