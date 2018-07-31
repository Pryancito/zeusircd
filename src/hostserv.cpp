#include "db.h"
#include "server.h"
#include "oper.h"
#include "mainframe.h"
#include "utils.h"
#include "services.h"
#include "parser.h"

using namespace std;

void HostServ::Message(User *user, string message) {
	StrVec  x;
	boost::split(x, message, boost::is_any_of(" \t"), boost::token_compress_on);
	std::string cmd = x[0];
	boost::to_upper(cmd);
	
	if (cmd == "HELP") {
		user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :[ /hostserv register|drop|transfer|request|accept|off|list ]" + config->EOFMessage);
		return;
	} else if (cmd == "REGISTER") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /hostserv register path (owner) ]" + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else {
			string owner;
			if (x.size() == 2)
				owner = user->nick();
			else
				owner = x[2];
			if (Parser::checknick(owner) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El nick " + owner + " contiene caracteres no validos." + config->EOFMessage);
				return;
			} else if (NickServ::IsRegistered(owner) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El nick " + owner + " no esta registrado." + config->EOFMessage);
				return;
			} else if (HostServ::CheckPath(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " no es valido." + config->EOFMessage);
				return;
			} else if (HostServ::Owns(user, x[1]) == false && x[1].find("/") != std::string::npos) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " no te pertenece." + config->EOFMessage);
				return;
			} else {
				string sql = "SELECT PATH from PATHS WHERE PATH='" + x[1] + "' COLLATE NOCASE;";
				if (boost::iequals(DB::SQLiteReturnString(sql), x[1]) == true) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " ya esta registrado." + config->EOFMessage);
					return;
				}
				sql = "SELECT VHOST from NICKS WHERE VHOST='" + x[1] + "' COLLATE NOCASE;";
				if (boost::iequals(DB::SQLiteReturnString(sql), x[1]) == true) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " ya esta registrado." + config->EOFMessage);
					return;
				}
				sql = "INSERT INTO PATHS VALUES ('" + owner + "', '" + x[1] + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " no ha podido ser registrado." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " ha sido registrado bajo la cuenta " + owner + config->EOFMessage);
				return;
			}
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /hostserv drop path ]" + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else {
			if (HostServ::CheckPath(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " no es valido." + config->EOFMessage);
				return;
			} else if (HostServ::Owns(user, x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " no te pertenece." + config->EOFMessage);
				return;
			} else {
				if (HostServ::DeletePath(x[1]) == true)
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " ha sido borrado." + config->EOFMessage);
				else
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " no ha podido ser borrado." + config->EOFMessage);
				return;
			}
		}
	} else if (cmd == "TRANSFER") {
		if (x.size() < 3) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /hostserv transfer path owner ]" + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else {
			string owner = x[2];
			if (Parser::checknick(owner) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El nick " + owner + " contiene caracteres no validos." + config->EOFMessage);
				return;
			} else if (NickServ::IsRegistered(owner) == 0) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El nick " + owner + " no esta registrado." + config->EOFMessage);
				return;
			} else if (HostServ::CheckPath(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " no es valido." + config->EOFMessage);
				return;
			} else if (HostServ::Owns(user, x[1]) == false && x[1].find("/") != std::string::npos) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " no te pertenece." + config->EOFMessage);
				return;
			} else {
				string sql = "UPDATE PATHS SET OWNER='" + owner + "' WHERE PATH='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El dueño del path " + x[1] + " no ha podido ser cambiado." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El dueño del path " + x[1] + " ha sido cambiado a " + owner + config->EOFMessage);
				return;
			}
		}
	} else if (cmd == "REQUEST") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /hostserv request path|off ]" + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else {
			if (HostServ::CheckPath(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " no es valido." + config->EOFMessage);
				return;
			} else if (HostServ::GotRequest(user->nick()) == true) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Ya tienes una peticion de vHost." + config->EOFMessage);
				return;
			} else if (HostServ::PathIsInvalid(x[1]) == true) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El path " + x[1] + " no es valido." + config->EOFMessage);
				return;
			} else if (boost::iequals(x[1], "OFF")) {
				string sql = "DELETE FROM REQUEST WHERE OWNER='" + user->nick() + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu peticion no ha podido ser borrada." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu peticion ha sido borrada. " + config->EOFMessage);
				return;
			} else {
				string sql = "INSERT INTO REQUEST VALUES ('" + user->nick() + "', '" + x[1] + "', " + std::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu peticion no ha podido ser registrada." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu peticion ha sido registrada con exito." + config->EOFMessage);
				return;
			}
		}
	} else if (cmd == "ACCEPT") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /hostserv accept nick ]" + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else {
			if (Parser::checknick(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El nick " + x[1] + " contiene caracteres no validos." + config->EOFMessage);
				return;
			} else if (NickServ::IsRegistered(x[1]) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :El nick " + x[1] + " no esta registrado." + config->EOFMessage);
				return;
			} else if (HostServ::GotRequest(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El nick " + x[1] + " no tiene una peticion de vHost." + config->EOFMessage);
				return;
			} else {
				string sql = "SELECT PATH from REQUEST WHERE OWNER='" + x[1] + "' COLLATE NOCASE;";
				string path = DB::SQLiteReturnString(sql);
				sql = "DELETE FROM REQUEST WHERE OWNER='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu aceptacion no ha podido ser finalizada." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				sql = "UPDATE NICKS SET VHOST='" + path + "' WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu aceptacion no ha podido ser finalizada." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu aceptacion ha sido finalizada con exito." + config->EOFMessage);
				User* target = Mainframe::instance()->getUserByName(x[1]);
				if (target)
					target->Cycle();
				return;
			}
		}
	} else if (cmd == "OFF") {
		if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu nick no esta registrado." + config->EOFMessage);
			return;
		} else if (NickServ::GetvHost(user->nick()) == "") {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu nick no tiene un vHost Configurado." + config->EOFMessage);
			return;
		} else {
			string sql = "UPDATE NICKS SET VHOST='' WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu eliminacion de vHost no ha podido ser finalizada.\r\n");
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu eliminacion de vHost ha sido finalizada con exito." + config->EOFMessage);
			return;
		}
	} else if (cmd == "LIST") {
		if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Tu nick no esta registrado." + config->EOFMessage);
			return;
		} else {
			if (x.size() == 1) {
				vector <string> subpaths;
				vector <string> owner;
				string sql = "SELECT PATH from PATHS WHERE OWNER='" + user->nick() + "' COLLATE NOCASE;";
				vector <string> paths = DB::SQLiteReturnVector(sql);
				for (unsigned int i = 0; i < paths.size(); i++) {
					vector <string> temp1;
					vector <string> temp2;
					sql = "SELECT OWNER from REQUEST WHERE PATH LIKE '" + paths[i] + "%' COLLATE NOCASE;";
					temp1 = DB::SQLiteReturnVector(sql);
					sql = "SELECT PATH from REQUEST WHERE PATH LIKE '" + paths[i] + "%' COLLATE NOCASE;";
					temp2 = DB::SQLiteReturnVector(sql);
					for (unsigned int j = 0; j < temp1.size(); j++) {
						subpaths.push_back(temp2[j]);
						owner.push_back(temp1[j]);
					}
				}
				string temp1;
				for (unsigned int i = 0; i < subpaths.size(); i++) {
					if (temp1.find(owner[i]) == std::string::npos) {
						user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :<" + owner[i] + "> PATH: " + subpaths[i] + config->EOFMessage);
						temp1.append(owner[i] + ' ');
					}
				}
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Fin de la lista." + config->EOFMessage);
				return;
			} else if (x.size() == 2) {
				string search = x[1];
				string sql = "SELECT PATH from PATHS ORDER BY OWNER;";
				vector <string> paths = DB::SQLiteReturnVector(sql);
				sql = "SELECT OWNER from PATHS ORDER BY OWNER;";
				vector <string> owner = DB::SQLiteReturnVector(sql);
				for (unsigned int i = 0; i < paths.size(); i++) {
					boost::algorithm::to_lower(paths[i]);
					boost::algorithm::to_lower(search);
					if (Utils::Match(search.c_str(), paths[i].c_str()) == 1)
						user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :<" + owner[i] + "> PATH: " + paths[i] + config->EOFMessage);
				}
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Fin de la lista." + config->EOFMessage);
				return;
			}
		}
	}
}

bool HostServ::CheckPath(string path) {
	StrVec subpaths;
	boost::split(subpaths,path,boost::is_any_of("/"));
	if (subpaths.size() < 1 || subpaths.size() > 10)
		return false;
	for (unsigned int i = 0; i < subpaths.size(); i++) {
		if (subpaths[i].length() == 0)
			return false;
		else if (Parser::checknick(subpaths[i]) == false)
			return false;
	}
	return true;
}

bool HostServ::Owns(User *user, string path) {
	StrVec subpaths;
	boost::split(subpaths,path,boost::is_any_of("/"));
	string pp = subpaths[0];
	for (unsigned int i = 1; i < subpaths.size(); i++) {
		string sql = "SELECT OWNER from PATHS WHERE PATH='" + pp + "' COLLATE NOCASE;";
		string retorno = DB::SQLiteReturnString(sql);
		if (boost::iequals(retorno, user->nick()))
			return true;
		else if (user->getMode('o') == true)
			return true;
		pp.append("/" + subpaths[i]);
	}
	return false;
}

bool HostServ::DeletePath(string path) {
	string sql = "SELECT PATH from PATHS WHERE PATH LIKE '" + path + "%' COLLATE NOCASE;";
	StrVec retorno = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < retorno.size(); i++) {
		string sql = "DELETE FROM PATHS WHERE PATH='" + retorno[i] + "' COLLATE NOCASE;";
		if (DB::SQLiteNoReturn(sql) == false) {
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Servidor::sendall(sql);
	}
	sql = "SELECT NICKNAME from NICKS WHERE VHOST LIKE '" + path + "%' COLLATE NOCASE;";
	retorno = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < retorno.size(); i++) {
		string sql = "UPDATE NICKS SET VHOST='' WHERE NICKNAME='" + retorno[i] + "' COLLATE NOCASE;";
		if (DB::SQLiteNoReturn(sql) == false) {
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Servidor::sendall(sql);
	}
	return true;
}

bool HostServ::GotRequest (string user) {
	string sql = "SELECT OWNER from REQUEST WHERE OWNER='" + user + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	return (boost::iequals(retorno, user));
}

bool HostServ::PathIsInvalid (string path) {
	string sql = "SELECT PATH from PATHS WHERE PATH='" + path + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	if (boost::iequals(retorno, path))
		return true;
	sql = "SELECT VHOST from NICKS WHERE VHOST='" + path + "' COLLATE NOCASE;";
	retorno = DB::SQLiteReturnString(sql);
	if (boost::iequals(retorno, path))
		return true;
		
	return false;
}
