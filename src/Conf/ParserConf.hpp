#ifndef PARSERCONF
#define PARSERCONF

#include "TokenConf.hpp"
#include <string>
#include <vector>
#include <map>

class ParserConf
{
    private:
        std::string _filename;
        //location
        std::string _pathMethods;
        std::vector<std::string> _methods;
        //config
        std::vector<int> _ports;
        std::string _serverName;
        std::string _root;
        std::string _index;
        long _clientMaxBodySize;
        std::map<int, std::string> _errorPages;
        std::vector<std::string> _tokens;
        std::vector<std::string> _cgiExtensions;
        std::string _uploadDir;

    public:
        ~ParserConf();
        ParserConf();
        
        //get
        std::vector<int> getPorts() const { return _ports; }
        std::string getServerName() const { return _serverName; }
        std::string getRoot() const { return _root; }
        std::string getIndex() const { return _index; }
        long getClientMaxBodySize() const { return _clientMaxBodySize; }
        std::map<int, std::string> getErrorPages() const { return _errorPages; }
        std::vector<std::string> getCgiExtensions() const { return _cgiExtensions; }
        std::string getUploadDir() const { return _uploadDir; }

        //bool
        bool isCgiExtension(const std::string& extension) const;
};

#endif