#include <string.h> //memeset(); strcpy; strcat();
#include <limits.h> //PATH_MAX
#include <stdlib.h> //
#include <stdio.h> //
#include <unistd.h> //read(); write();
#include <fcntl.h> ///open()
#include <sys/types.h> //send();
#include <sys/socket.h> //send();
#include <sys/stat.h> //stat()
#include "Server.h"
#include "itoa.h" //itoa
#include "get_post_delete.h" //enum extension

#define DEFAULT_PATH "./default/default.html"
#define POST_PATH "./media/gif/upload.gif"

int get_fsize(char *request, int comm_socket_fd)
{
	int len;
	char *content_length_str = strstr(request, "Content-Length: ");
	const char *error_message = "HTTP/1.1 411 Length Required\n\n";

	if (!content_length_str)
	{
		// Error: No Content-Length found
		send(comm_socket_fd, error_message, strlen(error_message), 0);
		return -1;
	}
	content_length_str += strlen("Content-Length: ");
	len = atoi(content_length_str);
	if (len > MAX_BODY_SIZE)
	{
		//gerer cette erreur plus tard sans oublier l'overflow.
		return -1;
	}
	return len;
}

static void feed_path(char *request, char *path)
{
	int i, j = 1;

	*path = '.';
	for (i = 0; *(request + i) != '\n' &&  *(request + i) != ' ' && *(request + i); ++i)
		;
	for (; *(request + i + j) != '\n' && *(request + i + j) != ' ' && *(request + i + j); ++j)
		*(path + j) = *(request + i + j);
	*(path + j) = 0;
}

static void respond_temp(char* header_body, char* path, int comm_socket_fd)
{
	send(comm_socket_fd, header_body, strlen(header_body), 0);
}

static int keygen(const char *ext)
{
	int sum = 0;
	int i;

	for (i = 1; *(ext + i); ++i)
		sum += *(ext + i);
	return sum * *(ext + 1);
}

static void fill_header(const char* path, char* header_body, char* body_size, int body_len)
{
	const char *ext;
	int key;

	strcpy(header_body, "HTTP/1.1 200 OK\r\n");
	strcat(header_body, "Server: My Personal HTTP Server\r\n");
	strcat(header_body, "Content-Length: ");
	itoa(body_len, body_size);
	strcat(header_body, body_size);
	strcat(header_body, "\r\n");
	strcat(header_body, "Connection: close\r\n"); 
	// Déterminer le type de contenu en fonction de l'extension du fichier.	
	ext = strrchr(path, '.');
	if (ext != NULL)
	{	
		key = keygen(ext);
		printf("*DEBUG! EXT = %s\r\n", ext);
		switch (key)
		{
			case HTML_KEY:
				strcat(header_body, "Content-Type: text/html; charset=UTF-8\r\n");
				break;
			case HTM_KEY:
				strcat(header_body, "Content-Type: text/html; charset=UTF-8\r\n");
				break;
			case CSS_KEY:
				strcat(header_body, "Content-Type: text/css\r\n");
				break;
			case JS_KEY:
				strcat(header_body, "Content-Type: application/javascript\r\n");
				break;
			case JSON_KEY:
				strcat(header_body, "Content-Type: application/json\r\n");
				break;
			case XML_KEY:
				strcat(header_body, "Content-Type: application/xml\r\n");
				break;
			case TXT_KEY:
				strcat(header_body, "Content-Type: text/plain\r\n");
				break;
			case JPEG_KEY:
				strcat(header_body, "Content-Type: image/jpeg\r\n");
				break;
			case JPG_KEY:
				strcat(header_body, "Content-Type: image/jpeg\r\n");
				break;
			case PNG_KEY:
				strcat(header_body, "Content-Type: image/png\r\n");
				break;
			case GIF_KEY:
				strcat(header_body, "Content-Type: image/gif\r\n");
				break;
			case SVG_KEY:
				strcat(header_body, "Content-Type: image/svg+xml\r\n");	
				break;
			case ICO_KEY:
				strcat(header_body, "Content-Type: image/x-icon\r\n");	
				break;
			case PDF_KEY:
				strcat(header_body, "Content-Type: application/pdf\r\n");	
				break;
			case ZIP_KEY:
				strcat(header_body, "Content-Type: application/zip\r\n");	
				break;
			case MP4_KEY:
				strcat(header_body, "Content-Type: video/mp4\r\n");
				break;
			case WEBM_KEY:
				strcat(header_body, "Content-Type: video/webm\r\n");
				break;
			case MP3_KEY:
				strcat(header_body, "Content-Type: audio/mpeg\r\n");
				break;
			case WAV_KEY:
				strcat(header_body, "Content-Type: audio/wav\r\n");
				break;
			default: //extension inconnue. 
				strcat(header_body, "Content-Type: application/octet-stream\r\n"); //Le client telecharge le fichier.
		}
	}
	else //pas d'extension.
		strcat(header_body, "Content-Type: application/octet-stream\r\n"); //Le client telecharge le fichier.
	strcat(header_body, "\r\n");
}

void Server::respond(const char *path, int client_socket_fd, int file_size)
{
	int response_len = strlen(response), file_to_send_fd, total_sent_bytes, sent_bytes, read_bytes, i;

	//Ouverture du fichier a envoyer:
	file_to_send_fd = open(path, O_RDONLY);
	if (file_to_send_fd < 0)
	{
		printf("Error: unable to open %s\n", path);
		return;
	}
	//Envoi du header
	//Verification des valeurs de retour de send() pour eviter les infinites loop en cas de spam.
	total_sent_bytes = 0;
	sent_bytes = 1;
	while (total_sent_bytes < response_len && sent_bytes > 0)
	{
		sent_bytes = send(client_socket_fd, response + total_sent_bytes, response_len - total_sent_bytes, 0); //Envoie du chunk.
		total_sent_bytes += sent_bytes;
	}
	//Envoi du fichier
printf("RESPOND DEBUG: file_size = %d\n", file_size);
	read_bytes = 0;
	total_sent_bytes = 0;
	i = 0;
	while (total_sent_bytes < file_size && i < TIMEOUT)
	{
		read_bytes = read(file_to_send_fd, body + total_sent_bytes, file_size); //Chargement du chunk dans le buffer.
		total_sent_bytes += read_bytes * (read_bytes > 0);
		++i;
	}
printf("RESPOND DEBUG: total_read_bytes = %d\n", total_sent_bytes);
	i = 0;
	sent_bytes = 0;
	total_sent_bytes = 0;
	while (total_sent_bytes < file_size && i < TIMEOUT)
	{
		sent_bytes = send(client_socket_fd, body + total_sent_bytes, file_size, 0); //Envoie du chunk.
		total_sent_bytes += sent_bytes * (sent_bytes > 0);
		++i;
	}
printf("RESPOND DEBUG: total_sent_bytes = %d\n", total_sent_bytes);
	close(file_to_send_fd);
}
void Server::get_methode(int comm_socket_fd)
{
	char body_size[14], path[PATH_MAX];
	struct stat st;
/*1.Parsing du file path*/
	feed_path(request, path);
	if (stat(path, &st) == 0 && strcmp(path, "./") && (st.st_mode & S_IFMT) == S_IFREG) //path existe &&  path != "./" && le fichier est regulier.
	{
/*2.Remplir le header puis L'envoyer avec le body*/
		fill_header(path, response, body_size, st.st_size);
		respond(path, comm_socket_fd, st.st_size);
		return;
	}
	if (stat(DEFAULT_PATH, &st) == 0)
	{
/*2ALT. pareil que l'etape 2 avec un path par default*/
		fill_header(DEFAULT_PATH, response, body_size, st.st_size);
		respond(DEFAULT_PATH, comm_socket_fd, st.st_size);
		return;
	}
	send(comm_socket_fd, "", 0, 0);
}

void Server::post_methode(char *header_end, int comm_socket_fd, int body_chunk_size)
{
	int file_fd, file_size, recv_bytes, wrote_bytes, i, total_recv_bytes;
	char body_size[14], path[PATH_MAX];
	struct stat st;

	//Trouver le chemin du fichier.
	feed_path(request, path);
	//Trouver la taille du fichier.
	file_size = get_fsize(request, comm_socket_fd);
	if (file_size < 0)
		return ;
	file_fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (file_fd < 0)
    {
        printf("Error : unable to create %s\n", path);
        return;
    }
	//Recevoir le fichier.
//printf("POST DEBUG body_chunk_size = %d\n",body_chunk_size);
	//Ecrire le morceau de body en trop.
	//TODO::turn into loop.
	if (body_chunk_size > 0)
	{
		recv_bytes = write(file_fd, header_end, body_chunk_size);
		file_size -= recv_bytes;
	}
	total_recv_bytes = 0;
	i = 0;
printf("POST DEBUG file_size = %d\n", file_size);
	//Chargement du buffer avec recv.
    while (total_recv_bytes < file_size && i < TIMEOUT)
    {
       recv_bytes = recv(comm_socket_fd, body + total_recv_bytes, file_size, 0);
		total_recv_bytes += recv_bytes * (recv_bytes > 0);
		++i;
    }
	//Ecriture du buffer content avec write()
	total_recv_bytes = 0;
	i = 0;
	while (total_recv_bytes < file_size && i < TIMEOUT)
	{
		wrote_bytes = write(file_fd, body + total_recv_bytes, file_size);
        total_recv_bytes += wrote_bytes * (wrote_bytes > 0);
		++i;
	}
printf("POST DEBUG total_recv_bytes = %d\n", total_recv_bytes);
    close(file_fd);
}

void Server::delete_methode(int comm_socket_fd)
{
	char body_size[14], path[PATH_MAX];
	const char *success_message = "HTTP/1.1 200 OK\nContent-Length: 0\n\n";
    const char *error_message = "HTTP/1.1 404 Not Found\nContent-Length: 0\n\n";

	feed_path(request, path);
	if (remove(path) == 0)
	{
		// Fichier supprimé avec succès
        send(comm_socket_fd, success_message, strlen(success_message), 0);
	}
	else
	{
		// Échec de la suppression du fichier
        send(comm_socket_fd, error_message, strlen(error_message), 0);
	}
}

void Server::error_methode(int comm_socket_fd)
{
	const char *body_ = "ERROR methode.";
	char body_size[14], path[PATH_MAX];

	feed_path(request, path);
	fill_header(path, response, body_size, strlen(body_));
	strcat(response, body_);
	respond_temp(response, path, comm_socket_fd);
}
