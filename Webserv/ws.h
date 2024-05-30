#ifndef WS_H
#define WS_H

#include <iostream>
#include <vector>
#include <algorithm>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h> //type fd_set;
#include <sys/socket.h> //struct sockaddr_in; AF_INET; SOL_SOCKET; SO_REUSEADDR; inet_ntoa();
#include <netinet/in.h> //IPPROTO_TCP;
#include <arpa/inet.h>	//ntohs;
#include <string.h> //memeset(); strcpy;
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include "data_format.h"

#define CNF_PATH "./cnf/cnf.txt"

typedef struct Location_s
{
    std::string limit_except;
    std::string return_path;
    std::string root;
    bool autoindex;
    std::string index_file;
    std::string cgi_path;
} Location_t;

typedef struct ServerConfig_s 
{
	int listen_port;
	std::string host;
	std::string server_names;
	std::string error_page;
	int client_max_body_size;
 	std::unordered_map<std::string, Location_t> location;
}ServerConfig_t;

#endif
