#include "../../include/ParserConf.hpp"
#include <cstdlib>
#include <iostream>

ParserConf::~ParserConf() {}

ParserConf::ParserConf(std::string filename)
{
    TokenConf tokens;
    _tokens = tokens.tokenizeConfig(filename);
    _filename = filename;
    parseConfig();
}

void ParserConf::parseConfig()
{
    std::vector<std::string>::iterator it = _tokens.begin();
    
    while (it != _tokens.end())
    {
        if (*it == "server")
            parseServerBlock(it);
        else
            it++;
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
            parseListen(it, server);
        else if (*it == "server_name")
            parseServerName(it, server);
        else if (*it == "root")
            parseRoot(it, server);
        else if (*it == "index")
            parseIndex(it, server);
        else if (*it == "client_max_body_size")
            parseClientMaxBodySize(it, server);
        else if (*it == "location")
            parseLocation(it, server);
        else if (*it == "error_page")
            parseErrorPage(it, server);
        else
            it++;
    }
    
    if (it != _tokens.end() && *it == "}")
        it++;
    
    _servers.push_back(server);
}

void ParserConf::parseListen(std::vector<std::string>::iterator& it, ServerConfig& server)
{
    it++; // Skip "listen"
    
    while (it != _tokens.end() && *it != ";")
    {
        int port = std::atoi((*it).c_str());
        if (port > 0)
            server.ports.push_back(port);
        it++;
    }
    
    if (it != _tokens.end() && *it == ";")
        it++;
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
        it++;
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
        it++;
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
        it++;
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
        it++;
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
            parseAllow(it, loc);
        else if (*it == "cgi_extensions")
            parseCgiExtensions(it, loc);
        else if (*it == "upload_dir")
            parseUploadDir(it, loc);
        else if (*it == "return")
            parseReturn(it, loc);
        else if (*it == "root")
        {
            it++; // Skip "root"
            if (it != _tokens.end() && *it != ";")
            {
                loc.root = *it;
                it++;
            }
            if (it != _tokens.end() && *it == ";")
                it++;
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
                it++;
        }
        else
            it++;
    }
    
    if (it != _tokens.end() && *it == "}")
        it++;
    
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
        it++;
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
        it++;
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
        it++;
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
        it++;
    
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
        it++;
}

const ParserConf::LocationConfig* ParserConf::findLocation(const std::string& path, const ServerConfig& server) const
{
    std::string current = path;
    while (true)
    {
        std::map<std::string, LocationConfig>::const_iterator it = server.locations.find(current);
        if (it != server.locations.end())
            return &(it->second);
        
        std::map<std::string, LocationConfig>::const_iterator slash_it = server.locations.find(current + "/");
        if (slash_it != server.locations.end())
            return &(slash_it->second);
        
        size_t last_slash = current.find_last_of('/');
        if (last_slash == 0 || last_slash == std::string::npos)
            break;
        current = current.substr(0, last_slash);
    }
    
    std::map<std::string, LocationConfig>::const_iterator it = server.locations.find("/");
    if (it != server.locations.end())
        return &(it->second);
    
    return NULL;
}

const ParserConf::ServerConfig& ParserConf::getServerConfig(int port, const std::string& host) const
{
    // Limpar host de porta se existir (ex: localhost:8080 -> localhost)
    std::string clean_host = host;
    size_t colon = host.find(':');
    if (colon != std::string::npos)
        clean_host = host.substr(0, colon);

    // 1. Tenta achar porta + server_name (Host)
    for (size_t i = 0; i < _servers.size(); i++)
    {
        for (size_t j = 0; j < _servers[i].ports.size(); j++)
        {
            if (_servers[i].ports[j] == port && _servers[i].serverName == clean_host)
                return _servers[i];
        }
    }
    
    // 2. Fallback: Apenas porta
    for (size_t i = 0; i < _servers.size(); i++)
    {
        for (size_t j = 0; j < _servers[i].ports.size(); j++)
        {
            if (_servers[i].ports[j] == port)
                return _servers[i];
        }
    }
    
    return _servers[0];
}

std::vector<int> ParserConf::getPorts() const
{
    std::vector<int> all_ports;
    for (size_t i = 0; i < _servers.size(); i++)
    {
        for (size_t j = 0; j < _servers[i].ports.size(); j++)
        {
            bool already_exists = false;
            for (size_t k = 0; k < all_ports.size(); k++)
                if (all_ports[k] == _servers[i].ports[j]) already_exists = true;
            if (!already_exists)
                all_ports.push_back(_servers[i].ports[j]);
        }
    }
    return all_ports;
}

std::string ParserConf::getRoot(const ServerConfig& server, const std::string& path) const
{
    if (!path.empty())
    {
        const LocationConfig* loc = findLocation(path, server);
        if (loc && !loc->root.empty())
            return loc->root;
    }
    return server.root;
}

std::string ParserConf::getIndex(const ServerConfig& server, const std::string& path) const
{
    if (!path.empty())
    {
        const LocationConfig* loc = findLocation(path, server);
        if (loc && !loc->index.empty())
            return loc->index;
    }
    return server.index;
}

long ParserConf::getClientMaxBodySize(const ServerConfig& server) const
{
    return server.clientMaxBodySize;
}

std::map<int, std::string> ParserConf::getErrorPages(const ServerConfig& server) const {
    return server.errorPages;
}

std::vector<std::string> ParserConf::getCgiExtensions(const ServerConfig& server, const std::string& path) const
{
    const LocationConfig* loc = findLocation(path, server);
    if (loc)
        return loc->cgiExtensions;
    return std::vector<std::string>();
}

std::string ParserConf::getUploadDir(const ServerConfig& server, const std::string& path) const
{
    const LocationConfig* loc = findLocation(path, server);
    if (loc)
        return loc->uploadDir;
    return "";
}

std::vector<std::string> ParserConf::getMethods(const ServerConfig& server, const std::string& path) const
{
    const LocationConfig* loc = findLocation(path, server);
    if (loc)
        return loc->methods;
    return std::vector<std::string>();
}

bool ParserConf::hasRedirect(const ServerConfig& server, const std::string& path) const
{
    const LocationConfig* loc = findLocation(path, server);
    if (loc)
        return loc->hasRedirect;
    return false;
}

int ParserConf::getRedirectCode(const ServerConfig& server, const std::string& path) const
{
    const LocationConfig* loc = findLocation(path, server);
    if (loc && loc->hasRedirect)
        return loc->redirect.code;
    return 0;
}

std::string ParserConf::getRedirectPath(const ServerConfig& server, const std::string& path) const
{
    const LocationConfig* loc = findLocation(path, server);
    if (loc && loc->hasRedirect)
        return loc->redirect.new_path;
    return "";
}

bool ParserConf::isCgiExtension(const ServerConfig& server, const std::string& extension, const std::string& path) const
{
    std::vector<std::string> extensions = getCgiExtensions(server, path);
    for (size_t i = 0; i < extensions.size(); i++)
    {
        if (extensions[i] == extension)
            return true;
    }
    return false;
}

bool ParserConf::isMethodAllowed(const ServerConfig& server, const std::string& path, const std::string& method) const
{
    std::vector<std::string> methods = getMethods(server, path);
    for (size_t i = 0; i < methods.size(); i++)
    {
        if (methods[i] == method)
            return true;
    }
    return false;
}
