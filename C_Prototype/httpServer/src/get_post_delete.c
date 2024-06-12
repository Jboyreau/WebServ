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
#include "get_post_delete.h" //enum extension

#define DEFAULT_PATH "./default/default.html"

void feed_path(char *request, char *path)
{
	int i, j = 1;

	*path = '.';
	for (i = 0; *(request + i) != '\n' &&  *(request + i) != ' ' && *(request + i); ++i)
		;
	for (; *(request + i + j) != '\n' && *(request + i + j) && *(request + i + j) != ' '; ++j)
		*(path + j) = *(request + i + j);
	*(path + j) = 0;
	printf("*DEBUG! EXTRACTED PATH = %s\n", path);
}

static void respond_g(char* header, char *path, int client_socket_fd, int file_size, int block_size)
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

static void respond(char* header_body, char* path, int comm_socket_fd)
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

	strcpy(header_body, "HTTP/1.1 200 OK\n");
	strcat(header_body, "Server: My Personal HTTP Server\n");
	strcat(header_body, "Content-Length: ");
	itoa(body_len, body_size);
	strcat(header_body, body_size);
	strcat(header_body, "\n");
	strcat(header_body, "Connection: close\n"); 
	// Déterminer le type de contenu en fonction de l'extension du fichier.	
	ext = strrchr(path, '.');
	if (ext != NULL)
	{	
		key = keygen(ext);
		printf("*DEBUG! EXT = %s\n", ext);
		switch (key)
		{
			case HTML_KEY:
				strcat(header_body, "Content-Type: text/html; charset=UTF-8\n");
				break;
			case HTM_KEY:
				strcat(header_body, "Content-Type: text/html; charset=UTF-8\n");
				break;
			case CSS_KEY:
				strcat(header_body, "Content-Type: text/css\n");
				break;
			case JS_KEY:
				strcat(header_body, "Content-Type: application/javascript\n");
				break;
			case JSON_KEY:
				strcat(header_body, "Content-Type: application/json\n");
				break;
			case XML_KEY:
				strcat(header_body, "Content-Type: application/xml\n");
				break;
			case TXT_KEY:
				strcat(header_body, "Content-Type: text/plain\n");
				break;
			case JPEG_KEY:
				strcat(header_body, "Content-Type: image/jpeg\n");
				break;
			case JPG_KEY:
				strcat(header_body, "Content-Type: image/jpeg\n");
				break;
			case PNG_KEY:
				strcat(header_body, "Content-Type: image/png\n");
				break;
			case GIF_KEY:
				strcat(header_body, "Content-Type: image/gif\n");
				break;
			case SVG_KEY:
				strcat(header_body, "Content-Type: image/svg+xml\n");	
				break;
			case ICO_KEY:
				strcat(header_body, "Content-Type: image/x-icon\n");	
				break;
			case PDF_KEY:
				strcat(header_body, "Content-Type: application/pdf\n");	
				break;
			case ZIP_KEY:
				strcat(header_body, "Content-Type: application/zip\n");	
				break;
			case MP4_KEY:
				strcat(header_body, "Content-Type: video/mp4\n");
				break;
			case WEBM_KEY:
				strcat(header_body, "Content-Type: video/webm\n");
				break;
			case MP3_KEY:
				strcat(header_body, "Content-Type: audio/mpeg\n");
				break;
			case WAV_KEY:
				strcat(header_body, "Content-Type: audio/wav\n");
				break;
			default: //extension inconnue. 
				strcat(header_body, "Content-Type: application/octet-stream\n"); //Le client telecharge le fichier.
		}
	}
	else //pas d'extension.
		strcat(header_body, "Content-Type: application/octet-stream\n"); //Le client telecharge le fichier.
	strcat(header_body, "\n");
}

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

void get_methode(char* header, char *request, int comm_socket_fd)
{
	char body_size[14], path[PATH_MAX];
	struct stat st;
/*1.Parsing du file path*/
	feed_path(request, path);
	if (stat(path, &st) == 0 && strcmp(path, "./") && (st.st_mode & S_IFMT) == S_IFREG) //path existe &&  path != "./" && le fichier est regulier.
	{
/*2.Remplir le header puis L'envoyer avec le body*/
		fill_header(path, header, body_size, st.st_size);
		respond_g(header, path, comm_socket_fd, st.st_size, st.st_blksize);
		return;
	}
	if (stat(DEFAULT_PATH, &st) == 0)
	{
/*2ALT. pareil aue l'etape 2 avec un path par default*/
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
