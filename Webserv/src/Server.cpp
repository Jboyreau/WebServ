#include "Server.h"
#include "get_post_delete.h"

Server::Server(void)
{
	if (parsing() == false)
		exit(EXIT_FAILURE);
	for (int i = 0; i < MAX_VSERVER; ++i)
		virtual_servers[i] = NULL;
	if ((body = new char [MAX_BODY_SIZE]) == NULL)
	{
		std::cerr << RED << "Error : Unable to allocate Body." << RESET << std::endl;
		exit(EXIT_FAILURE);
	}
	memset(body, 0, MAX_BODY_SIZE);
	addr_len = sizeof(struct sockaddr);
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

void Server::run(void)
{
	setup();
	communicate();
}

void Server::setup(void)
{
	int *virtual_server = NULL;

	for (int i = 0; i < cnf_len; ++i)
	{
		for(int k = 0; k < i; ++k)
			if (cnf[i].server_addr.sin_port == cnf[k].server_addr.sin_port)
			{
				if ((virtual_servers[i] = new int [MAX_CLIENT + 1]) == NULL)
				{
					std::cerr << RED << "Error : Unable to allocate server " << i << '.' << RESET << std::endl;
					exit(EXIT_FAILURE);
				}
				*(virtual_servers[i]) =  *(virtual_servers[k]);
				virtual_server = virtual_servers[i];
				for (int l = 1; l < MAX_CLIENT + 1; ++l)
					virtual_server[l] = -1;
				break;
			}
		if (virtual_servers[i] != NULL)
			continue;
		if ((virtual_servers[i] = new int [MAX_CLIENT + 1]) == NULL)
		{
			std::cerr << RED << "Error : Unable to allocate server " << i << '.' << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		virtual_server = virtual_servers[i];
		if ((virtual_server[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		{
			std::cerr << RED << "Error : Unable to create socket for server " << i << '.' << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (setsockopt(virtual_server[0], SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
		{
			std::cerr << RED << "Error : TCP socket creation failed for server " << i << '.' << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (bind(virtual_server[0], (struct sockaddr *)&(cnf[i].server_addr), sizeof(struct sockaddr)) == -1)
		{
			std::cerr << RED << "Error : Socket bind failed for server " << i << '.' << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (listen(virtual_server[0], REQUEST_QUEUE_LEN) < 0)
		{
			std::cerr << RED << "Error : Listen failed for server " << i << '.' << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		for (int j = 1; j < MAX_CLIENT + 1; ++j)
			virtual_server[j] = -1;
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
			for (int j = 0; j < MAX_CLIENT + 1; ++j)
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
			acceptServe(virtual_server, cnf[i], *virtual_server);
		}
	}
}

void Server::acceptServe(int *fds_buffer, t_config conf, int master_sock_tcp_fd)
{
	int i;

	/*Accepter la demande de connection*/
	if (FD_ISSET(master_sock_tcp_fd, &readfds))
	{
		//accept() renvoie un nouveau fd temporaire permettant d'identifier le client.
		if ((comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *)&client_addr, &addr_len)) < 0)
		{
			std::cerr << RED << "Error : accept error : " << errno << RESET << std::endl;
			return ;
		}
		std::cout << GREEN << "Connection accepted from client : " << comm_socket_fd << RESET << std::endl;
		/*
		   fds_buffer est mis a jours avec comm_socket_fd,
		   chaque virtual_server (virtual_servers[i])
		   partage le meme index qu'un t_config (cnf[i]).
		   Chaque client stocke dans un virtual_server,
		   est donc lie a une congiguration.
		   On a donc la relaton suivante:
		   cnf[i] --> virtual_servers[i] --> comm_socket_fd.
		 */
		for (i = 1; i < MAX_CLIENT; ++i)
			if (*(fds_buffer + i) == -1)
			{
				*(fds_buffer + i) = comm_socket_fd;
				break ;
			}
		if (i != MAX_CLIENT)
			return ;
		close(comm_socket_fd);
	}
	/*Communication avec les clients*/
	for (int i = 1; i < MAX_CLIENT + 1; ++i)
	{
		if (FD_ISSET(*(fds_buffer + i), &exceptfds))
		{
			std::cerr << RED << "Error : Exception on client  " << *(fds_buffer + i) << RESET << std::endl;
			close(*(fds_buffer + i));
			*(fds_buffer + i) = -1;
		}
		else if (FD_ISSET(*(fds_buffer + i), &readfds))
		{
			comm_socket_fd = *(fds_buffer + i);
			setNonBlocking(comm_socket_fd);
			memset(request, 0, HTTP_HEADER_SIZE);
			int total_recv_bytes = 0;
			int recv_bytes = 0;
			char *header_end = NULL;
			int time_cout = 0;
			while (total_recv_bytes < HTTP_HEADER_SIZE && header_end == NULL && time_cout < TIMEOUT)
			{
				recv_bytes = recv(comm_socket_fd, request + total_recv_bytes, HTTP_HEADER_SIZE, 0);
				header_end = strstr(request, "\r\n\r\n");
				total_recv_bytes += recv_bytes * (recv_bytes > 0);
				++time_cout;
			}
			//Trouver la bonne configi suivant le nom.
			int name_len = 0;
			char *ptr;
			if ((ptr = strstr(request, "Host")) != NULL)
			{
				for (ptr += 6; *(ptr + name_len) != ':'; ++name_len)
					;
				std::string name(ptr, name_len);
				std::cout << YELLOW << "DEBUG : NAME = " << name << RESET << std::endl;
				int j;
				for (j = 0; j < cnf_len; ++j)
					if (*(virtual_servers[j]) == master_sock_tcp_fd && cnf[j].name_map.find(name) != cnf[j].name_map.end())
					{
						conf = cnf[cnf[j].name_map[name]];
						break;
					}
				std::cout << YELLOW << "DEBUG : Config Index = " << j << RESET << std::endl;	
			}
			header_end += 4;
			int body_chunk_size = total_recv_bytes - (header_end - request);
			std::cout << GREEN << "Recvd bytes = " << total_recv_bytes << " from client " << comm_socket_fd << RESET << std::endl;
			std::cout << YELLOW << "\n####REQUEST CONTENT####:\n" << request << RESET << std::endl;
			if (total_recv_bytes == 0 || recv_bytes < 0)
			{
				std::cout << GREEN << "Communication ended by EMPTY MESSAGE with Client" << comm_socket_fd << RESET << std::endl;
				close(comm_socket_fd);
				*(fds_buffer + i) = -1;
				break;
			}
			switch(*request + *(request + 1))
			{
				case GET :
					if (FD_ISSET(comm_socket_fd, &writefds))
						get_methode(comm_socket_fd);
					break;
				case POST :
					post_methode(header_end, comm_socket_fd, body_chunk_size);
					break;
				case DELETE :
					delete_methode(response, request, comm_socket_fd);
					break;
				default :
					error_methode(response, request, comm_socket_fd);
			}
			std::cout << YELLOW << "\n####RESPONSE CONTENT####:\n" << response << RESET << std::endl;
			close(comm_socket_fd);
			*(fds_buffer + i) = -1;
			std::cout << GREEN << "Communication socket closed" << RESET << std::endl;
		}
	}
}

bool Server::canAccessDirectory(const char *path)
{
	struct stat info;
	if (stat(path, &info) != 0)
	{
		return false; // Impossible de récupérer les informations du dossier
	}
	else if (info.st_mode & S_IFDIR)
	{
		return (access(path, X_OK) == 0); // Vérifier l'accès en lecture
	}
	return false; // Ce n'est pas un dossier
}

bool Server::canAccessFile(const char *path, int flag)
{
	struct stat info;
	if (stat(path, &info) != 0)
	{
		return false; // Impossible de récupérer les informations du fichier
	}
	else if (info.st_mode & S_IFREG) 
	{
		return (access(path, flag) == 0); // Vérifier l'accès en lecture
	}
	return false; // Ce n'est pas un fichier régulier
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
