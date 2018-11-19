#include <regex>
#include "services.h"
#include "sha256.h"
#include "db.h"
#include "server.h"
#include "oper.h"
#include "utils.h"

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
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 1) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is already registered.", user->nick().c_str()) + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else {
			if (DB::EscapeChar(x[1]) == true || x[1].find(":") != std::string::npos || x[1].find("!") != std::string::npos ) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The password contains no valid characters (!:;').") + config->EOFMessage);
				return;
			}
			string sql = "INSERT INTO NICKS VALUES ('" + user->nick() + "', '" + sha256(x[1]) + "', '', '', '',  " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s cannot be registered. Please contact with an iRCop.", user->nick().c_str()) + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "INSERT INTO OPTIONS VALUES ('" + user->nick() + "', 0, 0, 0, 0, 0, '" + config->Getvalue("language") + "');";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s cannot be registered. Please contact with an iRCop.", user->nick().c_str()) + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s has been registered.", user->nick().c_str()) + config->EOFMessage);
			if (user->getMode('r') == false) {
				user->session()->send(":" + config->Getvalue("serverName") + " MODE " + user->nick() + " +r" + config->EOFMessage);
				user->setMode('r', true);
				Servidor::sendall("UMODE " + user->nick() + " +r");
			}
			return;
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", user->nick().c_str()) + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (NickServ::Login(user->nick(), x[1]) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Wrong password.") + config->EOFMessage);
			return;
		} else {
			string sql = "DELETE FROM NICKS WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s cannot be deleted. Please contact with an iRCop.", user->nick().c_str()) + config->EOFMessage);
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
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s has been deleted.", user->nick().c_str()) + config->EOFMessage);
			if (user->getMode('r') == true) {
				user->session()->send(":" + config->Getvalue("serverName") + " MODE " + user->nick() + " -r" + config->EOFMessage);
				user->setMode('r', false);
				Servidor::sendall("UMODE " + user->nick() + " -r");
			}
			return;
		}
	} else if (cmd == "EMAIL") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", user->nick().c_str()) + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			string email;
			if (boost::iequals(x[1], "OFF")) {
				email = "";
			} else {
				email = x[1];
			}
			if (std::regex_match(email, std::regex("^[_a-z0-9-]+(.[_a-z0-9-]+)*@[a-z0-9-]+(.[a-z0-9-]+)*(.[a-z]{2,32})$")) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The email seems to be wrong.") + config->EOFMessage);
				return;
			}
			string sql = "UPDATE NICKS SET EMAIL='" + email + "' WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The e-mail for nick %s cannot be changed. Contact with an iRCop.", user->nick().c_str()) + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			if (email.length() > 0)
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The e-mail for nick %s has been changed.", user->nick().c_str()) + config->EOFMessage);
			else
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The e-mail for nick %s has been deleted.", user->nick().c_str()) + config->EOFMessage);
			return;
		}
	} else if (cmd == "URL") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", user->nick().c_str()) + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			string url;
			if (boost::iequals(x[1], "OFF"))
				url = "";
			else
				url = x[1];
			if (std::regex_match(url, std::regex("(ftp|http|https)://\\w+(\\.\\w+)+\\w+(\\/\\w+)*")) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The url seems to be wrong.") + config->EOFMessage);
				return;
			}
			string sql = "UPDATE NICKS SET URL='" + url + "' WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The url for nick %s cannot be changed. Contact with an iRCop.", user->nick().c_str()) + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			if (url.length() > 0)
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your URL has changed.") + config->EOFMessage);
			else
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your URL has been deleted.") + config->EOFMessage);
			return;
		}
	} else if (cmd == "NOACCESS" || cmd == "SHOWMAIL" || cmd == "NOMEMO" || cmd == "NOOP" || cmd == "ONLYREG") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", user->nick().c_str()) + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
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
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The option %s cannot be changed.", cmd.c_str()) + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			if (option == 1)
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The option %s has been setted.", cmd.c_str()) + config->EOFMessage);
			else
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The option %s has been deleted.", cmd.c_str()) + config->EOFMessage);
			return;
		}
	} else if (cmd == "PASSWORD") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", user->nick().c_str()) + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			if (DB::EscapeChar(x[1]) == true || x[1].find(":") != std::string::npos || x[1].find("!") != std::string::npos ) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The password contains no valid characters (!:;').") + config->EOFMessage);
				return;
			}
			string sql = "UPDATE NICKS SET PASS='" + sha256(x[1]) + "' WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The password for nick %s cannot be changed. Contact with an iRCop.", user->nick().c_str()) + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The password for nick %s has been changed to: %s", user->nick().c_str(), x[1].c_str()) + config->EOFMessage);
			return;
		}
	} else if (cmd == "LANG") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(user->nick()) == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", user->nick().c_str()) + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			std::string lang = x[1];
			boost::to_lower(lang);
			if (lang != "es" && lang != "en" && lang != "ca" && lang != "gl") {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The language is not valid, the options are: %s.", "es, en, ca, gl") + config->EOFMessage);
				return;
			}
			string sql = "UPDATE OPTIONS SET LANG='" + lang + "' WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The language cannot be setted.") + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The language has been setted to: %s.", lang.c_str()) + config->EOFMessage);
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

bool NickServ::Login (const string &nickname, const string &pass) {
	string sql = "SELECT PASS from NICKS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	return (retorno == sha256(pass));
}

int NickServ::GetNicks () {
	string sql = "SELECT COUNT(*) FROM NICKS;";
	return DB::SQLiteReturnInt(sql);
}

bool NickServ::GetOption(const string &option, string nickname) {
	if (NickServ::IsRegistered(nickname) == false)
		return false;
	string sql = "SELECT " + option + " FROM OPTIONS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	return DB::SQLiteReturnInt(sql);
}

std::string NickServ::GetLang(string nickname) {
	if (NickServ::IsRegistered(nickname) == false)
		return config->Getvalue("language");
	string sql = "SELECT LANG FROM OPTIONS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	return DB::SQLiteReturnString(sql);
}

string NickServ::GetvHost (string nickname) {
	if (NickServ::IsRegistered(nickname) == false)
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
			user->session()->send(":" + config->Getvalue("nickserv") + " PRIVMSG " + user->nick() + " :" + Utils::make_string(user->nick(), "Memo from: %s Received %s Message: %s", (*it)->sender.c_str(), fecha.c_str(), (*it)->mensaje.c_str()) + config->EOFMessage);
			it = MemoMsg.erase(it);
		} else
			++it;
    }
    Servidor::sendall("MEMODEL " + user->nick());
}
