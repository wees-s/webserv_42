#ifndef TOKENCONF
#define TOKENCONF

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>

class TokenConf
{
    private:
        std::string _filename;
    public:
        ~TokenConf();
        TokenConf();
        std::vector<std::string> tokenizeConfig(const std::string& filename);
        std::vector<std::string> _tokens;
};

#endif