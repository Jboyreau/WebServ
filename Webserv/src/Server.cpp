#include "Server.h"

Server::Server(void)
{
	if (parsing() == false)
		exit(EXIT_FAILURE);
	for (int i = 0; i < MAX_VSERVER; ++i)
		virtual_servers[i] = NULL;
	body = new char [MAX_BODY_SIZE];
	timeout.tv_sec = SELECT_TIMEOUT_SEC;
	timeout.tv_usec = SELECT_TIMEOUT_USEC;
	opt = 1;
}

Server::~Server(void)
{
	if (body)
		delete[] body;
	for (int i = 0; i < MAX_VSERVER; ++i)
		if (virtual_servers[i])
			delete[] virtual_servers[i];
}

bool Server::parsing(void)
{
	int line = 1;
	bool parsing_result = false;
	char *file_content;
	std::vector<t_token> token_liste;
	std::vector<t_token>::iterator it;

	file_content = loadFileToBuffer(CNF_PATH);
	token_liste = tokenizer(file_content);
//Appel de ruleServer() pour traiter plusieurs blocs.
	it = token_liste.begin();
	server_index = 0;
	while (server_index < MAX_VSERVER)
	{
std::cout << YELLOW << "DEBUG : server_index = " << server_index << '.' << RESET << std::endl;
		if (it != token_liste.end())
		{
			if ((*it).type == SERVER)
			{
				//TODO:Definir des valeur par default pour cnf[i].
				std::cout << GREEN << "VIRTUAL SERVER " << server_index << " CONFIGURATION : " << std::endl;
				parsing_result = ruleServer(token_liste, it, line);
			}
			else
				break;
		}
		else
			break;
		if (parsing_result == false)
			break;
		++server_index;
	}
	if (server_index >= MAX_VSERVER)
	{
		parsing_result = false;
		std::cerr << RED << "Error Config : more than " << MAX_VSERVER << '.' << RESET << std::endl;
	}
std::cout << YELLOW << "DEBUG : parsing_result = " << parsing_result << '.' << RESET << std::endl;
	delete[] file_content;
	return parsing_result;
}

/*
Conventions du parsing:
	Les mots en maj tels que SERVER designent des tokens (voir ServerUtils.h->enum token_type).
	Les fonctions commencants par rule sont des regles.
*/
bool Server::ruleServer(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : SERVER END rPort() rIp() rName() rError() rLocation() SERVER ou .end()*/
	//Verification du nombre de token.
	if (std::distance(it, token_liste.end()) >= 2)
	{
		if ((*it).type == SERVER && (*(it + 1)).type == END)
		{
			//skip SERVER et END.
			it += 2;
			//increment de line.
			++line;
			//Appel des regles rPort() rIp() rName() rError() rLocation().
			if (rulePort(token_liste, it, line) == false)
				return configErr(it, line);
			if (ruleIp(token_liste, it, line) == false)
				return configErr(it, line);
			if (ruleName(token_liste, it, line) == false)
				return configErr(it, line);
			{
				std::cout << GREEN << "\tServer Names: ";
				for (std::vector<char>::iterator it = (*(cnf + server_index)).names.begin(); it != (*(cnf + server_index)).names.end(); ++it)
					std::cout << *it;
				std::cout << RESET << std::endl;
			}
			if (ruleError(token_liste, it, line) == false)
				return configErr(it, line);
			if (ruleSize(token_liste, it, line) == false)
				return configErr(it, line);
			if (ruleLocation(token_liste, it, line) == false)
				return configErr(it, line);
			//Test du token de fin de bloc, SERVER ou fin de vecteur.
			if (it == token_liste.end())
				return true;
			if ((*it).type == SERVER)
				return true;
			return printf("yo\n"),configErr(it, line);
		}
		return configErr(it, line);
	}
	std::cerr << RED << "Error Config : line " << line << "not enough tokens.";
	std::cerr << RESET << std::endl;
	return false;
}

bool Server::rulePort(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : listen word \n*/
	int port = 0;

	//Verification du nombre de token.	
	if (std::distance(it, token_liste.end()) >= 3)
	{
		if ((*it).type == PORT)
		{
			if ((*(it + 1)).type == WORD)
			{
				if ((*(it + 2)).type == END)
				{
					//Affectation du port a la strut s_config.
					if (is_valid_port_number(it + 1, port) == true)
					{
						(*(cnf + server_index)).server_addr.sin_port = port;
						std::cout << GREEN << "\tPort : " << ntohs((*(cnf + server_index)).server_addr.sin_port) << RESET << std::endl;
						//skip PORT WORD END.
						it += 3;
						//increment de line.
						++line;
						return true;
					}
					std::cerr << RED << "Error Config : line " << line << ' ';
					std::cerr.write((*(it + 1)).str, (*(it + 1)).len);
					std::cerr << " is not a port number.";
					std::cerr << RESET << std::endl;
					return false;
				}
				return configErr(it + 2, line);
			}
			return configErr(it + 1, line);
		}
		return configErr(it, line);
	}
	std::cerr << RED << "Error Config : line " << line << "not enough tokens.";
	std::cerr << RESET << std::endl;
	return false;
}

bool Server::ruleIp(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : host word \n*/
	u_int32_t ip = 0;

	//Verification du nombre de token.	
	if (std::distance(it, token_liste.end()) >= 3)
	{
		if ((*it).type == IP)
		{
			if ((*(it + 1)).type == WORD)
			{
				if ((*(it + 2)).type == END)
				{
					//Affectation du port a la strut s_config.
					if ((ip = ftInetAddr(it + 1)) != INADDR_NONE)
					{
						(*(cnf + server_index)).server_addr.sin_addr.s_addr = ip;
						std::cout << GREEN << "\tIpv4 : " << ntohl((*(cnf + server_index)).server_addr.sin_addr.s_addr) << RESET << std::endl;
						(*(cnf + server_index)).server_addr.sin_family = AF_INET;
						std::cout << GREEN << "\tFamily : " << (*(cnf + server_index)).server_addr.sin_family << RESET << std::endl;
						//skip IP WORD END
						it += 3;
						//increment de line.
						++line;
						return true;
					}
					std::cerr << RED << "Error Config : line " << line << ' ';
					std::cerr.write((*(it + 1)).str, (*(it + 1)).len);
					std::cerr << " is not an ip address.";
					std::cerr << RESET << std::endl;
					return false;
				}
				return configErr(it + 2, line);
			}
			return configErr(it + 1, line);
		}
		return configErr(it, line);
	}
	std::cerr << RED << "Error Config : line " << line << "not enough tokens.";
	std::cerr << RESET << std::endl;
	return false;
}

bool Server::ruleName(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : NAME rName_() | z*/

	//Verification du nombre de token.	
	if (std::distance(it, token_liste.end()) >= 1)
	{
		if ((*it).type == NAME)
		{
			//skip NAME.
			++it;
			return ruleName_(token_liste, it, line);
		}
		else if ((*it).type == WORD || (*it).type == PORT || (*it).type == IP || (*it).type == END)
			return configErr(it, line);
	}
	//Regle facultaive, si (distance >= 1 || token != WORD || token != PORT || token != IP || token != END), alors true.
	return true;
}

bool Server::ruleName_(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE: WORD END | word NAME_*/

	//Verification du nombre de token.
	if (std::distance(it, token_liste.end()) >= 2)
	{
		if ((*(it)).type == WORD)
		{
			if ((*(it + 1)).type == END)
			{
				//Ajout de server name dans un vecteur de char.
				for(int i = 0; i < (*it).len; ++i)
					((*(cnf + server_index)).names).push_back(*((*it).str + i));
				((*(cnf + server_index)).names).push_back(';');
				//skip WORD END.
				it += 2;
				//increment de line;
				++line;
				return true;
			}
			else if ((*(it + 1)).type == WORD)
			{
				//Ajout de server name dans un vecteur de char.
				for(int i = 0; i < (*it).len; ++i)
					((*(cnf + server_index)).names).push_back(*((*it).str + i));
				((*(cnf + server_index)).names).push_back(';');
				//skip WORD.
				++it;
				//Recursion.
				return ruleName_(token_liste, it, line);
			}
			return configErr(it + 2, line);
		}
		return configErr(it + 1, line);
	}
	std::cerr << RED << "Error Config : line " << line << "not enough tokens.";
	std::cerr << RESET << std::endl;
	return false;
}

bool Server::ruleError(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	static int error_count = 0;
    const int max_errors = 100;
	/*RULE: ERROR WORD WORD END rError() | z*/
	if (std::distance(it, token_liste.end()) >= 4)
	{
		if ((*it).type == ERR)
		{
			if (++error_count > max_errors)
			{
				std::cerr << RED << "Error Config : line " << line << "near " << ' ';
				std::cerr.write((*it).str, (*it).len);
				std::cerr << ", more than " << max_errors << " defined.";
				std::cerr << RESET << std::endl;
				return false;
			}
			if ((*(it + 1)).type == WORD)
			{
				if ((*(it + 2)).type == WORD)
				{
					if ((*(it + 3)).type == END)
					{
						std::string error_code((*(it + 1)).str, (*(it + 1)).len);
						std::string error_path((*(it + 2)).str, (*(it + 2)).len);
						// Ajout du code d'erreur et de l'URI à la map d'erreurs
						(*(cnf + server_index)).error_map[error_code] = error_path;
						std::cout << GREEN << "\tHTTP Error map Key: " << error_code << RESET << std::endl;
						std::cout << GREEN << "\tHTTP Error map Content : " << (*(cnf + server_index)).error_map[error_code] << RESET << std::endl;
						//skip ERR WORD WORD END.
						it += 4;
						//increment de line.
						++line;
						return ruleError(token_liste, it, line);
					}
					return configErr(it + 3, line);
				}
				return configErr(it + 2, line);
			}
			return configErr(it + 1, line);
		}
		else if ((*it).type == WORD || (*it).type == PORT || (*it).type == IP || (*it).type == NAME || (*it).type == END)
			return configErr(it, line);
	}
	//Regle facultaive, si (distance >= 1 || token != WORD || token != PORT || token != IP || token != NAME || token != END), alors true.
	return true;
}

bool Server::ruleSize(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : SIZE WORD END*/
	size_t size = 0;

	//Verification du nombre de token.	
	if (std::distance(it, token_liste.end()) >= 3)
	{
		if ((*it).type == SIZE)
		{
			if ((*(it + 1)).type == WORD)
			{
				if ((*(it + 2)).type == END)
				{
					for (int i = 0; i < (*(it + 1)).len; ++i)
						if (!std::isdigit(*((*(it + 1)).str + i)))
						{
							std::cerr << RED << "Error Config : line " << line << ' ';
							std::cerr.write((*(it + 1)).str, (*(it + 1)).len);
							std::cerr << " is not a digit.";
							std::cerr << RESET << std::endl;
						}
					//Affectation de la taille à cnf.
					if (((*(cnf + server_index)).max_body_size = atoi((*(it + 1)).str)) < MAX_BODY_SIZE)
					{
						std::cout << GREEN << "\tMax_body_size " << (*(cnf + server_index)).max_body_size << RESET << std::endl;
						//skip IP WORD END
						it += 3;
						//increment de line.
						++line;
						return true;
					}
					std::cerr << RED << "Error Config : line " << line << ' ';
					std::cerr.write((*(it + 1)).str, (*(it + 1)).len);
					std::cerr << " is too big.";
					std::cerr << RESET << std::endl;
					return false;
				}
				return configErr(it + 2, line);
			}
			return configErr(it + 1, line);
		}
		else if ((*it).type == WORD || (*it).type == PORT || (*it).type == IP || (*it).type == NAME || (*it).type == ERR || (*it).type == END)
			return configErr(it, line);
	}
	return true;
}

void clear_location(t_location &loc)
{
	loc.method_map.clear();
	memset(loc.root, 0, PATH_MAX);
	loc.autoindex = false;
	memset(loc.index, 0, PATH_MAX);
	memset(loc.cgi_path, 0, PATH_MAX);
	memset(loc.ret, 0, PATH_MAX);
}

bool Server::ruleLocation(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : LOC WORD END rMeth() rRoot() rAutoIndex() rIndex() rCgiPath() rReturn() rLocation() | z*/
	//Verification du nombre de token.
	clear_location(temp);
	if (std::distance(it, token_liste.end()) >= 3)
	{
		if ((*it).type == LOC && (*(it + 1)).type == WORD && (*(it + 2)).type == END)
		{
			std::string location_key((*(it + 1)).str, (*(it + 1)).len);
			//skip SERVER et END.
			it += 3;
			//increment de line.
			++line;
			//Appel des regles rMeth() rRoot() rAutoIndex() rIndex() rCgiPath() rReturn().
			if (ruleMethode(token_liste, it, line) == false)
				return false;
			if (ruleRoot(token_liste, it, line) == false)
				return false;
			if (ruleAutoIndex(token_liste, it, line) == false)
				return false;
			if (ruleIndex(token_liste, it, line) == false)
				return false;
			if (ruleCgiPath(token_liste, it, line) == false)
				return false;
			if (ruleReturn(token_liste, it, line) == false)
				return false;
			//Affectation de temp a location map.
			(*(cnf + server_index)).location_map[location_key] = temp;
			std::cout << GREEN << "\tLocationPath : " << location_key << RESET << std::endl;
			if ((*(cnf + server_index)).location_map[location_key].method_map.find("GET") != (*(cnf + server_index)).location_map[location_key].method_map.end())
        		std::cout << GREEN << "\t\t\"GET\" " << (*(cnf + server_index)).location_map[location_key].method_map["GET"] << RESET << std::endl;
			if ((*(cnf + server_index)).location_map[location_key].method_map.find("POST") != (*(cnf + server_index)).location_map[location_key].method_map.end())
        		std::cout << GREEN << "\t\t\"POST\" " << (*(cnf + server_index)).location_map[location_key].method_map["POST"] << RESET << std::endl;
			if ((*(cnf + server_index)).location_map[location_key].method_map.find("DELETE") != (*(cnf + server_index)).location_map[location_key].method_map.end())
        		std::cout << GREEN << "\t\t\"DELETE\" " << (*(cnf + server_index)).location_map[location_key].method_map["DELETE"] << RESET << std::endl;
			std::cout << GREEN << "\t\tROOT : " << (*(cnf + server_index)).location_map[location_key].root << RESET << std::endl;
			std::cout << GREEN << "\t\tAUTOINDEX : " << (*(cnf + server_index)).location_map[location_key].autoindex << RESET << std::endl;
			std::cout << GREEN << "\t\tINDEX : " << (*(cnf + server_index)).location_map[location_key].index << RESET << std::endl;
			std::cout << GREEN << "\t\tCGIPATH : " << (*(cnf + server_index)).location_map[location_key].cgi_path << RESET << std::endl;
			std::cout << GREEN << "\t\tRETURN : " << (*(cnf + server_index)).location_map[location_key].ret << RESET << std::endl;
			//Recursion
			return ruleLocation(token_liste, it, line);
			//Test du token de fin de bloc, SERVER ou fin de vecteur.
		}
	}
	//Regle facultaive.
	return true;
}

bool Server::ruleMethode(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : METH rMeth_()*/
	//Verification du nombre de token.	
	if (std::distance(it, token_liste.end()) >= 1)
	{
		if ((*it).type == METH)
		{
			//skip METH.
			++it;
			return ruleMethode_(token_liste, it, line);
		}
		return configErr(it, line);
	}
	std::cerr << RED << "Error Config : line " << line << "not enough tokens.";
	std::cerr << RESET << std::endl;
	return false;
}

bool Server::ruleMethode_(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{	
	/*RULE: WORD END | word rMeth_*/

	//Verification du nombre de token.
	if (std::distance(it, token_liste.end()) >= 2)
	{
		if ((*(it)).type == WORD)
		{
			if ((*(it + 1)).type == END)
			{
				//Ajout des METHODES dans la map methode map.
				std::string key((*it).str, (*it).len);
				temp.method_map[key] = true;
				//skip WORD END.
				it += 2;
				//increment de line;
				++line;
				return true;
			}
			else if ((*(it + 1)).type == WORD)
			{
				//Ajout de server name dans un vecteur de char.
				std::string key((*it).str, (*it).len);
				temp.method_map[key] = true;
				//skip WORD.
				++it;
				//Recursion.
				return ruleMethode_(token_liste, it, line);
			}
			return configErr(it + 2, line);
		}
		return configErr(it + 1, line);
	}
	std::cerr << RED << "Error Config : line " << line << "not enough tokens.";
	std::cerr << RESET << std::endl;
	return false;
}

bool Server::ruleRoot(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : ROOT WORD END*/
	int i = 0;

	//Verification du nombre de token.	
	if (std::distance(it, token_liste.end()) >= 3)
	{
		if ((*it).type == ROOT)
		{
			if ((*(it + 1)).type == WORD)
			{
				if ((*(it + 2)).type == END)
				{
					//Affectation de la racine.
					for (i = 0; i < (*(it + 1)).len && i < PATH_MAX; ++i)
						*(temp.root + i) = *((*(it + 1)).str + i);
					if (i == PATH_MAX)
					{
						std::cerr << RED << "Error Config : line " << line << ' ';
						std::cerr.write((*(it + 1)).str, (*(it + 1)).len);
						std::cerr << " is longer than PATH_MAX.";
						std::cerr << RESET << std::endl;
						return false;
					}
					//skip IP WORD END
					it += 3;
					//increment de line.
					++line;
					return true;
				}
				return configErr(it + 2, line);
			}
			return configErr(it + 1, line);
		}
		else if ((*it).type == WORD || (*it).type == PORT || (*it).type == IP || (*it).type == NAME || (*it).type == ERR || (*it).type == END
		|| (*it).type == METH)
			return configErr(it, line);
	}
	return true;
}

bool Server::ruleAutoIndex(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
/*RULE : AUTOINDEX WORD END*/
	int i = 0;

	//Verification du nombre de token.	
	if (std::distance(it, token_liste.end()) >= 3)
	{
		if ((*it).type == AUTOINDEX)
		{
			if ((*(it + 1)).type == WORD)
			{
				if ((*(it + 2)).type == END)
				{
					std::string state((*(it + 1)).str, (*(it +1)).len);
					//Affectation de l'autoindex.
					if (state == "on")
					{
						temp.autoindex = true;
					}
					else if (state == "off")
					{
						temp.autoindex = false;
					}
					else
					{	
						std::cerr << RED << "Error Config : line " << line << ' ';
						std::cerr.write((*(it + 1)).str, (*(it + 1)).len);
						std::cerr << " should be set to \"on\" or \"off\".";
						std::cerr << RESET << std::endl;
						return false;	
					}
					//skip IP WORD END
					it += 3;
					//increment de line.
					++line;
					return true;
				}
				return configErr(it + 2, line);
			}
			return configErr(it + 1, line);
		}
		else if ((*it).type == WORD || (*it).type == PORT || (*it).type == IP || (*it).type == NAME || (*it).type == ERR || (*it).type == END
		|| (*it).type == METH || (*it).type == ROOT)
			return configErr(it, line);
	}
	return true;
}

bool Server::ruleIndex(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{	
	/*RULE : INDEX WORD END*/
	int i = 0;

	//Verification du nombre de token.	
	if (std::distance(it, token_liste.end()) >= 3)
	{
		if ((*it).type == INDEX)
		{
			if ((*(it + 1)).type == WORD)
			{
				if ((*(it + 2)).type == END)
				{
					//Affectation de la racine.
					for (i = 0; i < (*(it + 1)).len && i < PATH_MAX; ++i)
						*(temp.index + i) = *((*(it + 1)).str + i);
					if (i == PATH_MAX)
					{
						std::cerr << RED << "Error Config : line " << line << ' ';
						std::cerr.write((*(it + 1)).str, (*(it + 1)).len);
						std::cerr << " is longer than PATH_MAX.";
						std::cerr << RESET << std::endl;
						return false;
					}
					//skip IP WORD END
					it += 3;
					//increment de line.
					++line;
					return true;
				}
				return configErr(it + 2, line);
			}
			return configErr(it + 1, line);
		}
		else if ((*it).type == WORD || (*it).type == PORT || (*it).type == IP || (*it).type == NAME || (*it).type == ERR || (*it).type == END
		|| (*it).type == METH || (*it).type == ROOT || (*it).type == AUTOINDEX)
			return configErr(it, line);
	}
	return true;
}

bool Server::ruleCgiPath(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : CGIPATH WORD END*/
	int i = 0;

	//Verification du nombre de token.	
	if (std::distance(it, token_liste.end()) >= 3)
	{
		if ((*it).type == CGIPATH)
		{
			if ((*(it + 1)).type == WORD)
			{
				if ((*(it + 2)).type == END)
				{
					//Affectation de la racine.
					for (i = 0; i < (*(it + 1)).len && i < PATH_MAX; ++i)
						*(temp.cgi_path + i) = *((*(it + 1)).str + i);
					if (i == PATH_MAX)
					{
						std::cerr << RED << "Error Config : line " << line << ' ';
						std::cerr.write((*(it + 1)).str, (*(it + 1)).len);
						std::cerr << " is longer than PATH_MAX.";
						std::cerr << RESET << std::endl;
						return false;
					}
					//skip CGI WORD END
					it += 3;
					//increment de line.
					++line;
					return true;
				}
				return configErr(it + 2, line);
			}
			return configErr(it + 1, line);
		}
		else if ((*it).type == WORD || (*it).type == PORT || (*it).type == IP || (*it).type == NAME || (*it).type == ERR || (*it).type == END
		|| (*it).type == METH || (*it).type == ROOT || (*it).type == AUTOINDEX || (*it).type == INDEX)
			return configErr(it, line);
	}
	return true;
}

bool Server::ruleReturn(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : RETURN WORD END*/
	int i = 0;

	//Verification du nombre de token.	
	if (std::distance(it, token_liste.end()) >= 3)
	{
		if ((*it).type == RETURN)
		{
			if ((*(it + 1)).type == WORD)
			{
				if ((*(it + 2)).type == END)
				{
					//Affectation de la racine.
					for (i = 0; i < (*(it + 1)).len && i < PATH_MAX; ++i)
						*(temp.ret + i) = *((*(it + 1)).str + i);
					if (i == PATH_MAX)
					{
						std::cerr << RED << "Error Config : line " << line << ' ';
						std::cerr.write((*(it + 1)).str, (*(it + 1)).len);
						std::cerr << " is longer than PATH_MAX.";
						std::cerr << RESET << std::endl;
						return false;
					}
					//skip RETURN WORD END
					it += 3;
					//increment de line.
					++line;
					return true;
				}
				return configErr(it + 2, line);
			}
			return configErr(it + 1, line);
		}
		else if ((*it).type == WORD || (*it).type == PORT || (*it).type == IP || (*it).type == NAME || (*it).type == ERR || (*it).type == END
		|| (*it).type == METH || (*it).type == ROOT || (*it).type == AUTOINDEX || (*it).type == INDEX || (*it).type == CGIPATH)
			return configErr(it, line);
	}
	return true;
}

void Server::run(void)
{
	setup();
	communicate();
}

void Server::setup(void)
{
	int *virtual_server;

	for (int i = 0; i < cnf_len; ++i)
	{
		if ((virtual_servers[i] = new int [MAX_CLIENT + 1]) == NULL)
		{
			std::cerr << RED << "Error : Unable to allocate server " << i << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		virtual_server = virtual_servers[i];
		if ((virtual_server[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		{
			std::cerr << RED << "Error : Unable to create socket for server " << i << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (setsockopt(virtual_server[0], SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
		{
			std::cerr << RED << "Error : TCP socket creation failed for server " << i << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (bind(virtual_server[0], (struct sockaddr *)&(cnf[i].server_addr), sizeof(struct sockaddr)) == -1)
		{
			std::cerr << RED << "Error : Socket bind failed for server " << i << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		if (listen(virtual_server[0], REQUEST_QUEUE_LEN) < 0)
		{
			std::cerr << RED << "Error : Listen failed for server " << i << RESET << std::endl;
			exit(EXIT_FAILURE);
		}
		for (int j = 1; j < MAX_CLIENT; ++j)
			virtual_server[i] = -1;
	}
}

void Server::communicate(void)
{
	int *virtual_server, i, j;

	//Empeche le server de crash lorsqu'un SIGPIPE est recu.
	signal(SIGPIPE, SIG_IGN);
	while (1)
	{
		//Parcours de chaque server virtuel.
		for (int i = 0; i < cnf_len; ++i)
		{
			virtual_server = virtual_servers[i];
			//Reset des fd_sets.
			FD_ZERO(&readfds);
			FD_ZERO(&writefds);
			FD_ZERO(&exceptfds);
			//Ajout des fd different de -1 dans les fd_sets.
			for (int j = 0; j < MAX_CLIENT; ++j)
			{
				if (virtual_server[j] != -1)
				{
					FD_SET(virtual_server[j], &readfds);
					FD_SET(virtual_server[j], &writefds);
					FD_SET(virtual_server[j], &exceptfds);
				}
			}
			//Activation des sockets ajoutes dans les fd_sets.
			select(getMaxFd(virtual_server) + 1, &readfds, &writefds, &exceptfds, &timeout);
			//Le server communique avec le client suivant sa config.
			acceptServe(virtual_server, cnf[i]);
		}
	}
}

int Server::getMaxFd(int* fds_buffer)
{
	int max = -1;

	for(int i = 0; i < MAX_CLIENT; ++i)
		if (max < *(fds_buffer + i))
			max = *(fds_buffer + i);
	return max;
}

void Server::setNonBlocking(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);
    
    if (flags == -1)
        std::cerr << RED << "ERROR : fcntl F_GETFL" << RESET << std::endl;
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1)
        std::cerr << RED << "ERROR : fcntl F_SETFL" << RESET << std::endl;
}

void Server::acceptServe(int *fd_buffer, t_config cnf)
{
	;
}
