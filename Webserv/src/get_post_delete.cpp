#include <string.h> //memeset(); strcpy; strcat();
#include <limits.h> //PATH_MAX
#include <stdlib.h> //
#include <stdio.h> //
#include <unistd.h> 
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

	if (!content_length_str)
	{
		// Error: No Content-Length found
		std::cerr << RED << "Error : no content-length."  << RESET << std::endl;
		sendErr(comm_socket_fd, "411");
		return -1;
	}
	content_length_str += 16;
	len = atoi(content_length_str);
	if (len > conf.max_body_size)
	{
		std::cerr << RED << "Error : wrong size."  << RESET << std::endl;
		sendErr(comm_socket_fd, "413");
		return -1;
	}
	return len;
}

const char *Server::concatPath(void)
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
		return OK;
	}
	ptr = temp.ret;
	if (*ptr)
		ptr = temp.ret + 1;
	for(i = 0; *(ptr + i) != '/' && *(ptr + i); ++i)
		;
	if (*(ptr + i) == '/')
		ptr = ptr + i + 1;
	std::strcpy(path, temp.root);
	std::strcat(path, ptr);
	ptr = location_key.c_str();
	for (i = 0; i < loc_len; ++i)
		;
	ptr = ptr + i;
	std::strcat(path, ptr);
	return H301;
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
	int response_len = std::strlen(response), total_sent_bytes, total_read_bytes, sent_bytes, read_bytes, i, j;

	//Ouverture du fichier a envoyer:
	opened_file = open(path, O_RDONLY);
	if (opened_file < 0)
	{
		if (errno == EACCES)
		{
			std::cerr << RED << "Error : remove access denied."  << RESET << std::endl;
			sendErr(comm_socket_fd, "403");
		}
		else
		{
			std::cerr << RED << "Error : internal server error."  << RESET << std::endl;
			sendErr(comm_socket_fd, "500");
		}
		return; //error
	}
	//Envoi du header.
	total_sent_bytes = 0;
	sent_bytes = 1;
	i = 0;
	while (total_sent_bytes < response_len && i < TIMEOUT)
	{
		//Envoie du chunk.
		sent_bytes = send(client_socket_fd, response + total_sent_bytes, response_len - total_sent_bytes, 0);
		total_sent_bytes += sent_bytes;
		++i;
	}
	//Envoi du fichier
	read_bytes = 0;
	total_read_bytes = 0;
	i = 0;
	while (total_read_bytes < file_size && i < TIMEOUT)
	{
		//Chargement du chunk dans le buffer.
		read_bytes = read(opened_file, body + total_read_bytes, file_size);
		j = 0;
		total_sent_bytes = 0;
		while (total_sent_bytes < read_bytes && j < TIMEOUT)
		{
			sent_bytes = send(client_socket_fd, body + total_read_bytes + total_sent_bytes, file_size, 0); //Envoie du chunk.
			total_sent_bytes += sent_bytes * (sent_bytes > 0);
			++j;
		}
		total_read_bytes += read_bytes * (read_bytes > 0);
		++i;
	}
	if (total_read_bytes < file_size)
	{
		std::cerr << RED << "Error : read."  << RESET << std::endl;
		close(opened_file);
		opened_file = -1;
		return;
	}
	close(opened_file);
	opened_file = -1;
}

std::string Server::generateAutoIndex(const std::string &path)
{
	struct dirent *ent;
	struct stat st;
	std::ostringstream oss;

	directory = NULL;
	if ((directory = opendir(path.c_str())) != NULL)
	{
		oss << "<html><head><title>Index of " << path << "</title></head><body>";
		oss << "<h1>Index of " << path << "</h1><ul>";
		while ((ent = readdir(directory)) != NULL)
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
		closedir(directory);
		directory = NULL;
	}
	else
	{
		perror("opendir");
		return "";
	}
	return oss.str();
}

void Server::get_methode(int comm_socket_fd)
{
	const char *msg;
	char body_size[14];
	int result;
	struct stat st;

	msg = concatPath();
	result = stat(path, &st);
	if (result == 0 && (st.st_mode & S_IFMT) == S_IFDIR && temp.autoindex && dir) //dossier existe && autoindex on
	{
		std::string autoIndexPage = generateAutoIndex(path);
		if (!autoIndexPage.empty())
		{
			std::strcat(path, ".html");
			fillHeader(msg, path, body_size, autoIndexPage.size());
			int len = std::strlen(response), sent_bytes;
			int i = 0;
			int total_sent_bytes; 
			for (total_sent_bytes = 0; total_sent_bytes < len && i < TIMEOUT; total_sent_bytes += sent_bytes * (sent_bytes > 0))
			{
				sent_bytes = send(comm_socket_fd, response, len, 0);
				++i;
			}
			len = autoIndexPage.size();
			i = 0;
			for (total_sent_bytes = 0; total_sent_bytes < len && i < TIMEOUT; total_sent_bytes += sent_bytes * (sent_bytes > 0))
			{
				sent_bytes = send(comm_socket_fd, autoIndexPage.c_str(), autoIndexPage.size(), 0);
				++i;
			}
		}
		else
		{
			std::cerr << RED << "Error : open dir failed."  << RESET << std::endl;
        	sendErr(comm_socket_fd, "500");
		}
	}
	else if (result == 0 && (st.st_mode & S_IFMT) == S_IFREG) //fichier existe
	{
		fillHeader(msg, path, body_size, st.st_size);
		respond(path, comm_socket_fd, st.st_size);
	}
	else if (stat(temp.index, &st) == 0) //index existe
	{
		fillHeader(msg, temp.index, body_size, st.st_size);
		respond(temp.index, comm_socket_fd, st.st_size);
	}
	else //index was a lie.
	{
		std::cerr << RED << "Error : index doesn't exist."  << RESET << std::endl;
		sendErr(comm_socket_fd, "404");
	}
	dir = 0;
}

void Server::post_methode(char *header_end, int comm_socket_fd, int body_chunk_size)
{
	const char *msg;
	int file_fd, file_size, recv_bytes, wrote_bytes, i, j, total_recv_bytes, total_wrote_bytes;
	char body_size[14];
	struct stat st;

	//Trouver le chemin du fichier.
	msg = concatPath();
	//Trouver la taille du fichier.
	file_size = get_fsize(request, comm_socket_fd);
	if (file_size < 0)
		return ;
	opened_file = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (opened_file < 0)
    {
		if (errno == EACCES)
		{
			std::cerr << RED << "Error : access denied."  << RESET << std::endl;
			sendErr(comm_socket_fd, "403");
		}
		else
		{
			std::cerr << RED << "Error : open failed."  << RESET << std::endl;
			sendErr(comm_socket_fd, "500");
		}
        return;
    }
	//Ecrire le morceau de body en trop.
	for (int k = 0; k < body_chunk_size; ++k)
		*(body + k) = *(header_end + k);
	total_recv_bytes = 0;
	i = 0;
	bool b = false;
	//Chargement du buffer avec recv.
    while (total_recv_bytes < file_size && i < TIMEOUT)
    {
		if (b)
		{
			recv_bytes = recv(comm_socket_fd, body + total_recv_bytes, file_size, 0);
		}
		else
		{
			recv_bytes = body_chunk_size;
			b = true;
		}
		//Ecriture du buffer content.
		j = 0;
		total_wrote_bytes = 0;
		while (total_wrote_bytes < recv_bytes && j < TIMEOUT)
		{
			wrote_bytes = write(opened_file, body + total_recv_bytes + total_wrote_bytes, recv_bytes);
			total_wrote_bytes += wrote_bytes * (wrote_bytes > 0);
				++j;
		}
		total_recv_bytes += recv_bytes * (recv_bytes > 0);
		++i;
    }
	if (total_recv_bytes < file_size)
	{
		std::cerr << RED << "Error : recv failed."  << RESET << std::endl;
		sendErr(comm_socket_fd, "500");
		close(opened_file);
		opened_file = -1;
		return;
	}
    close(opened_file);
	opened_file = -1;
	send(comm_socket_fd, msg, strlen(msg), 0);
}

void Server::delete_methode(int comm_socket_fd)
{
	char body_size[14], path[PATH_MAX];

	concatPath();
	if (std::remove(path) == 0)
	{
		// Fichier supprimé avec succès
		send(comm_socket_fd, OK, strlen(OK), 0);
	}
	else
	{
		if (errno == EACCES)
		{
			std::cerr << RED << "Error : remove access denied."  << RESET << std::endl;
			sendErr(comm_socket_fd, "403");
		}
		else
		{
			std::cerr << RED << "Error : remove."  << RESET << std::endl;
			sendErr(comm_socket_fd, "500");
		}
	}
}
