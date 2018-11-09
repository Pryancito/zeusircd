#include "db.h"
#include "server.h"
#include "oper.h"
#include "mainframe.h"
#include "utils.h"
#include "services.h"
#include "sha256.h"

using namespace std;

void OperServ::Message(User *user, string message) {
	StrVec  x;
	boost::split(x, message, boost::is_any_of(" \t"), boost::token_compress_on);
	std::string cmd = x[0];
	boost::to_upper(cmd);
	
	if (cmd == "HELP") {
		user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :[ /operserv gline|kill|drop|setpass|spam|oper ]" + config->EOFMessage);
		return;
	} else if (cmd == "GLINE") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /operserv gline add|del|list (ip) (motivo) ]" + config->EOFMessage);
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
					else if (it->second->host() == x[2] && it->second->server() == config->Getvalue("serverName"))
						it->second->cmdQuit();
					else if (it->second->host() == x[2] && it->second->server() != config->Getvalue("serverName"))
						it->second->QUIT();
				}
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
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :La GLINE no se ha podido borrar. Contacte con un iRCop." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Se ha quitado la GLINE." + config->EOFMessage);
			} else if (boost::iequals(x[1], "LIST")) {
				vector<vector<string> > result;
				string sql = "SELECT IP, NICK, MOTIVO FROM GLINE ORDER BY IP;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No hay GLINES." + config->EOFMessage);
					return;
				}
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :\002" + row.at(0) + "\002 por " + row.at(1) + ". Motivo: " + row.at(2) + config->EOFMessage);
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
		if (target->server() == config->Getvalue("serverName"))
			target->cmdQuit();
		else
			target->QUIT();
		Servidor::sendall("QUIT " + target->nick());
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
			User* target = Mainframe::instance()->getUserByName(x[1]);
			if (target) {
				if (target->server() == config->Getvalue("serverName")) {
					target->session()->send(":" + config->Getvalue("serverName") + " MODE " + user->nick() + " -r" + config->EOFMessage);
					target->setMode('r', false);
				} else
					Servidor::sendall("UMODE " + target->nick() + " -r");
			}
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
		if (x.size() < 3) {
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
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /operserv spam add|del|list (mascara) (flags) ]" + config->EOFMessage);
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
				} else if (OperServ::IsSpammed(x[2]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El SPAM ya existe." + config->EOFMessage);
					return;
				} else if (DB::EscapeChar(x[2]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El SPAM contiene caracteres no validos." + config->EOFMessage);
					return;
				}
				boost::to_lower(x[3]);
				std::string sql = "INSERT INTO SPAM VALUES ('" + x[2] + "', '" + user->nick() + "', 'Se ha activado el filtro antispam.', '" + x[3] + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El registro no se ha podido insertar." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				oper.GlobOPs("Se ha insertado el SPAM a la MASCARA: " + x[2] + " por el nick: " + user->nick());
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
				oper.GlobOPs("Se ha quitado el SPAM a la MASCARA: " + x[2] + " por el nick: " + user->nick());
			} else if (boost::iequals(x[1], "LIST")) {
				vector<vector<string> > result;
				string sql = "SELECT MASK, WHO, TARGET FROM SPAM ORDER BY WHO;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0)
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No hay SPAM." + config->EOFMessage);
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :\002" + row.at(0) + "\002 por " + row.at(1) + ". Flags: " + row.at(2) + config->EOFMessage);
				}
				return;
			}
			return;
		}
	} else if (cmd == "OPER") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /operserv oper add|del|list (nick) ]" + config->EOFMessage);
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
				if (x.size() < 3) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos." + config->EOFMessage);
					return;
				} if (NickServ::IsRegistered(x[2]) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El nick " + x[2] + " no esta registrado." + config->EOFMessage);
					return;
				} else if (OperServ::IsOper(x[2]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El iRCop ya existe." + config->EOFMessage);
					return;
				}
				std::string sql = "INSERT INTO OPERS VALUES ('" + x[2] + "', '" + user->nick() + "', " + std::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El registro no se ha podido insertar." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				oper.GlobOPs("Se ha insertado el OPER: " + x[2] + " por el nick: " + user->nick());
			} else if (boost::iequals(x[1], "DEL")) {
				Oper oper;
				if (x.size() < 3) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos." + config->EOFMessage);
					return;
				}
				if (OperServ::IsOper(x[2]) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No existe OPER con ese nick." + config->EOFMessage);
					return;
				}
				std::string sql = "DELETE FROM OPERS WHERE NICK='" + x[2] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :El registro no se ha podido borrar." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				oper.GlobOPs("Se ha borrado el OPER de: " + x[2] + " por el nick: " + user->nick());
			} else if (boost::iequals(x[1], "LIST")) {
				vector<vector<string> > result;
				string sql = "SELECT NICK, OPERBY, TIEMPO FROM OPERS ORDER BY NICK;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :No hay OPERs." + config->EOFMessage);
					return;
				}
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					time_t time = stoi(row.at(2));
					std::string cuando = Utils::Time(time);
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :\002" + row.at(0) + "\002 opeado por " + row.at(1) + " hace: " + cuando + config->EOFMessage);
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

std::string OperServ::ReasonGlined(const string &ip) {
	std::string sql = "SELECT MOTIVO from GLINE WHERE IP='" + ip + "' COLLATE NOCASE;";
	return DB::SQLiteReturnString(sql);
}

bool OperServ::IsOper(string nick) {
	std::string sql = "SELECT NICK from OPERS WHERE NICK='" + nick + "' COLLATE NOCASE;";
	std::string retorno = DB::SQLiteReturnString(sql);
	return (boost::iequals(retorno, nick));
}

bool OperServ::IsSpammed(string mask) {
	StrVec vect;
	std::string sql = "SELECT MASK from SPAM;";
	vect = DB::SQLiteReturnVector(sql);
	boost::to_lower(mask);
	for (unsigned int i = 0; i < vect.size(); i++) {
		boost::to_lower(vect[i]);
		if (Utils::Match(vect[i].c_str(), mask.c_str()) == true)
			return true;
	}
	return false;
}

bool OperServ::IsSpam(string mask, string flags) {
	StrVec vect;
	boost::to_lower(flags);
	std::string sql = "SELECT MASK from SPAM WHERE TARGET LIKE '%" + flags + "%' COLLATE NOCASE;";
	vect = DB::SQLiteReturnVector(sql);
	boost::to_lower(mask);
	for (unsigned int i = 0; i < vect.size(); i++) {
		boost::to_lower(vect[i]);
		if (Utils::Match(vect[i].c_str(), mask.c_str()) == true)
			return true;
	}
	return false;
}
