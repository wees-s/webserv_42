#include "../../include/ParserConf.hpp"
#include <cstdlib>
#include <iostream>

ParserConf::~ParserConf() {}

ParserConf::ParserConf(std::string filename)
{
    TokenConf tokens;
    _tokens = tokens.tokenizeConfig(filename);
    _filename = filename;
}

void ParserConf::parseConfig()
{
    std::vector<std::string>::iterator it = _tokens.begin();
    
    while (it != _tokens.end())
    {
        if (*it == "server")
        {
            parseServerBlock(it);
        }
        else
        {
            it++;
        }
    }
}

void ParserConf::parseServerBlock(std::vector<std::string>::iterator& it)
{
    it++; // Skip "server"
    
    if (it == _tokens.end() || *it != "{")
    {
        std::cerr << "Syntax Error: Expected '{' after server in " << _filename << std::endl;
        std::exit(EXIT_FAILURE);
    }
    it++; // Skip "{"
    
    ServerConfig server;
    server.clientMaxBodySize = 1024 * 1024; // Default 1MB
    
    while (it != _tokens.end() && *it != "}")
    {
        if (*it == "listen")
        {
            parseListen(it, server);
        }
        else if (*it == "server_name")
        {
            parseServerName(it, server);
        }
        else if (*it == "root")
        {
            parseRoot(it, server);
        }
        else if (*it == "index")
        {
            parseIndex(it, server);
        }
        else if (*it == "client_max_body_size")
        {
            parseClientMaxBodySize(it, server);
        }
        else if (*it == "location")
        {
            parseLocation(it, server);
        }
        else if (*it == "error_page")
        {
            parseErrorPage(it, server);
        }
        else
        {
            it++;
        }
    }
    
    if (it != _tokens.end() && *it == "}")
    {
        it++;
    }
    
    _servers.push_back(server);
}

void ParserConf::parseListen(std::vector<std::string>::iterator& it, ServerConfig& server)
{
    it++; // Skip "listen"
    
    while (it != _tokens.end() && *it != ";")
    {
        int port = std::atoi((*it).c_str());
        if (port > 0)
        {
            server.ports.push_back(port);
        }
        it++;
    }
    
    if (it != _tokens.end() && *it == ";")
    {
        it++;
    }
}

void ParserConf::parseServerName(std::vector<std::string>::iterator& it, ServerConfig& server)
{
    it++; // Skip "server_name"
    
    if (it != _tokens.end() && *it != ";")
    {
        server.serverName = *it;
        it++;
    }
    
    if (it != _tokens.end() && *it == ";")
    {
        it++;
    }
}

void ParserConf::parseRoot(std::vector<std::string>::iterator& it, ServerConfig& server)
{
    it++; // Skip "root"
    
    if (it != _tokens.end() && *it != ";")
    {
        server.root = *it;
        it++;
    }
    
    if (it != _tokens.end() && *it == ";")
    {
        it++;
    }
}

void ParserConf::parseIndex(std::vector<std::string>::iterator& it, ServerConfig& server)
{
    it++; // Skip "index"
    
    if (it != _tokens.end() && *it != ";")
    {
        server.index = *it;
        it++;
    }
    
    if (it != _tokens.end() && *it == ";")
    {
        it++;
    }
}

void ParserConf::parseClientMaxBodySize(std::vector<std::string>::iterator& it, ServerConfig& server)
{
    it++; // Skip "client_max_body_size"
    
    if (it != _tokens.end() && *it != ";")
    {
        server.clientMaxBodySize = parseSize(*it);
        it++;
    }
    
    if (it != _tokens.end() && *it == ";")
    {
        it++;
    }
}

long ParserConf::parseSize(const std::string& sizeStr)
{
    std::string numStr = sizeStr;
    long multiplier = 1;
    
    if (!sizeStr.empty())
    {
        char lastChar = sizeStr[sizeStr.length() - 1];
        if (lastChar == 'M' || lastChar == 'm')
        {
            multiplier = 1024 * 1024;
            numStr = sizeStr.substr(0, sizeStr.length() - 1);
        }
        else if (lastChar == 'K' || lastChar == 'k')
        {
            multiplier = 1024;
            numStr = sizeStr.substr(0, sizeStr.length() - 1);
        }
    }
    
    long value = std::atol(numStr.c_str());
    return value * multiplier;
}

void ParserConf::parseLocation(std::vector<std::string>::iterator& it, ServerConfig& server)
{
    it++; // Skip "location"
    
    if (it == _tokens.end())
    {
        std::cerr << "Syntax Error: Expected location path in " << _filename << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    std::string path = *it;
    it++;
    
    if (it == _tokens.end() || *it != "{")
    {
        std::cerr << "Syntax Error: Expected '{' after location path in " << _filename << std::endl;
        std::exit(EXIT_FAILURE);
    }
    it++; // Skip "{"
    
    LocationConfig loc;
    loc.hasRedirect = false;
    
    while (it != _tokens.end() && *it != "}")
    {
        if (*it == "allow")
        {
            parseAllow(it, loc);
        }
        else if (*it == "cgi_extensions")
        {
            parseCgiExtensions(it, loc);
        }
        else if (*it == "upload_dir")
        {
            parseUploadDir(it, loc);
        }
        else if (*it == "return")
        {
            parseReturn(it, loc);
        }
        else if (*it == "root")
        {
            it++; // Skip "root"
            if (it != _tokens.end() && *it != ";")
            {
                loc.root = *it;
                it++;
            }
            if (it != _tokens.end() && *it == ";")
            {
                it++;
            }
        }
        else if (*it == "index")
        {
            it++; // Skip "index"
            if (it != _tokens.end() && *it != ";")
            {
                loc.index = *it;
                it++;
            }
            if (it != _tokens.end() && *it == ";")
            {
                it++;
            }
        }
        else
        {
            it++;
        }
    }
    
    if (it != _tokens.end() && *it == "}")
    {
        it++;
    }
    
    server.locations[path] = loc;
}

void ParserConf::parseAllow(std::vector<std::string>::iterator& it, LocationConfig& loc)
{
    it++; // Skip "allow"
    
    while (it != _tokens.end() && *it != ";")
    {
        loc.methods.push_back(*it);
        it++;
    }
    
    if (it != _tokens.end() && *it == ";")
    {
        it++;
    }
}

void ParserConf::parseCgiExtensions(std::vector<std::string>::iterator& it, LocationConfig& loc)
{
    it++; // Skip "cgi_extensions"
    
    while (it != _tokens.end() && *it != ";")
    {
        loc.cgiExtensions.push_back(*it);
        it++;
    }
    
    if (it != _tokens.end() && *it == ";")
    {
        it++;
    }
}

void ParserConf::parseUploadDir(std::vector<std::string>::iterator& it, LocationConfig& loc)
{
    it++; // Skip "upload_dir"
    
    if (it != _tokens.end() && *it != ";")
    {
        loc.uploadDir = *it;
        it++;
    }
    
    if (it != _tokens.end() && *it == ";")
    {
        it++;
    }
}

void ParserConf::parseReturn(std::vector<std::string>::iterator& it, LocationConfig& loc)
{
    it++; // Skip "return"
    
    if (it != _tokens.end() && *it != ";")
    {
        loc.redirect.code = std::atoi((*it).c_str());
        it++;
    }
    
    if (it != _tokens.end() && *it != ";")
    {
        loc.redirect.new_path = *it;
        it++;
    }
    
    if (it != _tokens.end() && *it == ";")
    {
        it++;
    }
    
    loc.hasRedirect = true;
}

void ParserConf::parseErrorPage(std::vector<std::string>::iterator& it, ServerConfig& server)
{
    it++; // Skip "error_page"
    
    if (it != _tokens.end() && *it != ";")
    {
        int errorCode = std::atoi((*it).c_str());
        it++;
        
        if (it != _tokens.end() && *it != ";")
        {
            server.errorPages[errorCode] = *it;
            it++;
        }
    }
    
    if (it != _tokens.end() && *it == ";")
    {
        it++;
    }
}

const ParserConf::LocationConfig* ParserConf::findLocation(const std::string& path) const
{
    if (_servers.empty())
        return NULL;
    
    std::string current = path;
    while (true)
    {
        std::map<std::string, LocationConfig>::const_iterator it = _servers[0].locations.find(current);
        if (it != _servers[0].locations.end())
            return &(it->second);
        
        size_t last_slash = current.find_last_of('/');
        if (last_slash == 0 || last_slash == std::string::npos)
            break;
        current = current.substr(0, last_slash);
    }
    
    std::map<std::string, LocationConfig>::const_iterator it = _servers[0].locations.find("/");
    if (it != _servers[0].locations.end())
        return &(it->second);
    
    return NULL;
}

std::vector<int> ParserConf::getPorts() const
{
    if (_servers.empty())
        return std::vector<int>();
    return _servers[0].ports;
}

std::string ParserConf::getServerName() const
{
    if (_servers.empty())
        return "";
    return _servers[0].serverName;
}

std::string ParserConf::getRoot(const std::string& path) const
{
    if (_servers.empty())
        return "www/";
    
    if (!path.empty())
    {
        const LocationConfig* loc = findLocation(path);
        if (loc && !loc->root.empty())
            return loc->root;
    }
    
    return _servers[0].root;
}

std::string ParserConf::getIndex(const std::string& path) const
{
    if (_servers.empty())
        return "index.html";
    
    if (!path.empty())
    {
        const LocationConfig* loc = findLocation(path);
        if (loc && !loc->index.empty())
            return loc->index;
    }
    
    return _servers[0].index;
}

long ParserConf::getClientMaxBodySize() const
{
    if (_servers.empty())
        return 1024 * 1024;
    return _servers[0].clientMaxBodySize;
}

std::map<int, std::string> ParserConf::getErrorPages() const
{
    if (_servers.empty())
        return std::map<int, std::string>();
    return _servers[0].errorPages;
}

std::vector<std::string> ParserConf::getCgiExtensions(const std::string& path) const
{
    if (_servers.empty())
        return std::vector<std::string>();
    
    const LocationConfig* loc = findLocation(path);
    if (loc)
        return loc->cgiExtensions;
    
    return std::vector<std::string>();
}

std::string ParserConf::getUploadDir(const std::string& path) const
{
    if (_servers.empty())
        return "";
    
    const LocationConfig* loc = findLocation(path);
    if (loc)
        return loc->uploadDir;
    
    return "";
}

std::vector<std::string> ParserConf::getMethods(const std::string& path) const
{
    if (_servers.empty())
        return std::vector<std::string>();
    
    const LocationConfig* loc = findLocation(path);
    if (loc)
        return loc->methods;
    
    return std::vector<std::string>();
}

bool ParserConf::hasRedirect(const std::string& path) const
{
    if (_servers.empty())
        return false;
    
    const LocationConfig* loc = findLocation(path);
    if (loc)
        return loc->hasRedirect;
    
    return false;
}

int ParserConf::getRedirectCode(const std::string& path) const
{
    if (_servers.empty())
        return 0;
    
    const LocationConfig* loc = findLocation(path);
    if (loc && loc->hasRedirect)
        return loc->redirect.code;
    
    return 0;
}

std::string ParserConf::getRedirectPath(const std::string& path) const
{
    if (_servers.empty())
        return "";
    
    const LocationConfig* loc = findLocation(path);
    if (loc && loc->hasRedirect)
        return loc->redirect.new_path;
    
    return "";
}

bool ParserConf::isCgiExtension(const std::string& extension, const std::string& path) const
{
    std::vector<std::string> extensions = getCgiExtensions(path);
    for (size_t i = 0; i < extensions.size(); i++)
    {
        if (extensions[i] == extension)
            return true;
    }
    return false;
}

bool ParserConf::isMethodAllowed(const std::string& path, const std::string& method) const
{
    std::vector<std::string> methods = getMethods(path);
    for (size_t i = 0; i < methods.size(); i++)
    {
        if (methods[i] == method)
            return true;
    }
    return false;
}
