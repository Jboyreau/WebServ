#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <vector>
#include <algorithm>

#include <unistd.h> //read(); write();
#include <stdlib.h> //
#include <stdio.h> //
#include <sys/time.h> //struct timeval
#include <signal.h> //signal();
#include <sys/select.h> //type fd_set;
#include <sys/types.h> //send();
#include <sys/socket.h> //struct sockaddr_in; AF_INET; SOL_SOCKET; SO_REUSEADDR; inet_ntoa(); send();
#include <netinet/in.h> //IPPROTO_TCP;
#include <arpa/inet.h>	//ntohs();
#include <string.h> //memeset(); strcpy; strcat();
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include <fcntl.h> //fcntl
#include <limits.h> //PATH_MAX
#include <sys/types.h> //send();
#include <sys/socket.h> //send();
#include <sys/stat.h> //stat()
#include <stdint.h> //unint32_t

#define TIMEOUT 1000000
#define SELECT_TIMEOUT_SEC 0
#define SELECT_TIMEOUT_USEC 0
#define HTTP_HEADER_SIZE 16384
#define MAX_VSERVER 5
#define MAX_CLIENT 10
#define REQUEST_QUEUE_LEN 100
#define MAX_BODY_SIZE 500000000 //byte
#define CNF_PATH "./config/config.txt"
#define RED "\033[1;31m"
#define YELLOW "\033[1;33m"
#define GREEN "\033[1;32m" 
#define RESET "\033[0m"

typedef struct s_config
{
	struct sockaddr_in server_addr;	
} t_config;

class Server
{
	private:
		t_config cnf[MAX_VSERVER];
		char response[HTTP_HEADER_SIZE];
		char request[HTTP_HEADER_SIZE];
		int *virtual_servers[MAX_VSERVER];
		struct sockaddr_in client_addr;
		struct timeval timeout;
		fd_set readfds;
		fd_set writefds;
		fd_set exceptfds;
		char *body;
		int opt;
		int cnf_len;
	public:
		Server(int opt);
		~Server(void);
		void run(void);	
};
#endif
