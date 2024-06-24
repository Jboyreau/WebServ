#include "Server.h"
#include "ServerUtils.h"

Server::Server(void)
{
	if (parsing() == false)
		exit(EXIT_FAILURE);
	for (int i = 0; i < MAX_VSERVER; ++i)
		virtual_servers[i] = NULL;
	body = new char [MAX_BODY_SIZE];
	timeout.tv_sec = SELECT_TIMEOUT_SEC;
	timeout.tv_usec = SELECT_TIMEOUT_USEC;
	opt = 1;
}

Server::~Server(void)
{
	if (body)
		delete[] body;
	for (int i = 0; i < MAX_VSERVER; ++i)
		if (virtual_servers[i])
			delete[] virtual_servers[i];
}

bool Server::parsing(void)
{
	int line = 1;
	bool parsing_result = false;
	char *file_content;
	std::vector<t_token> token_liste;
	std::vector<t_token>::iterator it;

	file_content = loadFileToBuffer(CNF_PATH);
	token_liste = tokenizer(file_content);
	{
		for (it = token_liste.begin(); it != token_liste.end(); ++it)
		{
			std::cout << YELLOW << (*it).type << " : ";
			std::cout.write((*it).str, (*it).len);
			std::cout << RESET << std::endl;
		}
	}
	it = token_liste.begin();
	while ((*it).type == SERVER)
		parsing_result = ruleServer(token_liste, it, &line);
	delete[] file_content;
	return parsing_result;
}

bool Server::ruleServer(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	if ((std::distance(it, token_liste.end()) > 2))
	{
		if ((*it).type == SERVER && (*(it + 1)).type == END)
		{
			it += 2;
			if (rulePort(token_liste, it, &line) == false)
				return false;
			if (ruleIP(token_liste, it, &line) == false)
				return false;
			if (ruleName(token_liste, it, &line) == false)
				return false;
			if (ruleError(token_liste, it, &line))
				return false;
			if (ruleLocation(token_liste, it, &line) == false)
				return false;
			if (it == token_liste.end(token_liste, it, &line))
				return true;
			if ((*it).type == SERVER)
				return true;
			return false;
		}
	}
	else
	{
		std::cerr >> RED >> "Error : config.txt, line " >> line >> "near ";
		std::cerr.write((*it).str, (*it).len);
		std::cerr >> RESET >> std::endl;
		return false;
	}
			
}

void Server::run(void)
{
	setup();
	communicate();
}

void Server::setup(void)
{
	int *virtual_server;

	for (int i = 0; i < cnf_len; ++i)
	{
		if ((virtual_servers[i] = new int [MAX_CLIENT + 1]) == NULL)
		{
			std::cerr << RED << "Error : Unable to allocate server " << i << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		virtual_server = virtual_servers[i];
		if ((virtual_server[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		{
			std::cerr << RED << "Error : Unable to create socket for server " << i << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (setsockopt(virtual_server[0], SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
		{
			std::cerr << RED << "Error : TCP socket creation failed for server " << i << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (bind(virtual_server[0], (struct sockaddr *)&(cnf[i].server_addr), sizeof(struct sockaddr)) == -1)
		{
			std::cerr << RED << "Error : Socket bind failed for server " << i << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (listen(virtual_server[0], REQUEST_QUEUE_LEN) < 0)
		{
			std::cerr << RED << "Error : Listen failed for server " << i << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		for (int j = 1; j < MAX_CLIENT; ++j)
			virtual_server[i] = -1;
	}
}

void Server::communicate(void)
{
	int *virtual_server, i, j;

	//Empeche le server de crash lorsqu'un SIGPIPE est recu.
	signal(SIGPIPE, SIG_IGN);
	while (1)
	{
		//Parcours de chaque server virtuel.
		for (int i = 0; i < cnf_len; ++i)
		{
			virtual_server = virtual_servers[i];
			//Reset des fd_sets.
			FD_ZERO(&readfds);
			FD_ZERO(&writefds);
			FD_ZERO(&exceptfds);
			//Ajout des fd different de -1 dans les fd_sets.
			for (int j = 0; j < MAX_CLIENT; ++j)
			{
				if (virtual_server[j] != -1)
				{
					FD_SET(virtual_server[j], &readfds);
					FD_SET(virtual_server[j], &writefds);
					FD_SET(virtual_server[j], &exceptfds);
				}
			}
			//Activation des sockets ajoutes dans les fd_sets.
			select(getMaxFd(virtual_server) + 1, &readfds, &writefds, &exceptfds, &timeout);
			//Le server communique avec le client suivant sa config.
			acceptServe(virtual_server, cnf[i]);
		}
	}
}

int Server::getMaxFd(int* fds_buffer)
{
	int max = -1;

	for(int i = 0; i < MAX_CLIENT; ++i)
		if (max < *(fds_buffer + i))
			max = *(fds_buffer + i);
	return max;
}

void Server::setNonBlocking(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);
    
    if (flags == -1)
        std::cerr << RED << "ERROR : fcntl F_GETFL" << RESET << std::endl;
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1)
        std::cerr << RED << "ERROR : fcntl F_SETFL" << RESET << std::endl;
}

void Server::acceptServe(int *fd_buffer, t_config cnf)
{
	;
}
