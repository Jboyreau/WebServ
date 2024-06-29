#include "Server.h"

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
		if (it != token_liste.end())
		{
			if ((*it).type == SERVER)
			{
				(*(cnf + server_index)).max_body_size = MAX_BODY_SIZE;
				std::cout << GREEN << "\nVIRTUAL SERVER " << server_index << " CONFIGURATION : " << std::endl;
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
	if (server_index > MAX_VSERVER)
	{
		parsing_result = false;
		std::cerr << RED << "Error Config : more than " << MAX_VSERVER << '.' << RESET << std::endl;
	}
	cnf_len = server_index;
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

	(*(cnf + server_index)).error_map["411"] = E411;
	(*(cnf + server_index)).error_map["413"] = E413;
	(*(cnf + server_index)).error_map["403"] = E403;
	(*(cnf + server_index)).error_map["404"] = E404;
	(*(cnf + server_index)).error_map["500"] = E500;
	(*(cnf + server_index)).error_map["502"] = E502;
	(*(cnf + server_index)).error_map["503"] = E503;
	(*(cnf + server_index)).error_map["504"] = E504;
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
				std::string name((*it).str, (*it).len);
				(*(cnf + server_index)).name_map[name] = server_index;
				std::cout << GREEN << "\tName Key: " << name << RESET << std::endl;
				std::cout << GREEN << "\tName map Content : " << (*(cnf + server_index)).name_map[name] << RESET << std::endl;
				//skip WORD END.
				it += 2;
				//increment de line;
				++line;
				return true;
			}
			else if ((*(it + 1)).type == WORD)
			{
				//Ajout de server name dans un vecteur de char.
				std::string name((*it).str, (*it).len);
				(*(cnf + server_index)).name_map[name] = server_index;
				std::cout << GREEN << "\tName Key: " << name << RESET << std::endl;
				std::cout << GREEN << "\tName map Content : " << (*(cnf + server_index)).name_map[name] << RESET << std::endl;
				//skip WORD.
				++it;
				//Recursion.
				return ruleName_(token_liste, it, line);
			}
			return configErr(it, line);
		}
		return configErr(it, line);
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
					if (((*(cnf + server_index)).max_body_size = atoi((*(it + 1)).str)) <= MAX_BODY_SIZE)
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

bool Server::ruleLocation(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : LOC WORD END rMeth() rRoot() rAutoIndex() rIndex() rCgiPath() rReturn() rLocation() | z*/
	//Clear de la sructure temp.
	temp.method_map.clear();
	std::strcpy(temp.root, DEFAULT_ROOT);
	temp.autoindex = false;
	std::strcpy(temp.index, DEFAULT_INDEX_PATH);
	std::strcpy(temp.cgi_path, DEFAULT_CGI_PATH);	
	memset(temp.ret, 0, PATH_MAX);
	//Verification du nombre de token.
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
				if (key != "GET" && key != "POST" && key != "DELETE")
					return configErr(it, line);
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
			return configErr(it, line);
		}
		return configErr(it, line);
	}
	std::cerr << RED << "Error Config : line " << line << "not enough tokens.";
	std::cerr << RESET << std::endl;
	return false;
}

bool Server::ruleRoot(std::vector<t_token> &token_liste, std::vector<t_token>::iterator &it, int &line)
{
	/*RULE : ROOT WORD END | z*/
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
					memset(temp.root, 0, PATH_MAX);
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
					if (canAccessDirectory(temp.root) == false)
					{
						std::cerr << RED << "Error Config : line " << line << ' ';
                        std::cerr << "Cannot open directory: " << temp.root << RESET << std::endl;
                        return false;
					}
					//skip ROOT WORD END
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
/*RULE : AUTOINDEX WORD END | z*/
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
	/*RULE : INDEX WORD END | z*/
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
					memset(temp.index, 0, PATH_MAX);
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
					if (canAccessFile(temp.index, R_OK) == false)
					{
						std::cerr << RED << "Error Config : line " << line << ' ';
                        std::cerr << "Cannot open directory: " << temp.root << RESET << std::endl;
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
	/*RULE : CGIPATH WORD END | z*/
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
					memset(temp.cgi_path, 0, PATH_MAX);
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
					if (canAccessFile(temp.cgi_path, X_OK) == false)
					{
						std::cerr << RED << "Error Config : line " << line << ' ';
                        std::cerr << "Cannot open directory: " << temp.root << RESET << std::endl;
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
	/*RULE : RETURN WORD END | z*/
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
