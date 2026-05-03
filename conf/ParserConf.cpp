#include "ParserConf.hpp"

ParserConf::~ParserConf(){
}

ParserConf::ParserConf(std::string _filename){
    TokenConf tokens;
    _tokens = tokens.tokenizeConfig(_filename);
}

