#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h> //type fd_set;
#include <sys/socket.h> //struct sockaddr_in; AF_INET; SOL_SOCKET; SO_REUSEADDR; inet_ntoa();
#include <netinet/in.h> //IPPROTO_TCP;
#include <arpa/inet.h>	//ntohs;
#include <string.h> //memeset();
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include "data_format.h"

#define SERVER_PORT 2000
#define BUFFER_SIZE 1024



int main(int argc, char **argv)
{
	setup_tcp_communication();
	printf("TCPClient quits.\n");
	return 0;
}
