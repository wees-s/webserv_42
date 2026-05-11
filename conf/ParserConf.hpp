#ifndef PARSERCONF
#define PARSERCONF

#include "TokenConf.hpp"

class ParserConf
{
    public:
        std::string _filename;
        //location
        std::string _pathMethods;
        std::vector<std::string> _methods;
        //config
        int port;
        std::string _serverName;
        std::string _root;
        std::map<int, std::string> _errorPages;
        std::vector<std::string>_tokens;

        ~ParserConf();
        ParserConf(std::string _filename);
        bool serverCheck(std::vector<std::string> tokens);
        void parseConfig();
        void parsePort(std::vector<std::string>::iterator& it);
        void parseServerName(std::vector<std::string>::iterator& it);
        void parseRoot(std::vector<std::string>::iterator& it);
        void parseLocation(std::vector<std::string>::iterator& it);
        void parseErrorPage(std::vector<std::string>::iterator& it);
};

#endif