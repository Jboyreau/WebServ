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
	opened_file = -1;
	addr_len = sizeof(struct sockaddr);
	timeout.tv_sec = SELECT_TIMEOUT_SEC;
	timeout.tv_usec = SELECT_TIMEOUT_USEC;
	opt = 1;
	http_error_map["403"] = H403;
	http_error_map["404"] = H404;
	http_error_map["405"] = H405;
	http_error_map["411"] = H411;
	http_error_map["413"] = H413;
	http_error_map["500"] = H500;
	http_error_map["502"] = H502;
	http_error_map["503"] = H503;
	http_error_map["504"] = H504;
}

Server::~Server(void)
{
	if (body)
		delete[] body;
	// Fermer tous les descripteurs de fichiers clients et maîtres
	for (server_index = 0; server_index < MAX_VSERVER; ++server_index)
	{
		if (virtual_servers[server_index])
		{
			for (int i = 0; i < MAX_CLIENT + 1; ++i)
			{
				if (virtual_servers[server_index][i] != -1)
				{
					close(virtual_servers[server_index][i]);
				}
			}
			delete[] virtual_servers[server_index];
		}
	}
	if (opened_file != -1)
		close(opened_file);
}

void Server::run(void)
{
	setup();
	communicate();
}

void Server::setup(void)
{
	int *virtual_server = NULL;

	for (server_index = 0; server_index < cnf_len; ++server_index)
	{
		//Determiner si un autre server bloc partage le meme port.
		for(int k = 0; k < server_index; ++k)
			if (cnf[server_index].server_addr.sin_port == cnf[k].server_addr.sin_port)
			{
				if ((virtual_servers[server_index] = new int [MAX_CLIENT + 2]) == NULL)
				{
					std::cerr << RED << "Error : Unable to allocate server " << server_index << '.' << RESET << std::endl;
					exit(EXIT_FAILURE);
				}
				*(virtual_servers[server_index]) =  *(virtual_servers[k]);//dupliquer le master socket.
				virtual_server = virtual_servers[server_index];
				int l;
				for (l = 1; l < MAX_CLIENT + 1; ++l)
					virtual_server[l] = -1;
				*(virtual_servers[server_index] + l) = 1; //la dernierre case permet de determiner si un master socket a ete duplique.
				*(virtual_servers[k] + l) = 1; //la dernierre case permet de determiner si un master socket a ete duplique.
				break;
			}
		if (virtual_servers[server_index] != NULL)
			continue;
		if ((virtual_servers[server_index] = new int [MAX_CLIENT + 2]) == NULL)
		{
			std::cerr << RED << "Error : Unable to allocate server " << server_index << '.' << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		virtual_server = virtual_servers[server_index];
		if ((virtual_server[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		{
			std::cerr << RED << "Error : Unable to create socket for server " << server_index << '.' << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (setsockopt(virtual_server[0], SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
		{
			std::cerr << RED << "Error : TCP socket creation failed for server " << server_index << '.' << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (bind(virtual_server[0], (struct sockaddr *)&(cnf[server_index].server_addr), sizeof(struct sockaddr)) == -1)
		{
			std::cerr << RED << "Error : Socket bind failed for server " << server_index << '.' << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (listen(virtual_server[0], REQUEST_QUEUE_LEN) < 0)
		{
			std::cerr << RED << "Error : Listen failed for server " << server_index << '.' << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		int j;
		for (j = 1; j < MAX_CLIENT + 1; ++j)
			virtual_server[j] = -1;
		virtual_server[j] = 0; //la dernierre case permet de determiner si un server a ete duplique.
	}
}

void Server::communicate(void)
{
	int *virtual_server, j;

	//Empeche le server de crash lorsqu'un SIGPIPE est recu.
	signal(SIGPIPE, SIG_IGN);
	while (1)
	{
		//Parcours de chaque server virtuel.
		for (server_index = 0; server_index < cnf_len; ++server_index)
		{
			virtual_server = virtual_servers[server_index];
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
			conf = cnf[server_index];
			acceptServe(virtual_server,*virtual_server);
		}
	}
}

bool Server::findLongestMatchingPath(const char* location_key, std::map<std::string, t_location> &location_map, t_location &location)
{
	char current_path[PATH_MAX]; //pointe le debut.
	int len = std::strlen(location_key);
	std::strncpy(current_path, location_key, len);
	current_path[len] = '\0';
	char *current_path_end = current_path + len -1; //pointe la fin.
	
	dir = (*current_path_end == '/');
	test_php = (std::strstr(location_key2.c_str(), ".php ") || std::strstr(location_key2.c_str(), ".php?"));

	std::cout << YELLOW << "$$$$$$$$ location.key $$$$$$$$\n" << location_key << RESET << std::endl;
	std::cout << YELLOW << "$$$$$$$$ test_php $$$$$$$$\n" << test_php << RESET << std::endl;

	loc_len = 0;
	while (current_path != current_path_end)
	{
		std::cout << YELLOW << "DEBUG : loop current_path = " << current_path << RESET << std::endl;
		if (location_map.find(current_path) != location_map.end())
		{
			location = location_map[current_path];
			std::cout << YELLOW << "DEBUG : found current_path = " << current_path << RESET << std::endl;
			loc_len = std::strlen(current_path);
			return true;
		}
		if (current_path_end != current_path && *current_path_end == '/')
			--current_path_end;
		while (current_path_end != current_path && *current_path_end != '/')
			--current_path_end;
		if (*current_path_end == '/')
			*(current_path_end + 1) = 0;
	}
	if (*current_path == '/' && current_path == current_path_end)
		if (location_map.find(current_path) != location_map.end())
		{
			location = location_map[current_path];
			std::cout << YELLOW << "DEBUG : found current_path = " << current_path << RESET << std::endl;
			loc_len = std::strlen(current_path);
			return true;
		}	
	return false;
}

void Server::acceptServe(int *fds_buffer, int master_sock_tcp_fd)
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
		sendErr(comm_socket_fd, "503");
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
			//Lire du header de la request.
			int total_recv_bytes = 0;
			int recv_bytes = 0;
			char *header_end = NULL;
			int time_cout = 0;
			while (total_recv_bytes < HTTP_HEADER_SIZE && header_end == NULL && time_cout < TIMEOUT)
			{
				recv_bytes = recv(comm_socket_fd, request + total_recv_bytes, HTTP_HEADER_SIZE, 0);
				header_end = std::strstr(request, "\r\n\r\n");
				total_recv_bytes += recv_bytes * (recv_bytes > 0);
				++time_cout;
			}
			//Trouver la bonne config suivant le nom.	
			char *ptr = NULL;
			if (*(virtual_servers[server_index] + MAX_CLIENT + 1) == 1)
			{
				std::cout << YELLOW << "DEUBUG DUPLICATE" << RESET << std::endl;
				if ((ptr = std::strstr(request, "Host")) != NULL)
				{
					int name_len = 0;
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
					if (cnf_len == j)
						for (j = 0; j < cnf_len; ++j)
							if (*(virtual_servers[j]) == master_sock_tcp_fd)
							{
								conf = cnf[cnf[j].name_map[name]];
								break;
							}
					std::cout << YELLOW << "DEBUG : Config Index = " << j << RESET << std::endl;
				}
			}
			//Initialiser le pointeur de fin du header et la taille du body chunk.
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
			//Trouver le path.
			int loc_len = 0;
			int j;
			for (j = 0; *(request + j) != ' ' && j < HTTP_HEADER_SIZE; ++j)
				;
			ptr = request + (++j);
			for (; *(request + j) != ' ' && j < HTTP_HEADER_SIZE; ++j)
				++loc_len;
			std::string lk(ptr, loc_len);
			location_key = lk;
			std::string lk2(ptr, loc_len + 1);
			location_key2 = lk2;
			//Trouver la bonne location.
			if (conf.location_map.size() != 0)
			{
				std::cout << GREEN << "LocationKey : " << location_key << RESET << std::endl;
				if (findLongestMatchingPath(location_key.c_str(), conf.location_map, temp))
				{
					std::cout << GREEN << "LocationPath_ : " << location_key << RESET << std::endl;
				}
				else
				{
					temp.method_map["GET"] = true;
					temp.method_map["POST"] = true;
					std::strcpy(temp.root, DEFAULT_ROOT);
					temp.autoindex = false;
					std::strcpy(temp.index, DEFAULT_INDEX_PATH);
					std::strcpy(temp.cgi_path, DEFAULT_CGI_PATH);
					memset(temp.ret, 0, PATH_MAX);
					std::cout << GREEN << "LocationPath0 : " << "default" << RESET << std::endl;
				}
			}
			else
			{
				temp.method_map["GET"] = true;
				temp.method_map["POST"] = true;
				std::strcpy(temp.root, DEFAULT_ROOT);
				temp.autoindex = false;
				std::strcpy(temp.index, DEFAULT_INDEX_PATH);
				std::strcpy(temp.cgi_path, DEFAULT_CGI_PATH);
				memset(temp.ret, 0, PATH_MAX);
				std::cout << GREEN << "LocationPath1 : " << "default" << RESET << std::endl;
			}
			//Afficher location params.
			std::cout << GREEN << "\tLocation : "<< RESET << std::endl;
			if (temp.method_map.find("GET") != temp.method_map.end())
				std::cout << GREEN << "\t\t\"GET\" " << temp.method_map["GET"] << RESET << std::endl;
			if (temp.method_map.find("POST") != temp.method_map.end())
				std::cout << GREEN << "\t\t\"POST\" " << temp.method_map["POST"] << RESET << std::endl;
			if (temp.method_map.find("DELETE") != temp.method_map.end())
				std::cout << GREEN << "\t\t\"DELETE\" " << temp.method_map["DELETE"] << RESET << std::endl;
			std::cout << GREEN << "\t\tROOT : " << temp.root << RESET << std::endl;
			std::cout << GREEN << "\t\tAUTOINDEX : " << temp.autoindex << RESET << std::endl;
			std::cout << GREEN << "\t\tINDEX : " << temp.index << RESET << std::endl;
			std::cout << GREEN << "\t\tCGIPATH : " << temp.cgi_path << RESET << std::endl;
			std::cout << GREEN << "\t\tRETURN : " << temp.ret << RESET << std::endl;
			//servir le client.
			int methode_len;
			for (methode_len = 0; *(request + methode_len) != ' ' && methode_len < HTTP_HEADER_SIZE; ++methode_len)
				;
			std::string methode_key(request, methode_len);
			std::cout << RED << "Methode key :" << methode_key << RESET <<std::endl;
			if (test_php)
			{
				if (temp.method_map.find(methode_key) != temp.method_map.end())
				{
					methode_CGI(header_end, body_chunk_size, methode_key);
					std::cout << YELLOW << "\n####RESPONSE CONTENT####:" << response << RESET << std::endl;
					close(comm_socket_fd);
					*(fds_buffer + i) = -1;
					std::cout << GREEN << "Communication socket closed" << RESET << std::endl;
					return;				
				}
				else
				{
					std::cout << RED << "GET not allowed" << RESET << std::endl;//send error method not allowed
					sendErr(comm_socket_fd, "405"); //error
				}
					
				}
			switch (*request + *(request + 1))
			{
				case GET :
					if (FD_ISSET(comm_socket_fd, &writefds))
					{
						if (temp.method_map.find(methode_key) != temp.method_map.end())
						{
							get_methode(comm_socket_fd);
						}
						else
						{
							std::cout << RED << "GET not allowed" << RESET << std::endl;//send error method not allowed
							sendErr(comm_socket_fd, "405"); //error
						}
					}	
					break;
				case POST :
					if (temp.method_map.find(methode_key) != temp.method_map.end())
					{
						post_methode(header_end, comm_socket_fd, body_chunk_size);
					}
					else
					{
						std::cout << RED << "POST not allowed" << RESET << std::endl;//send error method not allowed
						sendErr(comm_socket_fd, "405"); //error
					}
					break;
				case DELETE :
					if (temp.method_map.find(methode_key) != temp.method_map.end())
					{
						delete_methode(comm_socket_fd);
					}
					else
					{
						std::cout << RED << "DELETE not allowed" << std::endl;//send error method not allowed
						sendErr(comm_socket_fd, "405"); //error
					}
					break;
				default :
					std::cout << RED << "NOOB not allowed" << RESET << std::endl;//send error method not handeled
					sendErr(comm_socket_fd, "503"); //error
			}
			std::cout << YELLOW << "\n####RESPONSE CONTENT####:" << response << RESET << std::endl;
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

void Server::sendErr(int comm_socket_fd, std::string code)
{
	char body_size[14];
	struct stat st;

	std::strcpy(path, conf.error_map[code].c_str());
	stat(path, &st);
	fillHeader(http_error_map[code].c_str(), path, body_size, st.st_size);
	respond(path, comm_socket_fd, st.st_size);
}

int Server::handleChunk(int fd, char *buff, size_t len)
{
	return write(fd, buff, len);
}
