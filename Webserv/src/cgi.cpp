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

void Server::exec_CGI(char *request, const char *path,int size_body, std::string methode)
{
    static const char *args[] = { "/usr/bin/php-cgi", path, NULL };
	
	std::string scriptPath = path;
	std::vector<std::string> env;
	std::vector<char*> CGIEnv;
	std::string request_string = request;
	std::string stg_sizebody = int_to_string(size_body);
	
	//path = "/CGI/script.php?name=nassim&id=325435435";

 	env.push_back("REQUEST_METHOD=" + methode);
    env.push_back("QUERY_STRING=" + parseQueryString(scriptPath));
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("REDIRECT_STATUS=1");

	if (methode == "POST")
	{
		env.push_back("CONTENT_LENGTH=" + stg_sizebody);
    	env.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
	}
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("PATH_INFO=" + scriptPath);
    env.push_back("REMOTE_ADDR=");
    env.push_back("SERVER_PORT=");
    env.push_back("SERVER_NAME=WEBSERV");
    env.push_back("SERVER_SOFTWARE=WEBSERV/1.0");


	for (std::vector<std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
        CGIEnv.push_back(const_cast<char*>(it->c_str()));
	
	CGIEnv.push_back(NULL);

    if (execve(args[0], const_cast<char* const*>(args), CGIEnv.data()) == -1)
    {
        perror("execve failed");
    }
}

bool set_up_CGI_file(char *CGIbodypath)
{
    int fd = open(CGIbodypath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

	if (fd == -1)
	{
        std::cerr << "Error opening CGI file" << std::endl;
        return false;
    }

    dup2(fd, 1);
	return true;
}


int Server::manage_CGI(char *request, char *scriptPath, char *CGIbodypath, char *header_end, int body_chunk_size)
{
    pid_t pid;
 	int cgi_input[2];
    int cgi_output[2];
    int file_fd, file_size, recv_bytes, wrote_bytes, i, j, total_recv_bytes, total_wrote_bytes;

    
    file_size = get_fsize(request, comm_socket_fd);
    std::string methode;
    for (int i = 0; request[i] != ' '; i++)
    {
        methode += request[i];
    }
	
	pipe(cgi_input);
    pipe(cgi_output);
	pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return -1;
    }
    else if (pid == 0)
    {
        dup2(cgi_output[1], STDOUT_FILENO);
        dup2(cgi_input[0], STDIN_FILENO);
        close(cgi_output[0]);
        close(cgi_output[1]);
        close(cgi_input[0]);
        close(cgi_input[1]);
		exec_CGI(request,scriptPath,file_size, methode);
	    exit(EXIT_FAILURE);
    }
    else
    {
        close(cgi_output[1]);
		close(cgi_input[0]);
        if (methode == "POST")
        {
        if (body_chunk_size > 0)
	    {
		recv_bytes = write(cgi_input[1], header_end, body_chunk_size);
		if (recv_bytes < 1)
		{
            std::cout << "1" << std::endl;
			sendErr(comm_socket_fd, "500");
			return -1;
		}
		file_size -= recv_bytes;
	}
    	total_recv_bytes = 0;
	i = 0;
    printf("POST DEBUG file_size = %d\n", file_size);
	//Chargement du buffer avec recv.
    while (total_recv_bytes < file_size && i < TIMEOUT)
    {
		recv_bytes = recv(comm_socket_fd, body + total_recv_bytes, file_size, 0);
		//Ecriture du buffer content avec write()
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
        std::cout << "2" << std::endl;
		sendErr(comm_socket_fd, "500");
		return -1;
	}
        }
     
        close(cgi_input[1]);

        int status;
        
        waitpid(pid, &status, 0);
    }
	return cgi_output[0];

}


// Helper function to read all data from a file descriptor
void Server::readFromFileDescriptor(int fd_file, int *readbytes) {
    ssize_t bytesRead = 0;

    while ((bytesRead = read(fd_file, body + bytesRead, BUFFER_SIZE << 3)) > 0) {
        *readbytes += bytesRead;
    }
    if (bytesRead == -1)
    {
      std::cout << "3" << std::endl;
        sendErr(comm_socket_fd, "500");
    }
    close (fd_file);
}

///////// TRANF LE FILE CONT DANS BODY //////////

// Function to split the CGI response into HTTP header and body
void Server::fillBody(int fd_file) {

    int readbytes = 0;
    readFromFileDescriptor(fd_file, &readbytes);

    // Find the position of the double CRLF which separates header and body
    header_size = ((size_t)(std::strstr(body,"\r\n\r\n") + 4));
    std::cout << "header size == " << header_size << std::endl;
    header_size -= ((size_t)body);
     std::cout << "header size 2 == " << header_size << std::endl;

    if (header_size == -((size_t)body)) 
    {
        std::cerr << "Invalid CGI response: No header-body separator found" << std::endl;
        return ;
    }
    size_body = readbytes - header_size;
    std::cout << RED << "******* INFO SIZE ***********\n" << "size body = " << size_body << "\nreadbytes = " << readbytes << "\nheader_size = " << header_size << std::endl;
  

    return ;
}


void Server::fill_header_cgi(char *body_size)
{
	const char *ext;
	int key;

	strcpy(response, "HTTP/1.1 200 OK\r\n");
	strcat(response, "Server: My Personal HTTP Server\r\n");
	strcat(response, "Content-Length: ");
	itoa(size_body, body_size);
	strcat(response, body_size);
	strcat(response, "\r\n");
	strcat(response, "Connection: close\r\n");
	// DÃ©terminer le type de contenu en fonction de l'extension du fichier.	
    strncat(response, body, header_size);
}

static void feed_path(char *request, char *path)
{
    int i, j = 1;

    *path = '.';
    // Find the start of the path in the request
    for (i = 0; *(request + i) != '\n' && *(request + i) != ' ' && *(request + i); ++i)
        ;
    // Extract the path from the request until space, newline or ?
    for (; *(request + i + j) != '\n' && *(request + i + j) != ' ' && *(request + i + j) != '?' && *(request + i + j); ++j)
        *(path + j) = *(request + i + j);
    *(path + j) = 0;

    printf("FIND PATH DEBUG! EXTRACTED PATH = %s\n", path);
}

void Server::respond_cgi()
{
	int header_len = strlen(response), file_to_send_fd, total_sent_bytes, sent_bytes, bytes_chunk_size, read_bytes, i;

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

printf("RESPOND DEBUG: total_read_bytes = %d\n", total_sent_bytes);
	i = 0;
	sent_bytes = 0;
	total_sent_bytes = 0;
  
	while (total_sent_bytes < size_body && i < TIMEOUT)
	{
		sent_bytes = send(comm_socket_fd, body_cgi + total_sent_bytes, size_body, 0); //Envoie du chunk.
		total_sent_bytes += sent_bytes * (sent_bytes > 0);
		++i;
	}
printf("RESPOND DEBUG: total_sent_bytes = %d\n", total_sent_bytes);
	close(file_to_send_fd);
	memset(body, 0, total_sent_bytes);
}


void Server::methode_CGI(char *header_end, int body_chunk_size)
{
	char body_size[14], scriptPath[PATH_MAX];
	char CGIBodyPath[] = "./cgibody";

	struct stat st;
/*1.Parsing du file path*/
	feed_path(request, scriptPath);
    if (stat(scriptPath, &st) == -1)
    {
        sendErr(comm_socket_fd,"404");
        return;
    }
	int fd_cgi = manage_CGI(request, scriptPath, CGIBodyPath, header_end, body_chunk_size);
    if (fd_cgi == -1)
        return;
    fillBody(fd_cgi);
    	
/*2.Remplir le header puis L'envoyer avec le body*/
		fill_header_cgi(body_size);
        std::cout << GREEN << "\n####Fill header cgi result ####:\n" << response << RESET << std::endl;
		respond_cgi();
		return;
	

	send(comm_socket_fd, "", 0, 0);
}