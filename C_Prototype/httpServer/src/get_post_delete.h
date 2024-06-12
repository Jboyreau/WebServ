#ifndef GETPOSTDELETE_H
#define GETPOSTDELETE_H
char what_methode(char *request);
void get_methode(char* header_body, char *request, int comm_socket_fd);
void post_methode(char* header_body, char *request, int comm_socket_fd);
void delete_methode(char* header_body, char *request, int comm_socket_fd);
void error_methode(char* header_body, char *request, int comm_socket_fd);
#endif
