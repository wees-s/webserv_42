#ifndef PARSERCONF
#define PARSERCONF

#include <string>
#include <vector>
#include <map>
#include "TokenConf.hpp"

class ParserConf
{
    private:
        std::string _filename;
        std::vector<std::string> _tokens;
        
        struct LocationConfig {
            std::vector<std::string> methods;
            std::vector<std::string> cgiExtensions;
            std::string uploadDir;
            std::string root;
            std::string index;
            
            struct Redirect {
                int code;
                std::string new_path;
            };
            Redirect redirect;
            bool hasRedirect;
        };
        
        struct ServerConfig {
            std::vector<int> ports;
            std::string serverName;
            std::string root;
            std::string index;
            long clientMaxBodySize;
            std::map<int, std::string> errorPages;
            std::map<std::string, LocationConfig> locations;
        };
        
        std::vector<ServerConfig> _servers;

        const LocationConfig* findLocation(const std::string& path) const;
        void parseServerBlock(std::vector<std::string>::iterator& it);
        void parseListen(std::vector<std::string>::iterator& it, ServerConfig& server);
        void parseServerName(std::vector<std::string>::iterator& it, ServerConfig& server);
        void parseRoot(std::vector<std::string>::iterator& it, ServerConfig& server);
        void parseIndex(std::vector<std::string>::iterator& it, ServerConfig& server);
        void parseClientMaxBodySize(std::vector<std::string>::iterator& it, ServerConfig& server);
        void parseLocation(std::vector<std::string>::iterator& it, ServerConfig& server);
        void parseAllow(std::vector<std::string>::iterator& it, LocationConfig& loc);
        void parseCgiExtensions(std::vector<std::string>::iterator& it, LocationConfig& loc);
        void parseUploadDir(std::vector<std::string>::iterator& it, LocationConfig& loc);
        void parseReturn(std::vector<std::string>::iterator& it, LocationConfig& loc);
        void parseErrorPage(std::vector<std::string>::iterator& it, ServerConfig& server);
        long parseSize(const std::string& sizeStr);

    public:
        ~ParserConf();
        ParserConf(std::string filename);
        void parseConfig();
        
        std::vector<int> getPorts() const;
        std::string getServerName() const;
        std::string getRoot(const std::string& path = "") const;
        std::string getIndex(const std::string& path) const;
        long getClientMaxBodySize() const;
        std::map<int, std::string> getErrorPages() const;
        std::vector<std::string> getCgiExtensions(const std::string& path) const;
        std::string getUploadDir(const std::string& path) const;
        std::vector<std::string> getMethods(const std::string& path) const;
        bool hasRedirect(const std::string& path) const;
        int getRedirectCode(const std::string& path) const;
        std::string getRedirectPath(const std::string& path) const;
        bool isCgiExtension(const std::string& extension, const std::string& path) const;
        bool isMethodAllowed(const std::string& path, const std::string& method) const;
};

#endif