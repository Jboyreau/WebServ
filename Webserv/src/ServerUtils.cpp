#include "ServerUtils.h"

char* loadFileToBuffer(const char* filename)
{
	
	char *buffer;
	std::ifstream file(filename, std::ios::binary | std::ios::ate); //Ouvrir en mode binaire et placer le curseur à la fin
	if (!file)
	{
		std::cerr << RED << "Error : Ubnable to open the configuration file." << RESET << std::endl;
		exit(EXIT_FAILURE);
	}
	std::streamsize size = file.tellg(); //Obtenir la taille du fichier
	file.seekg(0, std::ios::beg); //Revenir au début du fichier
	buffer = new char[size + 1]; //Allouer le buffer avec un octet supplémentaire pour le caractère nul
	if (file.read(buffer, size)) //Lire le fichier dans le buffer
	{
		buffer[size] = '\0'; //Ajouter le caractère nul à la fin du buffer
		return buffer;
	}
	delete[] buffer;
	std::cerr << RED << "Error : Configuration read failed." << RESET << std::endl;
	exit(EXIT_FAILURE);
}

uint32_t ftInetAddr(const char *ip_address)
{
	unsigned long int result = 0;
	int byte, i;

	for (i = 0; *(ip_address + i) != ';' && *(ip_address + i) != '\n' && *(ip_address + i); ++i)
		if (!isdigit(*(ip_address + i)) && *(ip_address + i) != '.')
			return INADDR_NONE;
	for (i = 1; *ip_address != ';' && *ip_address != '\n' && *ip_address; ++i)
	{
		byte = atoi(ip_address);
		if (byte < 0 || byte > 255)
			return INADDR_NONE;
		result += (byte << ((4 - i) * 8));
		while(*ip_address != '.' && *ip_address != ';' && *ip_address != '\n' && *ip_address)
			++ip_address;
		if (*ip_address == '.')
			++ip_address;
	}
	return htonl(result);
}

t_token findType(char *buffer, int *index)
{
	t_token token;
	int len = 0;

	while (*(buffer + len) && *(buffer + len) != ' ' && *(buffer + len) != '\n' && *(buffer + len) != '\t')
		++len;
	(*index) += len;
	token.str = buffer;
	token.len = len;
	if (strncmp(buffer, "server", len) == 0)
	{
		token.type = SERVER;
	}
	else if (strncmp(buffer, "listen", len) == 0)
	{
		token.type = PORT;
	}
	else if (strncmp(buffer, "host", len) == 0)
	{
		token.type = IP;
	}
	else if (strncmp(buffer, "server_name", len) == 0)
	{
		token.type = NAME;
	}
	else if (strncmp(buffer, "error_page", len) == 0)
	{
		token.type = ERR;
	}
	else if (strncmp(buffer, "client_max_body_size", len) == 0)
	{
		token.type = SIZE;
	}
	else if (strncmp(buffer, "location", len) == 0)
	{
		token.type = LOC;
	}
	else if (strncmp(buffer, "allow_methods", len) == 0)
	{
		token.type = METH;
	}
	else if (strncmp(buffer, "root", len) == 0)
	{
		token.type = ROOT;
	}
	else if (strncmp(buffer, "autoindex", len) == 0)
	{
		token.type = AUTOINDEX;
	}
	else if (strncmp(buffer, "index", len) == 0)
	{
		token.type = INDEX;
	}
	else if (strncmp(buffer, "cgi_path", len) == 0)
	{
		token.type = CGIPATH;
	}
	else if (strncmp(buffer, "return", len) == 0)
	{
		token.type = RETURN;
	}
	else
		token.type = WORD;
	return token;
}

std::vector<t_token> tokenizer(char *buffer)
{
	int i = 0;

	std::vector<t_token> token_liste;
	t_token token;

	while (*(buffer + i))
	{
		if (*(buffer + i) == ' ' || *(buffer + i) == '\n' || *(buffer + i) == '\t')
		{
			if (*(buffer + i) == '\n')
			{
				token.str = NL;
				token.len = 1;
				token.type = END;
				token_liste.push_back(token);
			}
			++i;
		}
		else if (*(buffer + i))
		{
			token = findType(buffer + i, &i);
			token_liste.push_back(token);
		}
	}
	return token_liste;
}
