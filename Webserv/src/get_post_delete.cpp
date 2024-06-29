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

int Server::get_fsize(char *request, int comm_socket_fd)
{
	int len;
	char *content_length_str = std::strstr(request, "Content-Length: ");
	const char *error_message = "HTTP/1.1 411 Length Required\n\n";

	if (!content_length_str)
	{
		// Error: No Content-Length found
		sendErr(comm_socket_fd, "411");
		return -1;
	}
	content_length_str += std::strlen("Content-Length: ");
	len = atoi(content_length_str);
	if (len > MAX_BODY_SIZE)
	{
		sendErr(comm_socket_fd, "413");
		return -1;
	}
	return len;
}

void Server::concatPath(void)
{
	const char *ptr;
	int i;
	if (*temp.ret == 0)
	{
		ptr = location_key.c_str();
		if (*ptr)
			ptr = location_key.c_str() + 1;
		for (i = 0; *(ptr + i) != '/' && *(ptr + i); ++i)
			;	
		if (*(ptr + i) == '/')
			ptr = ptr + i + 1;
		std::strcpy(path, temp.root);
		std::strcat(path, ptr);
	}
	else
	{
		ptr = temp.ret;
		if (*ptr)
			ptr = temp.ret + 1;
		for(i = 0; *(ptr + i) != '/' && *(ptr + i); ++i)
			;
		if (*(ptr + i) == '/')
			ptr = ptr + i + 1;
		std::strcpy(path, temp.root);
	std::cout << YELLOW << "PATH = " << path << RESET << std::endl;
		std::strcat(path, ptr);
	std::cout << YELLOW << "PATH = " << path << RESET << std::endl;
		ptr = location_key.c_str();
	std::cout << YELLOW << "pre loop ptr = " << ptr << RESET << std::endl;
	std::cout << YELLOW << "Location len =" << loc_len << RESET << std::endl;
		for (i = 0; i < loc_len; ++i)
			;
	ptr = ptr + i;
	std::cout << YELLOW << " post loop ptr + i = " << *(ptr + i) << RESET << std::endl;
	std::cout << YELLOW << " post loop ptr = " << ptr << RESET << std::endl;
	std::cout << YELLOW << "PATH = " << path << RESET << std::endl;
		std::strcat(path, ptr);
	}
	std::cout << YELLOW << "PATH = " << path << RESET << std::endl;
}

static int keygen(const char *ext)
{
	int sum = 0;
	int i;

	for (i = 1; *(ext + i); ++i)
		sum += *(ext + i);
	return sum * *(ext + 1);
}

void Server::fillHeader(const char *first_field, const char *path, char* body_size, int body_len)
{
	const char *ext;
	int key;

	std::strcpy(response, first_field);
	std::strcat(response, "Server: My Personal HTTP Server\r\n");
	std::strcat(response, "Content-Length: ");
	itoa(body_len, body_size);
	std::strcat(response, body_size);
	std::strcat(response, "\r\n");
	std::strcat(response, "Connection: close\r\n"); 
	// Déterminer le type de contenu en fonction de l'extension du fichier.	
	ext = std::strrchr(path, '.');
	if (ext != NULL)
	{
		key = keygen(ext);
		printf("*DEBUG! EXT = %s\r\n", ext);
		switch (key)
		{
			case HTML_KEY:
				std::strcat(response, "Content-Type: text/html; charset=UTF-8\r\n");
				break;
			case HTM_KEY:
				std::strcat(response, "Content-Type: text/html; charset=UTF-8\r\n");
				break;
			case CSS_KEY:
				std::strcat(response, "Content-Type: text/css\r\n");
				break;
			case JS_KEY:
				std::strcat(response, "Content-Type: application/javascript\r\n");
				break;
			case JSON_KEY:
				std::strcat(response, "Content-Type: application/json\r\n");
				break;
			case XML_KEY:
				std::strcat(response, "Content-Type: application/xml\r\n");
				break;
			case TXT_KEY:
				std::strcat(response, "Content-Type: text/plain\r\n");
				break;
			case JPEG_KEY:
				std::strcat(response, "Content-Type: image/jpeg\r\n");
				break;
			case JPG_KEY:
				std::strcat(response, "Content-Type: image/jpeg\r\n");
				break;
			case PNG_KEY:
				std::strcat(response, "Content-Type: image/png\r\n");
				break;
			case GIF_KEY:
				std::strcat(response, "Content-Type: image/gif\r\n");
				break;
			case SVG_KEY:
				std::strcat(response, "Content-Type: image/svg+xml\r\n");	
				break;
			case ICO_KEY:
				std::strcat(response, "Content-Type: image/x-icon\r\n");	
				break;
			case PDF_KEY:
				std::strcat(response, "Content-Type: application/pdf\r\n");	
				break;
			case ZIP_KEY:
				std::strcat(response, "Content-Type: application/zip\r\n");	
				break;
			case MP4_KEY:
				std::strcat(response, "Content-Type: video/mp4\r\n");
				break;
			case WEBM_KEY:
				std::strcat(response, "Content-Type: video/webm\r\n");
				break;
			case MP3_KEY:
				std::strcat(response, "Content-Type: audio/mpeg\r\n");
				break;
			case WAV_KEY:
				std::strcat(response, "Content-Type: audio/wav\r\n");
				break;
			default: //extension inconnue. 
				std::strcat(response, "Content-Type: application/octet-stream\r\n"); //Le client telecharge le fichier.
		}
	}
	else //pas d'extension.
		std::strcat(response, "Content-Type: application/octet-stream\r\n"); //Le client telecharge le fichier.
	std::strcat(response, "\r\n");
}

void Server::respond(const char *path, int client_socket_fd, int file_size)
{
	int response_len = std::strlen(response), file_to_send_fd, total_sent_bytes, sent_bytes, read_bytes, i;

	//Ouverture du fichier a envoyer:
	file_to_send_fd = open(path, O_RDONLY);
	if (file_to_send_fd < 0)
	{
		printf("Error: unable to open %s\n", path);
		return; //error
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

std::string generateAutoIndex(const std::string &path)
{
	DIR *dir;
	struct dirent *ent;
	struct stat st;
	std::ostringstream oss;

	if ((dir = opendir(path.c_str())) != NULL)
	{
		oss << "<html><head><title>Index of " << path << "</title></head><body>";
		oss << "<h1>Index of " << path << "</h1><ul>";
		while ((ent = readdir(dir)) != NULL)
		{
			std::string fullPath = path + "/" + ent->d_name;
			stat(fullPath.c_str(), &st);
			if (S_ISDIR(st.st_mode))
			{
				oss << "<li><a href=\"" << ent->d_name << "/\">" << ent->d_name << "/</a></li>";
			}
			else
				oss << "<li><a href=\"" << ent->d_name << "\">" << ent->d_name << "</a></li>";
		}
		oss << "</ul></body></html>";
		closedir(dir);
	}
	else {
		perror("opendir");
		return ""; // error ????
	}
	return oss.str();
}

void Server::get_methode(int comm_socket_fd)
{
	char body_size[14];
	int result;
	struct stat st;

	concatPath();
	result = stat(path, &st);
	if (result == 0 && (st.st_mode & S_IFMT) == S_IFDIR && temp.autoindex) //dossier existe && autoindex on
	{
		std::string autoIndexPage = generateAutoIndex(path);
		if (!autoIndexPage.empty())
		{
			std::strcat(path, ".html");
			fillHeader(OK, path, body_size, autoIndexPage.size());
			send(comm_socket_fd, response, strlen(response), 0);
			send(comm_socket_fd, autoIndexPage.c_str(), autoIndexPage.size(), 0);
		}
		else
			sendErr(comm_socket_fd, "404");
	}
	else if (result == 0 && (st.st_mode & S_IFMT) == S_IFREG) //fichier existe
	{
		fillHeader(OK, path, body_size, st.st_size);
		respond(path, comm_socket_fd, st.st_size);
	}
	else if (stat(temp.index, &st) == 0) //index existe
	{
		fillHeader(OK, temp.index, body_size, st.st_size);
		respond(temp.index, comm_socket_fd, st.st_size);
	}
	else //index was a lie.
		sendErr(comm_socket_fd, "404");
}

void Server::post_methode(char *header_end, int comm_socket_fd, int body_chunk_size)
{
	const char *success_message = "HTTP/1.1 200 OK\nContent-Length: 0\n\n";
	int file_fd, file_size, recv_bytes, wrote_bytes, i, total_recv_bytes;
	char body_size[14];
	struct stat st;

	//Trouver le chemin du fichier.
	concatPath();
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
printf("POST DEBUG body_chunk_size = %d\n",body_chunk_size);
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
	send(comm_socket_fd, success_message, strlen(success_message), 0);
}

void Server::delete_methode(int comm_socket_fd)
{
	char body_size[14], path[PATH_MAX];

	concatPath();
	if (remove(path) == 0)
	{
		// Fichier supprimé avec succès
		send(comm_socket_fd, OK, strlen(OK), 0);
	}
	else
	{
		// Échec de la suppression du fichier
        sendErr(comm_socket_fd, "404");//error 404
	}
}
