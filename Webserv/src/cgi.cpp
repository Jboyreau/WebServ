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
#include <sys/wait.h>
#include <sstream>
#include <string>
#include <time.h>
#include "ServerUtils.h"

#define DEFAULT_PATH "./default/default.html"
#define POST_PATH "./media/gif/upload.gif"
#define MAX_FILE_SIZE 0x0FFFFFFF

#define BUFFER_SIZE 1024



static int indice_body_request(char **request)
{
    char *ptr = *request;
    int i = 0;

    while (*ptr + i)
    {
        if (strncmp(ptr + i, "\r\n\r\n", 4) == 0)
		{
            return i + 4;
        }
        ++i;
    }
	return i + 4;
}

std::string parseQueryString(const std::string& path)
{
	std::string queryString;

	size_t pos = path.find('?');

	if (pos != std::string::npos)
	{
		queryString = path.substr(pos + 1);
	}
	return queryString;
}

std::string int_to_string(int nb)
{
    std::stringstream ss;
    ss << nb;
    std::string str = ss.str();
    return str; // Add this line to return the string
}

void Server::exec_CGI(char *request, const std::string scriptPath, int size_body, std::string &methode)
{
    static const char *args[] = { temp.cgi_path, scriptPath.c_str(), NULL };
	
    //"/usr/bin/php-cgi"
	std::vector<std::string> env;
	std::vector<char*> CGIEnv;
	std::string request_string = request;
	std::string stg_sizebody = int_to_string(size_body);
	
	//path = "/CGI/script.php?name=nassim&id=325435435";

 	env.push_back("REQUEST_METHOD=" + methode);
    env.push_back("QUERY_STRING=" + parseQueryString(path));
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("REDIRECT_STATUS=200");

	if (methode == "POST")
	{
		env.push_back("CONTENT_LENGTH=" + stg_sizebody);
    	env.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
	}
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("PATH_INFO=" + scriptPath);
    //env.push_back("REMOTE_ADDR=");
    //env.push_back("SERVER_PORT=");
    env.push_back("SERVER_NAME=WEBSERV");
    env.push_back("SERVER_SOFTWARE=WEBSERV/1.0");
	for (std::vector<std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
        CGIEnv.push_back(const_cast<char*>(it->c_str()));
	
	CGIEnv.push_back(NULL);

    if (execve(args[0], const_cast<char* const*>(args), CGIEnv.data()) == -1)
    {
        perror("execve failed");
		exit(EXIT_FAILURE);
    }
}

int Server::manage_CGI(char *request, std::string scriptPath, char *CGIbodypath, char *header_end, int body_chunk_size, std::string &methode)
{
	pid_t pid;
	int file_fd, recv_bytes, wrote_bytes, i, j, total_recv_bytes, total_wrote_bytes;
	int file_size = 0;

	if (methode == "POST")
		file_size = get_fsize(request, comm_socket_fd);
	if (pipe(cgi_input) == -1 || pipe(cgi_output) == -1)
	{
		std::cerr << RED << "Error : pipe fail "  << RESET << std::endl;
		closePipe(cgi_output);
		closePipe(cgi_input);
		sendErr(comm_socket_fd, "500");
		return -1;
	}
	pid = fork();
	if (pid == -1)
	{
		std::cerr << RED << "Error : fork fail "  << RESET << std::endl;
		closePipe(cgi_output);
		closePipe(cgi_input);
		sendErr(comm_socket_fd, "500");
		return -1;
	}
	else if (pid == 0)
	{
		dup2(cgi_output[1], STDOUT_FILENO);
		dup2(cgi_input[0], STDIN_FILENO);
		closePipe(cgi_output);
		closePipe(cgi_input);
		exec_CGI(request,scriptPath,file_size, methode);
		exit(EXIT_FAILURE);
	}
	else
	{
		if (methode == "POST")
		{
			for (int k = 0; k < body_chunk_size ; ++k)
				*(body + k) = *(header_end + k);
			total_recv_bytes = 0;
			i = 0;
			//Chargement du buffer avec recv.
			bool b = false;
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
					wrote_bytes = write(cgi_input[1], body + total_recv_bytes + total_wrote_bytes, recv_bytes);
					total_wrote_bytes += wrote_bytes * (wrote_bytes > 0);
					++j;
				}
				total_recv_bytes += recv_bytes * (recv_bytes > 0);
				++i;
			}
			if (total_recv_bytes < file_size)
			{
				std::cerr << RED << "Error : not all recv"  << RESET << std::endl;
				closePipe(cgi_output);
				closePipe(cgi_input);
				sendErr(comm_socket_fd, "500");
				return -1;
			}
		}
		closePipe(cgi_input);
		close(cgi_output[1]);
		cgi_output[1] = -1;
		int status, time = 0, timeout = 10000000;
		for (; time < timeout; ++time)
		{
			pid_t t = waitpid(pid, &status, WNOHANG);
			if (t > 0)
				break;
		}
		if (time == timeout)
		{
			std::cerr << RED << "Error : Timeout exec cgi"  << RESET << std::endl;
			kill(pid, SIGKILL);
			sendErr(comm_socket_fd,"504");
			closePipe(cgi_output);
			closePipe(cgi_input);
			return -1;
		}
		if (!WIFEXITED(status) || WEXITSTATUS(status) == EXIT_FAILURE)
		{
			std::cerr << RED << "Error : execve fail"  << RESET << std::endl;
			closePipe(cgi_output);
			closePipe(cgi_input);
			sendErr(comm_socket_fd, "500");
			return -1;
		}
	}
	return cgi_output[0];
}


// Helper function to read all data from a file descriptor
int Server::readFromFileDescriptor(int fd_file, int *readbytes) {
    ssize_t bytesRead = 0;

    while ((bytesRead = read(fd_file, body + bytesRead, BUFFER_SIZE << 3)) > 0) {
        *readbytes += bytesRead;
    }
    if (bytesRead == -1)
    {
		std::cerr << RED << "Error : read error" << RESET << std::endl;
        sendErr(comm_socket_fd, "500");
		closePipe(cgi_output);
		return (1);
    }
	closePipe(cgi_output);
	return (0);
}

///////// TRANF LE FILE CONT DANS BODY //////////

// Function to split the CGI response into HTTP header and body
int Server::fillBody(int fd_file) {
    int readbytes = 0;

    if (readFromFileDescriptor(fd_file, &readbytes))
		return (-1);
    // Find the position of the double CRLF which separates header and body
    header_size = ((size_t)(std::strstr(body,"\r\n\r\n") + 4));
    header_size -= ((size_t)body);
    if (header_size == -((size_t)body)) 
    {
        std::cerr << RED << "Error : wrong header."<< RESET << std::endl;
		sendErr(comm_socket_fd, "400");
		closePipe(cgi_output);
        return (-1);
    }
    size_body = readbytes - header_size;
    return (0);
}


void Server::fill_header_cgi(char *body_size, const char* first_value)
{
	const char *ext;
	int key;
	std::strcpy(response, first_value);
	std::strcat(response, "Server: My Personal HTTP Server\r\n");
	std::strcat(response, "Content-Length: ");
	itoa(size_body, body_size);
	std::strcat(response, body_size);
	std::strcat(response, "\r\n");
	std::strcat(response, "Connection: close\r\n");
	// DÃ©terminer le type de contenu en fonction de l'extension du fichier.	
    strncat(response, body, header_size);
}

void Server::clean_path(std::string &request)
{
    int i = 0; 
    int j = 0;

    for (; *(path + i + j) != '\n' && *(path + i + j) != ' ' && *(path + i + j) != '?' && *(path + i + j); ++j)
        request += *(path + i + j);
    request += "\0";
}

void Server::respond_cgi()
{
	int header_len = std::strlen(response), file_to_send_fd, total_sent_bytes, sent_bytes, bytes_chunk_size, read_bytes, i;

	//Ouverture du fichier a envoyer:
	//Envoi du header
	//Verification des valeurs de retour de send() pour eviter les infinites loop en cas de spam.
	total_sent_bytes = 0;
	sent_bytes = 1;
	while (total_sent_bytes < header_len && sent_bytes > 0)
	{
		sent_bytes = send(comm_socket_fd, response + total_sent_bytes, header_len - total_sent_bytes, 0); //Envoie du chunk.
		total_sent_bytes += sent_bytes;
	}
    char *body_cgi = body + header_size;
	i = 0;
	sent_bytes = 0;
	total_sent_bytes = 0;
  
	while (total_sent_bytes < size_body && i < TIMEOUT)
	{
		sent_bytes = send(comm_socket_fd, body_cgi + total_sent_bytes, size_body, 0); //Envoie du chunk.
		total_sent_bytes += sent_bytes * (sent_bytes > 0);
		++i;
	}
	closePipe(cgi_output);
	memset(body, 0, total_sent_bytes);
}


void Server::methode_CGI(char *header_end, int body_chunk_size, std::string &methode)
{
	char body_size[14];
	char CGIBodyPath[] = "./cgibody";
    std::string scriptPath;
	struct stat st;
/*1.Parsing du file path*/
    const char *msg = concatPath();

    if ((*request + *(request + 1)) == GET)
        clean_path(scriptPath);
    else
        scriptPath = path;
    if (stat(scriptPath.c_str(), &st) == -1)
    {
		std::cerr << RED << "Error : is not a file."  << RESET << std::endl;
        sendErr(comm_socket_fd,"404");
        return;
    }
	int fd_cgi = manage_CGI(request, scriptPath, CGIBodyPath, header_end, body_chunk_size, methode);
    if (fd_cgi == -1)
        return;
    if (fillBody(fd_cgi) == -1)
		return ;
/*2.Remplir le header puis L'envoyer avec le body*/
		fill_header_cgi(body_size, msg);
		respond_cgi();
		return;
	send(comm_socket_fd, "", 0, 0);
}
