#include "ws.h"

int main(void)
{
	/*1.Initialisation*/
 	std::unordered_map<std::string, ServerConfig_t> server_configs;
	/*2.Parsing du fichier cnf*/
	cnf_parsing(CNF_PATH, server_configs);
	/*3.Setup/Boucle de communication*/
	if (SetupCommunicationTcpServer(server_configs))
		return EXIT_FAILURE;
	return 0;
}
