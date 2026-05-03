#ifndef PARSERCONF
#define PARSERCONF

#include "TokenConf.hpp"

class ParserConf
{
    private:
        //location
        std::string _pathMethods;
        std::vector<std::string> _methods;
        //config
        int port;
        std::string _serverName;
        std::string _root;
        std::map<int, std::string> _errorPages;
        std::vector<std::string>_tokens;

    public:
        ~ParserConf();
        ParserConf(std::string _filename);
};

#endif