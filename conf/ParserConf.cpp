#include "ParserConf.hpp"
#include <cstdlib>

ParserConf::~ParserConf(){
}

ParserConf::ParserConf(std::string _filename){
    TokenConf tokens;
    _tokens = tokens.tokenizeConfig(_filename);
    this->_filename = _filename;
}

bool ParserConf::serverCheck(std::vector<std::string> tokens){
    
    if (tokens[0] != "server" || tokens[1] != "{" || tokens.back() != "}"){
        std::cout << "Sintaxe Error: " << _filename << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return true;
}