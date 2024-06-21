#include "ServerUtils.h"

uint32_t ft_inet_addr(const char *ip_address)
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
