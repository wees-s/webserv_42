#include "ParserConf.hpp"
#include <cstdlib>

ParserConf::~ParserConf() {}

ParserConf::ParserConf()
{
    // Cria um server temporário até o parser estar pronto
    ServerConfig server;
    
    server.ports.push_back(8080);
    server.ports.push_back(8081);
    server.ports.push_back(8082);
    server.ports.push_back(8083);
    
    server.serverName = "localhost";
    server.root = "www/";
    server.index = "index.html";
    server.clientMaxBodySize = 1024 * 1024; // 1MB
    
    // Error pages
    server.errorPages[400] = "www/error/400.html";
    server.errorPages[403] = "www/error/403.html";
    server.errorPages[404] = "www/error/404.html";
    server.errorPages[405] = "www/error/405.html";
    server.errorPages[413] = "www/error/413.html";
    server.errorPages[500] = "www/error/500.html";
    server.errorPages[504] = "www/error/504.html";
    
    // Location /
    LocationConfig loc_root;
    loc_root.methods.push_back("GET");
    loc_root.methods.push_back("POST");
    loc_root.methods.push_back("DELETE");
    loc_root.cgiExtensions.push_back(".py");
    loc_root.cgiExtensions.push_back(".php");
    loc_root.uploadDir = "";
    loc_root.root = "";
    loc_root.index = "";
    loc_root.hasRedirect = false;
    server.locations["/"] = loc_root;
    
    // Location /cgi-bin/
    LocationConfig loc_cgi;
    loc_cgi.methods.push_back("GET");
    loc_cgi.methods.push_back("POST");
    loc_cgi.cgiExtensions.push_back(".py");
    loc_cgi.cgiExtensions.push_back(".php");
    loc_cgi.uploadDir = "";
    loc_cgi.root = "";
    loc_cgi.index = "";
    loc_cgi.hasRedirect = false;
    server.locations["/cgi-bin/"] = loc_cgi;
    
    // Location /upload
    LocationConfig loc_upload;
    loc_upload.methods.push_back("POST");
    loc_upload.cgiExtensions.clear();
    loc_upload.uploadDir = "www/uploads/";
    loc_upload.root = "";
    loc_upload.index = "";
    loc_upload.hasRedirect = false;
    server.locations["/upload"] = loc_upload;
    
    // Location /old-path (redirect)
    LocationConfig loc_redirect1;
    loc_redirect1.methods.clear();
    loc_redirect1.cgiExtensions.clear();
    loc_redirect1.uploadDir = "";
    loc_redirect1.root = "";
    loc_redirect1.index = "";
    loc_redirect1.hasRedirect = true;
    loc_redirect1.redirect.code = 301;
    loc_redirect1.redirect.new_path = "/new-path";
    server.locations["/old-path"] = loc_redirect1;
    
    // Location /another-old (redirect)
    LocationConfig loc_redirect2;
    loc_redirect2.methods.clear();
    loc_redirect2.cgiExtensions.clear();
    loc_redirect2.uploadDir = "";
    loc_redirect2.root = "";
    loc_redirect2.index = "";
    loc_redirect2.hasRedirect = true;
    loc_redirect2.redirect.code = 302;
    loc_redirect2.redirect.new_path = "/another-new";
    server.locations["/another-old"] = loc_redirect2;
    
    _servers.push_back(server);
}

// Helper method para longest prefix match
const ParserConf::LocationConfig* ParserConf::findLocation(const std::string& path) const
{
    if (_servers.empty())
        return NULL;
    
    // Tenta path completo, depois remove partes até achar
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
    
    // Fallback para raiz "/"
    std::map<std::string, LocationConfig>::const_iterator it = _servers[0].locations.find("/");
    if (it != _servers[0].locations.end())
        return &(it->second);
    
    return NULL;
}

// Métodos getters - retornam valores do primeiro server (temporário)
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
    
    // Se path fornecido e location tem root específico, usa o da location
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
    
    // Se path fornecido e location tem index específico, usa o da location
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