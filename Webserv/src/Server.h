#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <vector>
#include <map>
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
#include <string.h> //memset(); strcpy; strcat();
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include <fcntl.h> //fcntl
#include <limits.h> //PATH_MAX
#include <sys/types.h> //send();
#include <sys/socket.h> //send();
#include <sys/stat.h> //stat()
#include <stdint.h> //unint32_t
#include "ServerUtils.h"

#define TIMEOUT 1000000
#define SELECT_TIMEOUT_SEC 0
#define SELECT_TIMEOUT_USEC 0
#define HTTP_HEADER_SIZE 16384
#define MAX_VSERVER 5
#define MAX_CLIENT 10
#define REQUEST_QUEUE_LEN 100
#define MAX_BODY_SIZE 500000000 //byte
#define MAX_SERVER_NAME_LEN 253
#define CNF_PATH "./config/config.txt"
#define DEFAULT_INDEX_PATH "./default/default.html"
#define DEFAULT_CGI_PATH "./cgi/php-cgi"
#define DEFAULT_ROOT "./"
#define RED "\033[1;31m" //Error
#define YELLOW "\033[1;33m" //Debug
#define GREEN "\033[1;32m" //Info
#define RESET "\033[0m"

typedef struct s_location
{
	std::map<std::string, bool> method_map;
	char root[PATH_MAX];
	bool autoindex;
	char index[PATH_MAX];
	char cgi_path[PATH_MAX];
	char ret[PATH_MAX];
} t_location;

typedef struct s_config
{
	struct sockaddr_in server_addr;
	std::vector<char> names;
	std::map<std::string, std::string> error_map;
	size_t max_body_size;
	std::map<std::string, t_location> location_map;	
} t_config;

class Server
{
	private:
		t_config cnf[MAX_VSERVER];
		t_location temp;
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
		size_t cnf_len;
		unsigned int addr_len;
		int server_index;
		int comm_socket_fd;
	public:
		Server(void);
		~Server(void);
		bool parsing(void);
		bool ruleServer(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool rulePort(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleIp(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleName(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleName_(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleError(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleSize(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleLocation(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleMethode(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleMethode_(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleRoot(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleAutoIndex(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleIndex(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleCgiPath(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		bool ruleReturn(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line);
		void run(void);
		void setup(void);
		void communicate(void);
		void acceptServe(int *fds_buffer, t_config cnf, int master_sock_tcp_fd);
		int getMaxFd(int* fds_buffer);
		void setNonBlocking(int socket);
		bool canAccessDirectory(const char *path);
		bool canAccessFile(const char *path, int flag);
		//TEMP
		void post_methode(char* response, char *request, int comm_socket_fd, int total_recv_bytes);
		void respond(char* header, const char *path, int client_socket_fd, int file_size, int block_size);
		void get_methode(char* response, char *request, int comm_socket_fd);
};
#endif
