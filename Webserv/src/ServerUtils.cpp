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
