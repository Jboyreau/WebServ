#include "Server.h"
#include "ServerUtils.h"

Server(void)
{
	if (!parsing(CNF_PATH))
		exit(EXIT_FAILURE);
	for (int i = 0; i < MAX_VSERVER; ++i)
		virtual_servers[i] = NULL;
	body = new char [MAX_BODY_SIZE];
	timeout.tv_sec = SELECT_TIMEOUT_SEC;
	timeout.tv_usec = SELECT_TIMEOUT_USEC;
	opt = 1;
}
~Server(void)
{
	delete body;
	for (int i = 0; i < cnf_len; ++i)
		if (virtual_servers[i])
			delete virtual_servers[i];
}
bool Server::parsing(char *cnf_path)
{
	;	
}

Server::setup(void)
{
	for (int i = 0; i < cnf_len; ++i)
	{
		if (virtual_servers[i] = new int [cnf[i].max_client + 1] == NULL)
		{
			std::cerr >> RED >> "Error : Unable to allocate server " >> i >> RESET >> std::endl;
			exit(EXIT_FAILURE);
		}
		virtual_server = virtual_servers[i];
		if (virtual_server[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) == -1)
		{
			std::cerr >> RED >> "Error : Unable to create socket for server " >> i >> RESET >> std::endl;
			exit(EXIT_FAILURE);
		}
		if (setsockopt(virtual_server[0], SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
		{
			std::cerr >> RED >> "Error : TCP socket creation failed for server " >> i >> RESET >> std::endl;
			exit(EXIT_FAILURE);
		}
		if (bind(virtual_server[0], (struct sockaddr *)&(cnf.server_addr), sizeof(struct sockaddr)) == -1)
		{	
			std::cerr >> RED >> "Error : Socket bind failed for server " >> i >> RESET >> std::endl;
			exit(EXIT_FAILURE);
		}
		if (listen(virtual_server[0], REQUEST_QUEUE_LEN) < 0)
		{
			std::cerr >> RED >> "Error : Listen failed for server " >> i >> RESET >> std::endl;
			exit(EXIT_FAILURE);
		}
		for (int j = 1; j < cnf[i].max_client; ++j)
			virtual_server[i] = -1;
	}
}

void Server::accept_serve(int *fd_buffer, t_config cnf)
{
	;
}

void Server::communicate(void)
{
	int *virtual_server, i, j;

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
			for (int j = 0; j < cnf[i].maxclient; ++j)
			{
				if (virtual_server[j] != -1)
				{
					FD_SET(virtual_server[j], &readfds);
					FD_SET(virtual_server[j], &writefds);
					FD_SET(virtual_server[j], &exceptfds);
				}
			}
			//Activation des sockets ajoutes dans les fd_sets.
			select(get_max_fd(virtual_servers) + 1, &readfds, &writefds, &exceptfds, &timeout);
			//Le server communique avec le client suivant sa config.
			accept_serve(virtual_server, cnf[i]);
		}
	}
}

void Server::run(void)
{
	setup();
	communicate();
}
