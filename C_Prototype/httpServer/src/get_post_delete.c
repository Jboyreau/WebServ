#include <string.h> //memeset(); strcpy; strcat();
#include <limits.h> //PATH_MAX
#include <stdlib.h> //
#include <stdio.h> //
#include <unistd.h> //read(); write();
#include <fcntl.h> ///open()
#include <sys/types.h> //send();
#include <sys/socket.h> //send();
#include <sys/stat.h> //stat()
#include "itoa.h" //itoa

#define DEFAULT_PATH "./default/default.html"

char what_methode(char* request)
{
 	if (strncmp(request, "GET ", 4) == 0)
        return 'G';
	else if (strncmp(request, "POST ", 5) == 0)
        return 'P';
	else if (strncmp(request, "DELETE ", 7) == 0)
        return 'D';
    return '\0';
}

void feed_path(char *request, char *path)
{
	int i, j = 1;

	*path = '.';
	for (i = 0; *(request + i) != '\n' &&  *(request + i) != ' ' && *(request + i); ++i)
		;
	for (; *(request + i + j) != '\n' && *(request + i + j) && *(request + i + j) != ' '; ++j)
		*(path + j) = *(request + i + j);
	*(path + j) = 0;
	printf("EXTRACTED PATH = %s\n", path);
}

void respond_g(char* header, char *path, int client_socket_fd, int file_size, int block_size)
{
	char *buffer;
	//Ouverture du fichier a envoyer:
	int file_to_send_fd = open(path, O_RDONLY), sent_bytes, bytes_chunk_size;

	//Envoi du header
	sent_bytes = send(client_socket_fd, header, strlen(header), 0); //Envoie du chunk.

	//Envoi du fichier
	buffer = malloc(block_size); //Allocation du buffer de la taille d'un block/chunk.
	while(file_size > 0)
	{
		bytes_chunk_size = ((file_size < block_size) ? file_size : block_size);
		read(file_to_send_fd, buffer, bytes_chunk_size); //Chargement du chunk dans le buffer.
		sent_bytes = send(client_socket_fd, buffer, bytes_chunk_size, 0); //Envoie du chunk.
		file_size -= sent_bytes;
	}
	close(file_to_send_fd);
	free(buffer);	
}

void respond(char* header_body, char* path, int comm_socket_fd)
{
	send(comm_socket_fd, header_body, strlen(header_body), 0);
}

void fill_header(const char* path, char* header_body, char* body_size, int body_len)
{
	strcpy(header_body, "HTTP/1.1 200 OK\n");
	strcat(header_body, "Server: My Personal HTTP Server\n");
	strcat(header_body, "Content-Length: ");
	itoa(body_len, body_size);
	strcat(header_body, body_size);
	strcat(header_body, "\n");
	strcat(header_body, "Connection: close\n"); 
	// Déterminer le type de contenu en fonction de l'extension du fichier.
	const char *ext = strrchr(path, '.');
	if (ext != NULL)
	{
		if (strcmp(ext, ".html") == 0)
		{
			strcat(header_body, "Content-Type: text/html; charset=UTF-8\n");
		}
		else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
		{
			strcat(header_body, "Content-Type: image/jpeg\n");
		}
		else if (strcmp(ext, ".png") == 0)
		{
			strcat(header_body, "Content-Type: image/png\n");
		}
		else if (strcmp(ext, ".css") == 0)
		{
			strcat(header_body, "Content-Type: text/css\n");
		}
		else if (strcmp(ext, ".js") == 0)
		{
			strcat(header_body, "Content-Type: application/javascript\n");
		}
		else if (strcmp(ext, ".mp4") == 0)
		{
			strcat(header_body, "Content-Type: video/mp4\n");
		}
		else if (strcmp(ext, ".webm") == 0)
		{
			strcat(header_body, "Content-Type: video/webm\n");
		}
		else
			strcat(header_body, "Content-Type: application/octet-stream\n"); //Le client telecharge le fichier.
	}
	else
		strcat(header_body, "Content-Type: application/octet-stream\n"); //Le client telecharge le fichier.
	strcat(header_body, "\n");
}

void get_methode(char* header, char *request, int comm_socket_fd)
{
	char body_size[14], path[PATH_MAX];
	struct stat st;
	
	feed_path(request, path);
	if (stat(path, &st) == 0 && strcmp(path, "./") && (st.st_mode & S_IFMT) == S_IFREG) //path existe &&  path != "./" && le fichier est regulier.
	{
		fill_header(path, header, body_size, st.st_size);
		respond_g(header, path, comm_socket_fd, st.st_size, st.st_blksize);
		return;
	}
	if (stat(DEFAULT_PATH, &st) == 0)
	{
		fill_header(DEFAULT_PATH, header, body_size, st.st_size);
		respond_g(header, DEFAULT_PATH, comm_socket_fd, st.st_size, st.st_blksize);
		return;
	}
	send(comm_socket_fd, "", 0, 0);
}

void post_methode(char* header_body, char *request, int comm_socket_fd)
{
	char *body = "POST methode.", body_size[14], path[PATH_MAX];

	feed_path(request, path);
	fill_header(path, header_body, body_size, strlen(body));
	strcat(header_body, body);
	respond(header_body, path, comm_socket_fd);
}

void delete_methode(char* header_body, char *request, int comm_socket_fd)
{
	char *body = "DELETE methode.", body_size[14], path[PATH_MAX];

	feed_path(request, path);
	fill_header(path, header_body, body_size, strlen(body));
	strcat(header_body, body);
	respond(header_body, path, comm_socket_fd);
}

void error_methode(char* header_body, char *request, int comm_socket_fd)
{
	char *body = "ERROR methode.", body_size[14], path[PATH_MAX];

	feed_path(request, path);
	fill_header(path, header_body, body_size, strlen(body));
	strcat(header_body, body);
	respond(header_body, path, comm_socket_fd);
}
