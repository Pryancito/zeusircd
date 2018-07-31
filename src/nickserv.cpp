#include <regex>
#include "services.h"
#include "sha256.h"
#include "db.h"
#include "server.h"
#include "oper.h"

using namespace std;

Memos MemoMsg;

void NickServ::Message(User *user, string message) {
	StrVec  x;
	boost::split(x, message, boost::is_any_of(" \t"), boost::token_compress_on);
	std::string cmd = x[0];
	boost::to_upper(cmd);
	
	if (cmd == "HELP") {
		user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :[ /nickserv register|drop|email|url|noaccess|nomemo|noop|showmail|onlyreg|password|lang ]" + config->EOFMessage);
		return;
	} else if (cmd == "REGISTER") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /nickserv register password ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 1) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick ya esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else {
			if (DB::EscapeChar(x[1]) == true) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El password contiene caracteres no validos (!:;')." + config->EOFMessage);
				return;
			}
			string sql = "INSERT INTO NICKS VALUES ('" + user->nick() + "', '" + sha256(x[1]) + "', '', '', '',  " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick " + user->nick() + " no ha sido registrado." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "INSERT INTO OPTIONS VALUES ('" + user->nick() + "', 0, 0, 0, 0, 0, '" + config->Getvalue("language") + "');";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick " + user->nick() + " no ha sido registrado." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick " + user->nick() + " ha sido registrado." + config->EOFMessage);
			if (user->getMode('r') == false) {
				user->session()->send(":" + config->Getvalue("serverName") + " MODE " + user->nick() + " +r" + config->EOFMessage);
				user->setMode('r', true);
				Servidor::sendall("UMODE " + user->nick() + " +r");
			}
			return;
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /nickserv drop password ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :No te has identificado, para hacer DROP necesitas tener el nick puesto." + config->EOFMessage);
			return;
		} else if (NickServ::Login(user->nick(), x[1]) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :La password no coincide." + config->EOFMessage);
			return;
		} else {
			string sql = "DELETE FROM NICKS WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick " + user->nick() + " no ha sido borrado." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "DELETE FROM ACCESS WHERE USUARIO='" + user->nick() + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick " + user->nick() + " ha sido borrado." + config->EOFMessage);
			if (user->getMode('r') == true) {
				user->session()->send(":" + config->Getvalue("serverName") + " MODE " + user->nick() + " -r" + config->EOFMessage);
				user->setMode('r', false);
				Servidor::sendall("UMODE " + user->nick() + " -r");
			}
			return;
		}
	} else if (cmd == "EMAIL") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /nickserv email tu@email.tld|off ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :No te has identificado, para hacer EMAIL necesitas tener el nick puesto." + config->EOFMessage);
			return;
		} else {
			string email;
			if (boost::iequals(x[1], "OFF")) {
				email = "";
			} else {
				email = x[1];
			}
			if (std::regex_match(email, std::regex("^[_a-z0-9-]+(.[_a-z0-9-]+)*@[a-z0-9-]+(.[a-z0-9-]+)*(.[a-z]{2,4})$")) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El email contiene caracteres no validos." + config->EOFMessage);
				return;
			}
			string sql = "UPDATE NICKS SET EMAIL='" + email + "' WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick " + user->nick() + " no ha podido cambiar el correo electronico." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			if (email.length() > 0)
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Has cambiado tu EMAIL." + config->EOFMessage);
			else
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Has borrado tu EMAIL." + config->EOFMessage);
			return;
		}
	} else if (cmd == "URL") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /nickserv url www.tuweb.com|off ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :No te has identificado, para hacer URL necesitas tener el nick puesto." + config->EOFMessage);
			return;
		} else {
			string url;
			if (boost::iequals(x[1], "OFF"))
				url = "";
			else
				url = x[1];
			if (std::regex_match(url, std::regex("(ftp|http|https)://\\w+(\\.\\w+)+\\w+(\\/\\w+)*")) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El url contiene caracteres no validos." + config->EOFMessage);
				return;
			}
			string sql = "UPDATE NICKS SET URL='" + url + "' WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick " + user->nick() + " no ha podido cambiar la web." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			if (url.length() > 0)
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Has cambiado tu URL." + config->EOFMessage);
			else
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Has borrado tu URL." + config->EOFMessage);
			return;
		}
	} else if (cmd == "NOACCESS" || cmd == "SHOWMAIL" || cmd == "NOMEMO" || cmd == "NOOP" || cmd == "ONLYREG") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /nickserv noaccess|showmail|nomemo|noop|onlyreg on|off ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :No te has identificado, para hacer URL necesitas tener el nick puesto." + config->EOFMessage);
			return;
		} else {
			int option = 0;
			if (boost::iequals(x[1], "OFF"))
				option = 0;
			else if (boost::iequals(x[1], "ON"))
				option = 1;
			else
				return;
			string sql = "UPDATE OPTIONS SET " + cmd + "=" + std::to_string(option) + " WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick " + user->nick() + " no ha podido cambiar las opciones." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			if (option == 1)
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Has activado la opcion " + cmd + config->EOFMessage);
			else
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Has desactivado la opcion " + cmd + config->EOFMessage);
			return;
		}
	} else if (cmd == "PASSWORD") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /nickserv password nuevapass ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :No te has identificado, para cambiar la password necesitas tener el nick puesto." + config->EOFMessage);
			return;
		} else {
			if (DB::EscapeChar(x[1]) == true) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El password contiene caracteres no validos (!:\')." + config->EOFMessage);
				return;
			}
			string sql = "UPDATE NICKS SET PASS='" + sha256(x[1]) + "' WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick " + user->nick() + " no ha podido cambiar la password." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Has cambiado la contraseña a: " + x[1] + config->EOFMessage);
			return;
		}
	} else if (cmd == "LANG") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /nickserv lang es|en ]" + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El nick no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :No te has identificado, para hacer LANG necesitas tener el nick puesto." + config->EOFMessage);
			return;
		} else {
			std::string lang = x[1];
			boost::to_lower(lang);
			if (lang != "es" && lang != "en") {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El idioma no es valido, las opciones son: es, en." + config->EOFMessage);
				return;
			}
			string sql = "UPDATE NICKS SET LANG='" + lang + "' WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :El idioma no se ha podido cambiar." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Has cambiado tu idioma." + config->EOFMessage);
			return;
		}
	}
	return;
}

void NickServ::UpdateLogin (User *user) {
	if (Server::HUBExiste() == false) return;

	string sql = "UPDATE NICKS SET LASTUSED=" + std::to_string(time(0)) + " WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
	if (DB::SQLiteNoReturn(sql) == false) return;
	sql = "DB " + DB::GenerateID() + " " + sql;
	DB::AlmacenaDB(sql);
	Servidor::sendall(sql);
	return;
}

bool NickServ::IsRegistered(string nickname) {
	string sql = "SELECT NICKNAME from NICKS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	return (boost::iequals(retorno, nickname));
} 

bool NickServ::Login (string nickname, string pass) {
	string sql = "SELECT PASS from NICKS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	return (retorno == sha256(pass));
}

int NickServ::GetNicks () {
	string sql = "SELECT COUNT(*) FROM NICKS;";
	return DB::SQLiteReturnInt(sql);
}

bool NickServ::GetOption(string option, string nickname) {
	if (NickServ::IsRegistered(nickname) == 0)
		return false;
	string sql = "SELECT " + option + " FROM OPTIONS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	return DB::SQLiteReturnInt(sql);
}

std::string NickServ::GetLang(string nickname) {
	if (NickServ::IsRegistered(nickname) == 0)
		return config->Getvalue("language");
	string sql = "SELECT LANG FROM OPTIONS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	return DB::SQLiteReturnString(sql);
}

string NickServ::GetvHost (string nickname) {
	if (NickServ::IsRegistered(nickname) == 0)
		return "";
	string sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	return DB::SQLiteReturnString(sql);
}

int NickServ::MemoNumber(const std::string& nick) {
	int i = 0;
	Memos::iterator it = MemoMsg.begin();
	for(; it != MemoMsg.end(); ++it)
		if (boost::iequals((*it)->receptor, nick))
			i++;
	return i;
}

void NickServ::checkmemos(User* user) {
	if (!IsRegistered(user->nick()))
		return;
    Memos::iterator it = MemoMsg.begin();
    while (it != MemoMsg.end()) {
		if (boost::iequals((*it)->receptor, user->nick())) {
			struct tm *tm = localtime(&(*it)->time);
			char date[30];
			strftime(date, sizeof(date), "%r %d-%m-%Y", tm);
			string fecha = date;
			user->session()->send(":" + config->Getvalue("nickserv") + " PRIVMSG " + user->nick() + " :Memo de: " + (*it)->sender + " Recibido: " + fecha + (*it)->mensaje + config->EOFMessage);
			it = MemoMsg.erase(it);
		} else
			it++;
    }
    Servidor::sendall("MEMODEL " + user->nick());
}
