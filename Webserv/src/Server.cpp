#include "Server.h"
#include "ServerUtils.h"

Server(void)
{
	if (!parsing(CNF_PATH))
		exit(EXIT_FAILURE);
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	opt = 1;
	cnf_len = 0;
}

bool Server::parsing(char *cnf_path)
{
		
	cnf_len = 2;	
}

Server::setup(void)
{
	for (int i = 0; i < cnf_len; ++i)
	{
		if (virtual_servers[i] = new int [cnf[i].max_client] == NULL)
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
	virtual_servers[i] = NULL;
}

Server::run(void)
{
	int *virtual_server, i, j;

	setup();
	while (1)
	{
		//Parcours de chaque server virtuel.
		for (int i = 0; virtual_servers[i]; ++i)
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
			//Communication
			client_accept_or_serve(virtual_server, cnf[i]);
		}
	}
}
