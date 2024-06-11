#include <sys/time.h> //struct timeval
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h> //type fd_set;
#include <sys/types.h> //send();
#include <sys/socket.h> //struct sockaddr_in; AF_INET; SOL_SOCKET; SO_REUSEADDR; inet_ntoa(); send();
#include <netinet/in.h> //IPPROTO_TCP;
#include <arpa/inet.h>	//ntohs;
#include <string.h> //memeset(); strcpy; strcat();
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include "data_format.h"
#include "get_post_delete.h"

#define HEADER_BODY_SIZE 16384
#define SERVER_PORT 2000
#define REQUEST_SIZE 16384
#define MAX_CLIENT 42
#define MAX_REQUEST 5

int get_max_fd(int* fds_buffer)
{
	int max = -1;
	for(int i = 0; i < MAX_CLIENT; ++i)
		if (max < *(fds_buffer + i))
			max = *(fds_buffer + i);
	return max;
}

void SetupCommunicationTcpServer(char* request, int* fds_buffer)
{
/*
#######################################
#####################1.Initialisation:#
#######################################
	-master_sock_tcp_fd:
		Donne en argument a select(), permet d'accepter de nouveaux clients.
	-comm_socket_fd:
		File descriptor du socket de communication du client.
		Utilise pour l'echange de data entre client et serveur.
	-fd_set reafds:
		Ensemble des fds(master/clients) que select() va interroger.
	-server/client_addr:
		Contient les infos serveur/client.
		struct sockaddr_in
		{
        	sa_family_t		sin_family;	AF_INET
        	in_port_t		sin_port;	Numero de port
        	struct in_addr	sin_addr;	Adresse IPv4
		};
		struct in_addr
		{
			uint32_t	s_addr;	addresse ip dans l'ordre reseau (big/little endian)
		};
	-sent_recv_bytes:
		...
	-addr_len:
		...
	-opt:
		Contient le numero correspondant a l'option du socket set par setsockopt.
*/
	int master_sock_tcp_fd = 0, comm_socket_fd = 0, sent_recv_bytes = 0, opt = 1;
	fd_set readfds, writefds, exceptfds;
	struct sockaddr_in server_addr, client_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	int addr_len = sizeof(struct sockaddr);
	char header_body[HEADER_BODY_SIZE];
	struct timeval timeout;
	timeout.tv_sec = 0; // 0 secondes
	timeout.tv_usec = 0; // 0 microsecondes	

/*MULTIPLEXING!!!!
##########################################################
#####################1,5.Initialisation du tableau de fd.#
##########################################################
fds_buffer sera utilise pour stocker le master fd et les clients fds.
Puisque readfds est reinitilise a chaque tour de boucle,
il faut sauvguarder nos fd.
*/
	{
		for (int i = 0; i < MAX_CLIENT; ++i)
			*(fds_buffer + i) = -1;
		//-1 indique une case vide, sans fd.
	}
/*
##################################################
#####################2.Creation du master socket.#
##################################################
*/
	if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("socket creation failed\n");
		exit(EXIT_FAILURE);
	}
	/* 
		AF_INET: Famille d'adresse IPV4
		SOCK_STREAM: Socket de type tcp
		IPPROTO_TCP: Type de protocole tcp
	*/
	if (setsockopt(master_sock_tcp_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
	{
		printf("TCP socket creation failed for multiple connections\n");
		exit(EXIT_FAILURE);
	}
	/*
	* setsockopt()
	* permet de controler divers comportements du socket comme
	* la reutilisation des adresses ect.
	*/
	/*
		SOL_SOCKET (int):
			Specifie le niveau de l'option,
			l'option s'applique au niveau du socket lui-meme,
			plutot qu'a un niveau protocolaire comme TCP ou UDP.

		SO_REUSEADDR (int):
			Permet la reutilisation des adresses:ports locales si elles sont en attente,
			(dans un etat de "TIME_WAIT", usuelement apres la fermeture d'un socket).
			Utile pour les serveurs qui redemarrent rapidement,
			et qui doivent se lier a un port specifique immediatement apres une fermeture.

		(char*)&opt (void*):
			Contient la nouvelle valeur pour l'option specifiee.

		sizeof(opt) (socklen_t): taille de la variable pointee.

	*/

/*
####################################################
#####################3.Specifier les infos serveur.#
####################################################
bind() est un syscall permetant de dire a l'os quel type de donnees notre serveur attend.
*/
	if (bind(master_sock_tcp_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        printf("socket bind failed\n");
        return;
    }
/*
###################################################################
#####################4.Definir le nombre max de connexions client.#
###################################################################
Lorsque des requetes de plusieurs clients arrivent en meme temps,
une queue de n(second argument de listen) clients les stokera.
Les autres requetes n+x sont ignorees.
*/
    if (listen(master_sock_tcp_fd, MAX_REQUEST) < 0)
    {
        printf("listen failed\n");
        return;
    }
/*MULTIPLEXING!!!!
###########################################################
#####################4,5.Ajouter MasterFd dans fds_buffer.#
###########################################################
*/
	{
		*fds_buffer = master_sock_tcp_fd;
	}
	//Boucle infinie du serveur pour detecter/accepter et servir le client.
	while(1)
	{
		/*MULTIPLEXING!!!
		######################################################
		#####################5.Initialiser et remplir readfds#
		######################################################
		On copy fds_buffer dans readfds apres l'avoir reinitialise*/
		{
			FD_ZERO(&readfds); //vide readfds
			FD_ZERO(&writefds); //vide writefds
			FD_ZERO(&exceptfds); //vide exceptfds
			for (int i = 0; i < MAX_CLIENT; ++i)
				if (*(fds_buffer + i) != -1)
				{
					FD_SET(*(fds_buffer + i), &readfds);
					FD_SET(*(fds_buffer + i), &writefds);
					FD_SET(*(fds_buffer + i), &exceptfds);
				}
		}
		/*#####################################################
		#####################6.Attendre la connexion du client#
		#######################################################*/
		select(get_max_fd(fds_buffer) + 1, &readfds, &writefds, &exceptfds, &timeout);
		/*
			ATTENTION: select() est bloquant!!!
			select() se debloque lorsque le programme recoit:
				-demande de connexion d'un client
				-requete d'un client
			Arg1: 1 + valeur max presente dans readfds/fds_buffer.
			Arg2: collection des fd du socket.
			Arg3: ...
			Arg4: ...
			Arg5: ...
		*/
		/*
			Condition verifiant une demande de communication d'un client.
			Si un client demande le droit de parler avec le serveur,
			le master_sock_tcp_fd s'active.
			FD_ISSET(master_sock_tcp_fd, readfds) verifie cela.
		*/
		if (FD_ISSET(master_sock_tcp_fd, &readfds))
		{
			printf("New connection recvd\n");
			/*#######################################################
			#####################7.Accepter la demande de connection#
			#########################################################
			accept() renvoie un nouveau fd temporaire permettant d'identifier le client.*/
			comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *)&client_addr, &addr_len);
			if (comm_socket_fd < 0)
			{
				printf("accept error : errno = %d\n", errno);
				exit(EXIT_FAILURE);
			}
			
			/*MULTIPLEXING!!!!
			############################################################
			#####################7,5.Ajouter client fd dans fds_buffer.#
			############################################################
			fds_buffer est mis a jours avec comm_socket_fd*/
			{
				for (int i = 0; i < MAX_CLIENT; ++i)
					if (*(fds_buffer + i) == -1)
					{
						*(fds_buffer + i) = comm_socket_fd;
						break;
					}
			}
			printf(
				"Connection accepted from client : %s:%u\n",
				inet_ntoa(client_addr.sin_addr),
				ntohs(client_addr.sin_port));
		}
		else
		{
			/*
				Boucle de communication avec les clients.
			*/
			for (int i = 1; i < MAX_CLIENT; ++i)
			{
				if (FD_ISSET(*(fds_buffer + i), &exceptfds))
				{
					printf("Exception on client %d\n", *(fds_buffer + i));
                    close(*(fds_buffer + i));
                    *(fds_buffer + i) = -1;
				}
				else if (FD_ISSET(*(fds_buffer + i), &readfds))
				{
					comm_socket_fd = *(fds_buffer + i);
					printf("clear buffer.\n");
					memset(request, 0, REQUEST_SIZE);
					/*############################################################
					#####################8.Server recieving the data from client.#
					##############################################################
					L'IP & PORT sont stockes dans client_addr par l'os.
					Le serveur utilisera les infos du client pour repondre.
					ATTENTION!!! recvfrom() est bloquant.
					*/
					sent_recv_bytes = recvfrom(
						comm_socket_fd,
						(char*)request,
						REQUEST_SIZE,
						0,
						(struct sockaddr*)&client_addr, &addr_len);
					printf(
						"Server ready to recv %d bytes from client %s:%u\n",
						sent_recv_bytes,
						inet_ntoa(client_addr.sin_addr),
						ntohs(client_addr.sin_port));
					printf("-------------REQUEST CONTENT:\n%s\n", request);
					/*
					###########################################################
					#####################9.Verifier la fin de la communication#
					###########################################################
					Un message vide est envoye si un onglet est ferme,
					sent_recv_bytes == 0 -> true.
					*/
					if (sent_recv_bytes == 0)
					{
						//si le msg du client est vide, on passe au client suivant.
						printf(
							"-------------Communication ended by EMPTY MESSAGE with Client %s:%u\n",
							inet_ntoa(client_addr.sin_addr),
							ntohs(client_addr.sin_port));
						close(comm_socket_fd); // effacer client fd.
						*(fds_buffer + i) = -1;
						break;
					}
					/*
					###########################################################
					#####################9,5.Servir le client##################
					###########################################################
					*/
					//Detection des methodes GET, POST, DELETE.
					//Servir le client
					switch(what_methode(request))
					{
						case 'G' :
							get_methode(header_body, request, comm_socket_fd);
							break;
						case 'P' :
							post_methode(header_body, request, comm_socket_fd);
							break;
						case 'D' :
							delete_methode(header_body, request, comm_socket_fd);
							break;
						default :
							error_methode(header_body, request, comm_socket_fd);
					}
					//Debug.
					{
						int h_size;
						for (h_size = 0; *(header_body + h_size); ++h_size)
							if (*(header_body + h_size) == '\n')
								if (*(header_body + h_size + 1) == '\n')
								{
									++h_size;
									break;
								}
						printf("Total Size: %ld, Header Size: %d, Body Size: %ld\n", strlen(header_body), h_size, strlen(header_body + h_size + 1));
						printf("%s\n", header_body);
					}
					/*
					if (FD_ISSET(comm_socket_fd, &writefds))
					{
						sent_recv_bytes = sendto(comm_socket_fd,
								header_body,
								strlen(header_body),
								0,
								(struct sockaddr*)&client_addr,
								sizeof(struct sockaddr));
						printf("##########sent_recv_bytes = %d\n", sent_recv_bytes);
					}
					*/

					/*
					###########################################################
					#####################9,6.Fermeture de la communication#####
					###########################################################
					*/
					{
						close(comm_socket_fd);
						*(fds_buffer + i) = -1;
						printf("Communication socket closed\n");
					}
				}
			}
		}
		/*#################################################
		#####################10.Attendre un nouveau client#
		###################################################
		GoTo step 5*/
	}
}

int main(void)
{
	char request[REQUEST_SIZE];
	int  fds_buffer[MAX_CLIENT];
	SetupCommunicationTcpServer(request, fds_buffer);
	return EXIT_SUCCESS;
}
