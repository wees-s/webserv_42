#ifndef PARSERCONF
#define PARSERCONF

#include <string>
#include <vector>
#include <map>

class ParserConf
{
    private:
        std::string _filename;
        
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
            std::map<std::string, LocationConfig> locations; // path -> LocationConfig
        };
        
        std::vector<ServerConfig> _servers;

        // Helper method para longest prefix match
        const LocationConfig* findLocation(const std::string& path) const;

    public:
        ~ParserConf();
        ParserConf();
        
        //get - retorna valores do primeiro server (temporário até ter lógica de seleção de server)
        std::vector<int> getPorts() const;
        std::string getServerName() const;
        std::string getRoot(const std::string& path = "") const;
        std::string getIndex(const std::string& path) const;
        long getClientMaxBodySize() const;
        std::map<int, std::string> getErrorPages() const;
        std::vector<std::string> getCgiExtensions(const std::string& path) const;
        std::string getUploadDir(const std::string& path) const;
        
        // location methods
        std::vector<std::string> getMethods(const std::string& path) const;
        
        // redirect
        bool hasRedirect(const std::string& path) const;
        int getRedirectCode(const std::string& path) const;
        std::string getRedirectPath(const std::string& path) const;
        
        // helper methods
        bool isCgiExtension(const std::string& extension, const std::string& path) const;
        bool isMethodAllowed(const std::string& path, const std::string& method) const;
};

#endif