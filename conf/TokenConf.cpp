#include "TokenConf.hpp"

TokenConf::TokenConf() {
}

TokenConf::~TokenConf() {
}

std::vector<std::string> TokenConf::tokenizeConfig(const std::string& filename) {
    std::ifstream file(filename.c_str());
    std::string word;
    _filename = filename;

    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo!" << std::endl;
        return _tokens;
    }
    //o operador >> lê palavra por palavra, ignorando espaços/tabs/newlines
    while (file >> word) {
        // LINHA DE TESTE>>>>>>>> std::cout << word << std::endl;
        //se o ponto e vírgula estiver colado na palavra separa 8080; = [8080][;]
        if (word.length() > 1 && word[word.length() - 1] == ';') {//se tem palavra, e o último caractere for ;
            _tokens.push_back(word.substr(0, word.length() - 1));//pegando tudo antes do ; e colocando push_back nos tokens.
            _tokens.push_back(";");//adicionando ; manualmente
        } else {
            _tokens.push_back(word);//caso contrário, apenas push_back.
        }
    }
    file.close();
    return _tokens;
}


//TESTE
/*int main() {
    TokenConf tokens;
    tokens.tokenizeConfig("default.conf");

    // Percorrendo os tokens com iterador (Padrão C++98)
    for (std::vector<std::string>::iterator it = tokens._tokens.begin(); it != tokens._tokens.end(); ++it) {
        std::cout << "Token: [" << *it << "]" << std::endl;
    }

    return 0;
}*/