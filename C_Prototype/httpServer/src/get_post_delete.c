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
#define POST_PATH "./media/gif/upload.gif"
#define MAX_FILE_SIZE 0x0FFFFFFF
#define TIMEOUT 100000

int get_fsize(char *request, int comm_socket_fd)
{
	int len;
	char *content_length_str = strstr(request, "Content-Length: ");
	char *error_message = "HTTP/1.1 411 Length Required\n\n";

	if (!content_length_str)
	{
		// Error: No Content-Length found
		send(comm_socket_fd, error_message, strlen(error_message), 0);
		return -1;
	}
	content_length_str += strlen("Content-Length: ");
	len = atoi(content_length_str);
	if (len > MAX_FILE_SIZE)
	{
		//gerer cette erreur plus tard sans oublier l'overflow.
		return -1;
	}
	return len;
}

static int purge_request(char **request)
{
    char *ptr = *request;
    int i = 0;

    while (*ptr + i)
    {
        if (strncmp(ptr + i, "\r\n\r\n", 4) == 0)
        {
            *request = ptr + 4 + i;
            return i + 4;
        }
        ++i;
    }
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
printf("FIND PATH DEBUG! EXTRACTED PATH = %s\n", path);
}

static void respond(char* header, char *path, int client_socket_fd, int file_size, int block_size)
{
	char *buffer;
	int header_len = strlen(header), file_to_send_fd, total_sent_bytes, sent_bytes, bytes_chunk_size, read_bytes, i;

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
	while (total_sent_bytes < header_len && sent_bytes > 0)
	{
		sent_bytes = send(client_socket_fd, header + total_sent_bytes, header_len - total_sent_bytes, 0); //Envoie du chunk.
		total_sent_bytes += sent_bytes;
	}
	//Envoi du fichier
	buffer = malloc(file_size); //Allocation du buffer de la taille d'un block/chunk.
	if (buffer == NULL)
	{
		printf("Error: unable to allocate GET BUFFER.\n");
		close(file_to_send_fd);
		return ;
	}

printf("RESPOND DEBUG: file_size = %d\n", file_size);
	read_bytes = 0;
	total_sent_bytes = 0;
	i = 0;
	while (total_sent_bytes < file_size && i < TIMEOUT)
	{
		read_bytes = read(file_to_send_fd, buffer + total_sent_bytes, file_size); //Chargement du chunk dans le buffer.
		total_sent_bytes += read_bytes * (read_bytes > 0);
		++i;
	}
	i = 0;
	sent_bytes = 0;
	total_sent_bytes = 0;
	while (total_sent_bytes < file_size && i < TIMEOUT)
	{
		sent_bytes = send(client_socket_fd, buffer + total_sent_bytes, file_size, 0); //Envoie du chunk.
		total_sent_bytes += sent_bytes * (sent_bytes > 0);
		++i;
	}
printf("RESPOND DEBUG: total_sent_bytes = %d\n", total_sent_bytes);
	close(file_to_send_fd);
	free(buffer);
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
		respond(header, path, comm_socket_fd, st.st_size, st.st_blksize);
		return;
	}
	if (stat(DEFAULT_PATH, &st) == 0)
	{
/*2ALT. pareil que l'etape 2 avec un path par default*/
		fill_header(DEFAULT_PATH, header, body_size, st.st_size);
		respond(header, DEFAULT_PATH, comm_socket_fd, st.st_size, st.st_blksize);
		return;
	}
	send(comm_socket_fd, "", 0, 0);
}


void post_methode(char* response, char *request, int comm_socket_fd, int total_recv_bytes)
{
	int file_fd, file_size, recv_bytes, wrote_bytes, body_chunk_size, i;
	char body_size[14], path[PATH_MAX], *buffer;
	struct stat st;

	//Trouver le chemin du fichier.
	feed_path(request, path);
	//Trouver la taille du fichier.
	file_size = get_fsize(request, comm_socket_fd);
	if (file_size < 0)
		return ;
	body_chunk_size = total_recv_bytes - purge_request(&request);
	file_fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (file_fd < 0)
    {
        printf("Error : unable to create %s\n", path);
        return;
    }
	//Recevoir le fichier.
	buffer = malloc(file_size);
	if (buffer == NULL)
	{
		printf("Error : unbale to allocate recv buffer\n");
		return;
	}
//printf("POST DEBUG body_chunk_size = %d\n",body_chunk_size);
	//Ecrire le morceau de body en trop.
	if (body_chunk_size > 0)
	{
		recv_bytes = write(file_fd, request, body_chunk_size);
		file_size -= recv_bytes;
	}
	//Verifier les valeurs de retour de recv et write pour eviter les boucles infinies.
	total_recv_bytes = 0;
	i = 0;
printf("POST DEBUG file_size = %d\n", file_size);
	//Chargement du buffer avec recv.
    while (total_recv_bytes < file_size && i < TIMEOUT)
    {
       recv_bytes = recv(comm_socket_fd, buffer + total_recv_bytes, file_size, 0);
		total_recv_bytes += recv_bytes * (recv_bytes > 0);
		++i;
    }
	//Ecriture du buffer content avec write()
	total_recv_bytes = 0;
	i = 0;
	while (total_recv_bytes < file_size && i < TIMEOUT)
	{
		wrote_bytes = write(file_fd, buffer + total_recv_bytes, file_size);
        total_recv_bytes += wrote_bytes * (wrote_bytes > 0);
		++i;
	}
printf("POST DEBUG total_recv_bytes = %d\n", total_recv_bytes);
    close(file_fd);
    free(buffer);
}

void delete_methode(char* header_body, char *request, int comm_socket_fd)
{
	char body_size[14], path[PATH_MAX];
	char *success_message = "HTTP/1.1 200 OK\nContent-Length: 0\n\n";
    char *error_message = "HTTP/1.1 404 Not Found\nContent-Length: 0\n\n";

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

void error_methode(char* header_body, char *request, int comm_socket_fd)
{
	char *body = "ERROR methode.", body_size[14], path[PATH_MAX];

	feed_path(request, path);
	fill_header(path, header_body, body_size, strlen(body));
	strcat(header_body, body);
	respond_temp(header_body, path, comm_socket_fd);
}
