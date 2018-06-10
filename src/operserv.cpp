#include "db.h"
#include "server.h"
#include "oper.h"
#include "mainframe.h"
#include "utils.h"
#include "services.h"
#include "sha256.h"

using namespace std;

void insert_rule (std::string ip)
{
	std::string cmd;
	if (ip.find(":") == std::string::npos) {
		for (unsigned int i = 0; config->Getvalue("listen["+std::to_string(i)+"]port").length() > 0 && config->Getvalue("listen["+std::to_string(i)+"]class") == "client"; i++) {
			cmd = "sudo iptables -A INPUT -s " + ip + " " + config->Getvalue("listen["+std::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen["+std::to_string(i)+"]port") + " -j DROP";
			system(cmd.c_str());
		}
	} else {
		for (unsigned int i = 0; config->Getvalue("listen6["+std::to_string(i)+"]port").length() > 0 && config->Getvalue("listen6["+std::to_string(i)+"]class") == "client"; i++) {
			cmd = "sudo ip6tables -A INPUT -s " + ip + " " + config->Getvalue("listen6["+std::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen6["+std::to_string(i)+"]port") + " -j DROP";
			system(cmd.c_str());
		}
	}
}

void delete_rule (std::string ip)
{
	std::string cmd;
	if (ip.find(":") == std::string::npos) {
		for (unsigned int i = 0; config->Getvalue("listen["+std::to_string(i)+"]port").length() > 0 && config->Getvalue("listen["+std::to_string(i)+"]class") == "client"; i++) {
			cmd = "sudo iptables -D INPUT -s " + ip + " " + config->Getvalue("listen["+std::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen["+std::to_string(i)+"]port") + " -j DROP";
			system(cmd.c_str());
		}
	} else {
		for (unsigned int i = 0; config->Getvalue("listen6["+std::to_string(i)+"]port").length() > 0 && config->Getvalue("listen6["+std::to_string(i)+"]class") == "client"; i++) {
			cmd = "sudo ip6tables -D INPUT -s " + ip + " " + config->Getvalue("listen6["+std::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen6["+std::to_string(i)+"]port") + " -j DROP";
			system(cmd.c_str());
		}
	}
}

void OperServ::ApplyGlines () {
	std::string cmd = "sudo iptables -F INPUT";
	system(cmd.c_str());
	vector <string> ip;
	std::string sql = "SELECT IP FROM GLINE";
	ip = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < ip.size(); i++)
		insert_rule(ip[i]);
}

void OperServ::Message(User *user, string message) {
	StrVec  x;
	boost::split(x, message, boost::is_any_of(" \t"), boost::token_compress_on);
	std::string cmd = x[0];
	boost::to_upper(cmd);
	
	if (cmd == "HELP") {
		user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :[ /operserv gline|kill|drop|setpass ]" + config->EOFMessage);
		return;
	} else if (cmd == "GLINE") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /operserv gline add|del|list (ip) (motivo) ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Tu nick no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No te has identificado, para manejar las listas de acceso necesitas tener el nick puesto." + config->EOFMessage);
			return;
		} else {
			if (boost::iequals(x[1], "ADD")) {
				if (x.size() < 4) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos." + config->EOFMessage);
					return;
				} else if (OperServ::IsGlined(x[2]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El GLINE ya existe." + config->EOFMessage);
					return;
				} else if (DB::EscapeChar(x[2]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El GLINE contiene caracteres no validos." + config->EOFMessage);
					return;
				}
				int length = 7 + x[1].length() + x[2].length();
				std::string motivo = message.substr(length);
				std::string sql = "INSERT INTO GLINE VALUES ('" + x[2] + "', '" + motivo + "', '" + user->nick() + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El registro no se ha podido insertar." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				
				UserMap usermap = Mainframe::instance()->users();
				UserMap::iterator it = usermap.begin();
				for (; it != usermap.end(); ++it) {
					if (!it->second)
						continue;
					else if (it->second->session()->ip() == x[2])
						it->second->cmdQuit();
				}
				insert_rule(x[2]);
				Servidor::sendall("NEWGLINE");
				Oper oper;
				oper.GlobOPs("Se ha insertado el GLINE a la IP " + x[2] + " por " + user->nick() + ". Motivo: " + motivo);
			} else if (boost::iequals(x[1], "DEL")) {
				if (x.size() < 3) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos." + config->EOFMessage);
					return;
				}
				if (OperServ::IsGlined(x[2]) == 0) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No existe GLINE con esa IP." + config->EOFMessage);
					return;
				}
				std::string sql = "DELETE FROM GLINE WHERE IP='" + x[2] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El registro no se ha podido borrar." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				delete_rule(x[2]);
				Servidor::sendall("NEWGLINE");
				user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Se ha quitado la GLINE." + config->EOFMessage);
			} else if (boost::iequals(x[1], "LIST")) {
				StrVec ip;
				StrVec who;
				StrVec motivo;
				string sql = "SELECT IP FROM GLINE;";
				ip = DB::SQLiteReturnVector(sql);
				sql = "SELECT NICK FROM GLINE;";
				who = DB::SQLiteReturnVector(sql);
				sql = "SELECT MOTIVO FROM GLINE;";
				motivo = DB::SQLiteReturnVector(sql);
				if (ip.size() == 0)
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No hay GLINES." + config->EOFMessage);
				for (unsigned int i = 0; i < ip.size(); i++) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :\002" + ip[i] + "\002 por " + who[i] + ". Motivo: " + motivo[i] + config->EOFMessage);
				}
				return;
			}
			return;
		}
	} else if (cmd == "KILL") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /operserv kill nick ]" + config->EOFMessage);
			return;
		}
		User* target = Mainframe::instance()->getUserByName(x[1]);
		if (!target) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El nick no esta conectado." + config->EOFMessage);
			return;
		}
		target->cmdQuit();
		return;
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /operserv drop (nick|canal) ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(x[1]) == 0 && ChanServ::IsRegistered(x[1]) == 0) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El nick no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No te has identificado, para hacer DROP necesitas tener el nick puesto." + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(x[1]) == 1) {
			std::string sql = "DELETE FROM NICKS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :El nick " + x[1] + " no ha sido borrado." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "DELETE FROM ACCESS WHERE USUARIO='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El nick " + x[1] + " ha sido borrado." + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == 1) {
			std::string sql = "DELETE FROM CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":CHaN!*@* NOTICE " + user->nick() + " :El canal " + x[1] + " no ha sido borrado." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "DELETE FROM ACCESS WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "DELETE FROM AKICK WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":CHaN!*@* NOTICE " + user->nick() + " :El canal " + x[1] + " ha sido borrado." + config->EOFMessage);
			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			if (chan->getMode('r') == true) {
				chan->setMode('r', false);
				chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -r" + config->EOFMessage);
				Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -r");
			}
		}
	} else if (cmd == "SETPASS") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /operserv setpass nick pass ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(x[1]) == 0) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El nick no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No te has identificado, para hacer DROP necesitas tener el nick puesto." + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(x[1]) == 1) {
			string sql = "UPDATE NICKS SET PASS='" + sha256(x[2]) + "' WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :La pass del nick " + x[1] + " no ha podido ser cambiada." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :La pass del nick " + x[1] + " ha sido cambiada a " + x[2] + config->EOFMessage);
			return;
		}
	} else if (cmd == "SPAM") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /operserv spam add|del|list (flags) (mascara) ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Tu nick no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No te has identificado, para manejar las listas de spam necesitas tener el nick puesto." + config->EOFMessage);
			return;
		} else {
			if (boost::iequals(x[1], "ADD")) {
				Oper oper;
				if (x.size() < 4) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos." + config->EOFMessage);
					return;
				} else if (OperServ::IsSpammed(x[3]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El SPAM ya existe." + config->EOFMessage);
					return;
				} else if (DB::EscapeChar(x[3]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El GLINE contiene caracteres no validos." + config->EOFMessage);
					return;
				}
				std::string sql = "INSERT INTO SPAM VALUES ('" + x[2] + "', '" + user->nick() + "', 'Se ha activado el filtro antispam.', '" + x[3] + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El registro no se ha podido insertar." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				oper.GlobOPs("Se ha insertado el SPAM a la MASCARA: " + x[2] + " por el nick: " + user->nick() + config->EOFMessage);
			} else if (boost::iequals(x[1], "DEL")) {
				Oper oper;
				if (x.size() < 3) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos." + config->EOFMessage);
					return;
				}
				if (OperServ::IsSpammed(x[2]) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No existe SPAM con esa MASK." + config->EOFMessage);
					return;
				}
				std::string sql = "DELETE FROM SPAM WHERE MASK='" + x[2] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El registro no se ha podido borrar." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				oper.GlobOPs("Se ha quitado el SPAM a la MASCARA: " + x[2] + " por el nick: " + user->nick() + "." + config->EOFMessage);
			} else if (boost::iequals(x[1], "LIST")) {
				StrVec mask;
				StrVec who;
				StrVec target;
				std::string sql = "SELECT MASK FROM GLINE ORDER BY WHO;";
				mask = DB::SQLiteReturnVector(sql);
				sql = "SELECT WHO FROM GLINE ORDER BY WHO;";
				who = DB::SQLiteReturnVector(sql);
				sql = "SELECT TARGET FROM GLINE ORDER BY WHO;";
				target = DB::SQLiteReturnVector(sql);
				if (mask.size() == 0)
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No hay SPAM." + config->EOFMessage);
				for (unsigned int i = 0; i < mask.size(); i++) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :\002" + mask[i] + "\002 por " + who[i] + ". Flags: " + target[i] + config->EOFMessage);
				}
				return;
			}
			return;
		}
	}
}

bool OperServ::IsGlined(string ip) {
	std::string sql = "SELECT IP from GLINE WHERE IP='" + ip + "' COLLATE NOCASE;";
	std::string retorno = DB::SQLiteReturnString(sql);
	return (boost::iequals(retorno, ip));
}

bool OperServ::IsSpammed(string mask) {
	StrVec vect;
	std::string sql = "SELECT MASK from SPAM;";
	vect = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < vect.size(); i++)
		if (Utils::Match(mask.c_str(), vect[i].c_str()) == true)
			return true;
	return false;
}

bool OperServ::IsSpam(string mask, string flags) {
	StrVec vect;
	std::string sql = "SELECT MASK from SPAM WHERE TARGET LIKE '%" + flags + "%' COLLATE NOCASE;";
	vect = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < vect.size(); i++)
		if (Utils::Match(mask.c_str(), vect[i].c_str()) == true)
			return true;
	return false;
}
