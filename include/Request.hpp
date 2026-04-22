#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

class Request 
{
	public:
		std::string method;
		std::string path;
		std::string version;
		std::string body;

		std::map<std::string, std::string> headers;
};

#endif