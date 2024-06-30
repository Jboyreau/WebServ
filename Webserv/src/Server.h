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
#include <sstream>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include <fcntl.h> //fcntl
#include <limits.h> //PATH_MAX
#include <sys/types.h> //send();
#include <sys/socket.h> //send();
#include <sys/stat.h> //stat()
#include <stdint.h> //unint32_t
#include <dirent.h> // opendir, readdir, closedir
#include "ServerUtils.h"

#define TIMEOUT 10000000
#define SELECT_TIMEOUT_SEC 0
#define SELECT_TIMEOUT_USEC 0
#define HTTP_HEADER_SIZE 16384
#define MAX_VSERVER 5
#define MAX_CLIENT 10
#define REQUEST_QUEUE_LEN 100
#define MAX_BODY_SIZE 1000000000 //byte
#define MAX_SERVER_NAME_LEN 253
#define CNF_PATH "./config/config.txt"
#define DEFAULT_INDEX_PATH "./default/default.html"
#define DEFAULT_CGI_PATH "./cgi/php-cgi"
#define DEFAULT_ROOT "./"
#define RED "\033[1;31m" //Error
#define YELLOW "\033[1;33m" //Debug
#define GREEN "\033[1;32m" //Info
#define RESET "\033[0m"
#define E411 "./error/411.html"
#define E413 "./error/413.html"
#define E404 "./error/404.html"
#define E403 "./error/403.html"
#define E500 "./error/500.html"
#define E502 "./error/502.html"
#define E503 "./error/503.html"
#define E504 "./error/504.html"
#define OK "HTTP/1.1 200 OK\r\n"
#define H301 "HTTP/1.1 301 Moved Permanently\r\n"
#define H411 "HTTP/1.1 411 Length Required\r\n"
#define H413 "HTTP/1.1 413 Payload Too Large\r\n"
#define H404 "HTTP/1.1 404 Not Found\r\n"
#define H403 "HTTP/1.1 403 Forbidden\r\n"
#define H500 "HTTP/1.1 500 Internal Server Error\r\n"
#define H502 "HTTP/1.1 502 Bad Gateway\r\n"
#define H503 "HTTP/1.1 503 Service Unavailable\r\n"
#define H504 "HTTP/1.1 504 Gateway Timeout\r\n"

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
	std::map<std::string, int> name_map;
	std::map<std::string, std::string> error_map;
	size_t max_body_size;
	std::map<std::string, t_location> location_map;	
} t_config;

class Server
{
	private:
		std::map<std::string, std::string> http_error_map;
		std::string location_key;
		t_config cnf[MAX_VSERVER];
		t_config conf;
		t_location temp;
		char response[HTTP_HEADER_SIZE];
		char request[HTTP_HEADER_SIZE];
		char path[PATH_MAX];
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
		int loc_len;
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
		void acceptServe(int *fds_buffer, int master_sock_tcp_fd);
		int getMaxFd(int* fds_buffer);
		void setNonBlocking(int socket);
		bool canAccessDirectory(const char *path);
		bool canAccessFile(const char *path, int flag);
		//TEMP
		void post_methode(char *header_end, int comm_socket_fd, int body_chunk_size);
		void respond(const char *path, int client_socket_fd, int file_size);
		void get_methode(int comm_socket_fd);
		void delete_methode(int comm_socket_fd);
		void error_methode(int comm_socket_fd);
		const char* concatPath(void);
		void fillHeader(const char *first_field, const char *path, char* body_size, int body_len);
		void sendErr(int comm_socket_fd, std::string code);
		int get_fsize(char *request, int comm_socket_fd);
		bool findLongestMatchingPath(const char* location_key, std::map<std::string, t_location> &location_map, t_location &location);
};
#endif
