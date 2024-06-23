#ifndef SERVERUTILS_H
#define SERVERUTILS_H


#include <iostream>
#include <fstream>
#include <ctype.h> //isdigit
#include <stdlib.h>
#include <netinet/in.h> //IPPROTO_TCP

#define RED "\033[1;31m" //Error
#define YELLOW "\033[1;33m" //Debug
#define GREEN "\033[1;32m" //Info
#define RESET "\033[0m"

uint32_t ftInetAddr(const char *ip_address);
char* loadFileToBuffer(const char* filename);

#endif
