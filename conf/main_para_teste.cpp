/*

#include "ParserConf.hpp"
#include <iostream>

int main() {
    ParserConf parser("default.conf");
    
    parser.parseConfig();
    
    std::cout << "=== Parser Test Results ===" << std::endl;
    std::cout << "Port: " << parser.port << std::endl;
    std::cout << "Server Name: " << parser._serverName << std::endl;
    std::cout << "Root: " << parser._root << std::endl;
    
    std::cout << "\nLocation Path: " << parser._pathMethods << std::endl;
    std::cout << "Allowed Methods: ";
    for (size_t i = 0; i < parser._methods.size(); i++) {
        std::cout << parser._methods[i];
        if (i < parser._methods.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;
    
    std::cout << "\nError Pages:" << std::endl;
    for (std::map<int, std::string>::iterator it = parser._errorPages.begin(); it != parser._errorPages.end(); ++it) {
        std::cout << "  " << it->first << " -> " << it->second << std::endl;
    }
    
    return 0;
}
*/