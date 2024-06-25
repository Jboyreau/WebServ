#ifndef SERVERUTILS_H
#define SERVERUTILS_H


#include <iostream>
#include <fstream>
#include <vector>
#include <cctype> //ispace
#include <cstring> // pour strncmp
#include <ctype.h> //isdigit
#include <stdlib.h>
#include <netinet/in.h> //IPPROTO_TCP

#define RED "\033[1;31m" //Error
#define YELLOW "\033[1;33m" //Debug
#define GREEN "\033[1;32m" //Info
#define RESET "\033[0m"
#define NL "$"

enum token_type
{
	SERVER,
	PORT,
	IP,
	NAME,
	ERR,
	SIZE,
	LOC,
	METH,
	ROOT,
	AUTOINDEX,
	INDEX,
	CGIPATH,
	RETURN,
	WORD,
	END,
};

typedef struct s_token
{
	const char *str;
	size_t len;
	int	type;
}t_token;

uint32_t ftInetAddr(const std::vector<t_token>::iterator &it);
char* loadFileToBuffer(const char* filename);
std::vector<t_token> tokenizer(char *buffer);
bool is_valid_port_number(const std::vector<t_token>::iterator &it, int &port);

#endif
