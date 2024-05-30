#include "ws.h"

void cnf_parsing(char *path, std::unordered_map<std::string, ServerConfig_t>& server_configs)
{
/*1.Charger le contenu du fichier cnf dans un buffer*/
	(void)path;
	// Exemple d'ajout de la première configuration de serveur
	ServerConfig_t server1;

	server1.listen_port = 2000;
	server1.host = "127.0.0.1";
	server1.server_names = "locahost";
	server1.error_page = "/custom_404.html";
	server1.client_max_body_size = 1;
	{
		Location_t location1;
		location1.limit_except = "POST";
		location1.return_path = "/new-path";
		location1.root = "./share";
		location1.autoindex = true;
		location1.index_file = "index.html";
		location1.cgi_path = "./bin/php";
	}
	server1.location_path["/storage"] = location1;

	// Exemple d'ajout de la deuxième configuration de serveur
	ServerConfig_t server2;
	server2.listen_port = 2020;
	server2.host = "127.0.0.1";
	server2.server_names = "locahost";
	server2.error_page = "/custom_404.html";
	server2.client_max_body_size = 5;
	{
		Location_t location2;
		location2.limit_except = "GET";
		location2.return_path = "/new-path";
		location2.root = "./share";
		location2.autoindex = true;
		location2.index_file = "index.html";
		location2.cgi_path = "./bin/php";
	}
	server2.location_path["/img"] = location2;

/*2.Peupler la map server_configs avec les pairs IP/Port*/
	
	// Ajouter la configuration à la map
	std::string key1 = "localhost127.0.0.1:2000";
		//permet d'utiliser directement la ligne HOST du header http comme cle.
	server_configs[key1] = server1;

	// Ajouter la configuration à la map
	std::string key2 = "localhost127.0.0.1.4:2020";
		//permet d'utiliser directement la ligne HOST du header http comme cle.
	server_configs[key2] = server2;
}
