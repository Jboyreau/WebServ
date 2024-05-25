#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h> //type fd_set;
#include <sys/socket.h> //struct sockaddr_in; AF_INET; SOL_SOCKET; SO_REUSEADDR; inet_ntoa();
#include <netinet/in.h> //IPPROTO_TCP;
#include <arpa/inet.h>	//ntohs;
#include <string.h> //memeset();
#include <netdb.h> //gethostbyname();
#include <memory.h>
#include <errno.h>
#include "data_format.h"

#define DEST_PORT 2000
#define SERVER_IP_ADDRESS "127.0.0.1"

void setup_tcp_communication(void)
{
	/*1.Initialisation/declaration*/
	int sockfd = 0, sent_recv_bytes = 0, addr_len = sizeof(struct sockaddr),
	client_data_len = sizeof(operand_t), server_data_len = sizeof(result_t);
	operand_t client_data;
	result_t server_data;
	/*2.Specification des infos server*/
	struct sockaddr_in dest;
	dest.sin_family = AF_INET;
	dest.sin_port = htons(DEST_PORT);
	struct hostent* host = (struct hostent*)gethostbyname(SERVER_IP_ADDRESS);
	dest.sin_addr = *((struct in_addr*)((*host).h_addr));
	/*3.Creer socket TCP*/
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	/*3.5 Requete de connexion*/
	connect(sockfd, (struct sockaddr*)&dest, addr_len);
	/*4.SEND USER INPUT*/
PROMPT_USER:
	printf("ENTER a : ");
	scanf("%u", &client_data.a);
	putchar('\n');
	printf("ENTER b : ");
	scanf("%u", &client_data.b);
	putchar('\n');
	/*5.Envoyer la data au serveur*/
	sent_recv_bytes = sendto(sockfd, &client_data, client_data_len, 0,
	(struct sockaddr*)&dest, sizeof(struct sockaddr));
	printf("No of bytes sent = %d\n", sent_recv_bytes);
	/*6.Recevoir la reponse*/
	sent_recv_bytes = recvfrom(sockfd, &server_data, server_data_len, 0,
	(struct sockaddr*)&dest, &addr_len);
	printf("No of bytes recvd = %d\n", sent_recv_bytes);
	printf("Result recvd = %u\n", server_data.c);
	if (sent_recv_bytes == 0)
		return;
	/*7.goto step 4*/
	goto PROMPT_USER;
}

int main(void)
{
	setup_tcp_communication();
	printf("TCPClient quits.\n");
	return 0;
}
