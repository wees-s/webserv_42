#include "ParserConf.hpp"
#include <cstdlib>

ParserConf::~ParserConf() {}

ParserConf::ParserConf()
{
    // Valores temporários do default.conf até o parser estar pronto
    _ports.push_back(8080);
    _ports.push_back(8081);
    _ports.push_back(8082);
    _ports.push_back(8083);
    
    _serverName = "localhost";
    _root = "www/";
    _index = "index.html";
    _clientMaxBodySize = 1024 * 1024; // 1MB
    
    // CGI extensions
    _cgiExtensions.push_back(".py");
    _cgiExtensions.push_back(".php");
    
    // Upload directory
    _uploadDir = "www/uploads/";
    
    // Error pages
    _errorPages[400] = "www/error/400.html";
    _errorPages[403] = "www/error/403.html";
    _errorPages[404] = "www/error/404.html";
    _errorPages[405] = "www/error/405.html";
    _errorPages[413] = "www/error/413.html";
    _errorPages[500] = "www/error/500.html";
}

bool ParserConf::isCgiExtension(const std::string& extension) const
{
    for (size_t i = 0; i < _cgiExtensions.size(); ++i)
    {
        if (_cgiExtensions[i] == extension)
            return true;
    }
    return false;
}