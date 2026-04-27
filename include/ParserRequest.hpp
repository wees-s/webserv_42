#ifndef PARSERREQUEST_HPP
#define PARSERREQUEST_HPP

#include <string>
#include <map>
#include <sstream>

class ParserRequest
{
	public:
		std::string method;
		std::string path;
		std::string version;
		std::string body;
		std::map<std::string, std::string> headers;

		~ParserRequest();
		ParserRequest(const std::string& req);
};

#endif