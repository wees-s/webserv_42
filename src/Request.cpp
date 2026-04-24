#include "../include/Request.hpp"

Request::~Request() {}

Request::Request(const std::string& req)
{
	//separa o header do body, e já preenche body
	size_t pos = req.find("\r\n\r\n");
	std::string header_part = req.substr(0, pos);
	if (pos != std::string::npos)
		body = req.substr(pos + 4);
	else
		body = "";

	//Transforma a string header em um fluxo, como se fosse um arquivo, e tira /r do fim da primeira linha
	std::istringstream stream(header_part);
	std::string line;
	std::getline(stream, line);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	//Transforma a primeira linha do header em um fluxo, e preenche as variaveis >>
	std::istringstream first_line(line);
	first_line >> method >> path >> version;

	//preencher o map headers
	while (std::getline(stream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		size_t sep = line.find(":");
		if (sep == std::string::npos)
			continue ;

		std::string key = line.substr(0, sep);
		std::string value = line.substr(sep + 2);

		headers[key] = value;
	}
}
