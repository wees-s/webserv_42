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

void ParserConf::parseConfig(){
    std::vector<std::string>::iterator it = _tokens.begin();
    
    if (!serverCheck(_tokens))
        return;
    
    it += 2; // Estou pulando "server {"
    
    while (it != _tokens.end() && *it != "}"){
        if (*it == "listen"){
            parsePort(it);
        } else if (*it == "server_name"){
            parseServerName(it);
        } else if (*it == "root"){
            parseRoot(it);
        } else if (*it == "location"){
            parseLocation(it);
        } else if (*it == "error_page"){
            parseErrorPage(it);
        } else {
            it++;
        }
    }
}

void ParserConf::parsePort(std::vector<std::string>::iterator& it){
    it++; // Pulando "listen"
    if (it != _tokens.end()){
        port = std::atoi((*it).c_str());
        it++;
    }
    if (it != _tokens.end() && *it == ";"){
        it++;
    }
}

void ParserConf::parseServerName(std::vector<std::string>::iterator& it){
    it++; // Pulando "server_name"
    if (it != _tokens.end()){
        _serverName = *it;
        it++;
    }
    if (it != _tokens.end() && *it == ";"){
        it++;
    }
}

void ParserConf::parseRoot(std::vector<std::string>::iterator& it){
    it++; // Pulando "root"
    if (it != _tokens.end()){
        _root = *it;
        it++;
    }
    if (it != _tokens.end() && *it == ";"){
        it++;
    }
}

void ParserConf::parseLocation(std::vector<std::string>::iterator& it){
    it++; // Pulando "location"
    if (it != _tokens.end()){
        _pathMethods = *it; // Armazenando path
        it++;
    }
    if (it != _tokens.end() && *it == "{"){
        it++;
        while (it != _tokens.end() && *it != "}"){
            if (*it == "allow"){
                it++; // Pulando "allow"
                while (it != _tokens.end() && *it != ";"){
                    _methods.push_back(*it);
                    it++;
                }
                if (it != _tokens.end() && *it == ";"){
                    it++;
                }
            } else {
                it++;
            }
        }
        if (it != _tokens.end() && *it == "}"){
            it++;
        }
    }
}

void ParserConf::parseErrorPage(std::vector<std::string>::iterator& it){
    it++; // Pulando "error_page"
    if (it != _tokens.end()){
        int errorCode = std::atoi((*it).c_str());
        it++;
        if (it != _tokens.end()){
            _errorPages[errorCode] = *it;
            it++;
        }
    }
    if (it != _tokens.end() && *it == ";"){
        it++;
    }
}