#include "ws.h"

void cnf_parsing(char *path, std::unordered_map<std::string, ServerConfig>& server_configs)
{
/*1.Charger le contenu du fichier cnf dans un buffer*/
	(void)path;
	// Exemple d'ajout de la première configuration de serveur
	ServerConfig server1;
	server1.listen_port = 2000;
	server1.host = "127.0.0.1";
	server1.server_names = {"locahost"};
	server1.error_page = "/custom_404.html";
	server1.client_max_body_size = 1;
	server1.location_path["/index.php"] = "/index.php";
	server1.limit_except = "POST";
	server1.return_path = "/new-path";
	server1.root = "./share/nginx/html";
	server1.autoindex = true;
	server1.index_file = "index.html";
	server1.cgi_path = "./bin/php";

	// Exemple d'ajout de la deuxième configuration de serveur
	ServerConfig server2;
	server2.listen_port = 2020;
	server2.host = "127.0.0.1";
	server2.server_names = {"locahost"};
	server2.error_page = "/custom_404.html";
	server2.client_max_body_size = 5;
	server1.location_path["/index.php"] = "/index.php";
	server2.limit_except = "GET";
	server2.return_path = "/new-path";
	server2.root = "./share/nginx/html";
	server2.autoindex = true;
	server2.index_file = "index.html";
	server2.cgi_path = "./bin/php";

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
