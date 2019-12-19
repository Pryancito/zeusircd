/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <regex>
#include "ZeusBaseClass.h"
#include "db.h"
#include "Server.h"
#include "oper.h"
#include "Utils.h"
#include "services.h"
#include "base64.h"
#include "Config.h"
#include "sha256.h"

using namespace std;

Memos MemoMsg;

void NickServ::Message(LocalUser *user, string message) {
	std::vector<std::string> split;
	Config::split(message, split, " \t");
	
	if (split.size() == 0)
		return;

	std::string cmd = split[0];
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
	
	if (cmd == "HELP") {
		user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :[ /nickserv register|drop|email|url|noaccess|nomemo|noop|showmail|onlyreg|password|lang|nocolor ]");
		return;
	} else if (cmd == "REGISTER") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('r') == true) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is already registered.", user->mNickName.c_str()));
			return;
		} else if (Server::HUBExiste() == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else {
			if (DB::EscapeChar(split[1]) == true || split[1].find(":") != std::string::npos || split[1].find("!") != std::string::npos ) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password contains no valid characters (!:;')."));
				return;
			}
			string sql = "INSERT INTO NICKS VALUES ('" + user->mNickName + "', '" + sha256(split[1]) + "', '', '', '',  " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be registered. Please contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "INSERT INTO OPTIONS (NICKNAME, LANG) VALUES ('" + user->mNickName + "', '" + config->Getvalue("language") + "');";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be registered. Please contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Server::Send(sql);
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s has been registered.", user->mNickName.c_str()));
			if (user->getMode('r') == false) {
				user->Send(":" + config->Getvalue("serverName") + " MODE " + user->mNickName + " +r");
				user->setMode('r', true);
				Server::Send("UMODE " + user->mNickName + " +r");
			}
			return;
		}
	} else if (cmd == "DROP") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
			return;
		} else if (Server::HUBExiste() == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else if (NickServ::Login(user->mNickName, split[1]) == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Wrong password."));
			return;
		} else {
			string sql = "DELETE FROM NICKS WHERE NICKNAME='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "DELETE FROM CANALES WHERE OWNER='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "DELETE FROM ACCESS WHERE USUARIO='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "SELECT PATH FROM PATHS WHERE OWNER='" + user->mNickName + "';";
			vector <std::string> result = DB::SQLiteReturnVector(sql);
			for (unsigned int i = 0; i < result.size(); i++)
				HostServ::DeletePath(result[i]);
			sql = "DELETE FROM REQUEST WHERE OWNER='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "DELETE FROM OPERS WHERE NICK='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s has been deleted.", user->mNickName.c_str()));
			if (user->getMode('r') == true) {
				user->Send(":" + config->Getvalue("serverName") + " MODE " + user->mNickName + " -r");
				user->setMode('r', false);
				Server::Send("UMODE " + user->mNickName + " -r");
			}
			return;
		}
	} else if (cmd == "EMAIL") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
			return;
		} else if (Server::HUBExiste() == 0) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else {
			string email;
			if (strcasecmp(split[1].c_str(), "OFF") == 0) {
				email = "";
			} else {
				email = split[1];
			}
			if (std::regex_match(email, std::regex("^[_a-z0-9-]+(.[_a-z0-9-]+)*@[a-z0-9-]+(.[a-z0-9-]+)*(.[a-z]{2,32})$")) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The email seems to be wrong."));
				return;
			}
			string sql = "UPDATE NICKS SET EMAIL='" + email + "' WHERE NICKNAME='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The e-mail for nick %s cannot be changed. Contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			if (email.length() > 0)
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The e-mail for nick %s has been changed.", user->mNickName.c_str()));
			else
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The e-mail for nick %s has been deleted.", user->mNickName.c_str()));
			return;
		}
	} else if (cmd == "URL") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
			return;
		} else if (Server::HUBExiste() == 0) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else {
			string url;
			if (strcasecmp(split[1].c_str(), "OFF") == 0)
				url = "";
			else
				url = split[1];
			if (std::regex_match(url, std::regex("(ftp|http|https)://\\w+(\\.\\w+)+\\w+(\\/\\w+)*")) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The url seems to be wrong."));
				return;
			}
			string sql = "UPDATE NICKS SET URL='" + url + "' WHERE NICKNAME='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The url for nick %s cannot be changed. Contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			if (url.length() > 0)
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your URL has changed."));
			else
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your URL has been deleted."));
			return;
		}
	} else if (cmd == "NOACCESS" || cmd == "SHOWMAIL" || cmd == "NOMEMO" || cmd == "NOOP" || cmd == "ONLYREG" || cmd == "NOCOLOR") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
			return;
		} else if (Server::HUBExiste() == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else {
			int option = 0;
			if (strcasecmp(split[1].c_str(), "OFF") == 0)
				option = 0;
			else if (strcasecmp(split[1].c_str(), "ON") == 0)
				option = 1;
			else
				return;
			string sql = "UPDATE OPTIONS SET " + cmd + "=" + std::to_string(option) + " WHERE NICKNAME='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The option %s cannot be changed.", cmd.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			if (option == 1)
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The option %s has been setted.", cmd.c_str()));
			else
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The option %s has been deleted.", cmd.c_str()));
			return;
		}
	} else if (cmd == "PASSWORD") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
			return;
		} else if (Server::HUBExiste() == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else {
			if (DB::EscapeChar(split[1]) == true || split[1].find(":") != std::string::npos || split[1].find("!") != std::string::npos ) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password contains no valid characters (!:;')."));
				return;
			}
			string sql = "UPDATE NICKS SET PASS='" + sha256(split[1]) + "' WHERE NICKNAME='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password for nick %s cannot be changed. Contact with an iRCop.", user->mNickName.c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password for nick %s has been changed to: %s", user->mNickName.c_str(), split[1].c_str()));
			return;
		}
	} else if (cmd == "LANG") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
			return;
		} else if (Server::HUBExiste() == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else {
			std::string lang = split[1];
			std::transform(lang.begin(), lang.end(), lang.begin(), ::tolower);
			if (lang != "es" && lang != "en" && lang != "ca" && lang != "gl") {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The language is not valid, the options are: %s.", "es, en, ca, gl"));
				return;
			}
			string sql = "UPDATE OPTIONS SET LANG='" + lang + "' WHERE NICKNAME='" + user->mNickName + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The language cannot be setted."));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			user->mLang = lang;
			user->Send(":" + config->Getvalue("nickserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The language has been setted to: %s.", lang.c_str()));
			return;
		}
	}
	return;
}

void NickServ::UpdateLogin (LocalUser *user) {
	if (Server::HUBExiste() == false) return;

	string sql = "UPDATE NICKS SET LASTUSED=" + std::to_string(time(0)) + " WHERE NICKNAME='" + user->mNickName + "';";
	if (DB::SQLiteNoReturn(sql) == false) return;
	if (config->Getvalue("cluster") == "false") {
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Server::Send(sql);
	}
	return;
}

bool NickServ::IsRegistered(string nickname) {
	if (nickname.empty())
		return false;
	string sql = "SELECT NICKNAME from NICKS WHERE NICKNAME='" + nickname + "';";
	string retorno = DB::SQLiteReturnString(sql);
	return (strcasecmp(retorno.c_str(), nickname.c_str()) == 0);
} 

bool NickServ::Login (const string &nickname, const string &pass) {
	string sql = "SELECT PASS from NICKS WHERE NICKNAME='" + nickname + "';";
	string retorno = DB::SQLiteReturnString(sql);
	return (retorno == sha256(pass));
}

int NickServ::GetNicks () {
	string sql = "SELECT COUNT(*) FROM NICKS;";
	return DB::SQLiteReturnInt(sql);
}

bool NickServ::GetOption(const string &option, string nickname) {
	if (IsRegistered(nickname) == false)
		return false;
	string sql = "SELECT " + option + " FROM OPTIONS WHERE NICKNAME='" + nickname + "';";
	return DB::SQLiteReturnInt(sql);
}

std::string NickServ::GetLang(string nickname) {
	if (IsRegistered(nickname) == false)
		return "";
	string sql = "SELECT LANG FROM OPTIONS WHERE NICKNAME='" + nickname + "';";
	return DB::SQLiteReturnString(sql);
}

string NickServ::GetvHost (string nickname) {
	if (IsRegistered(nickname) == false)
		return "";
	string sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + nickname + "';";
	return DB::SQLiteReturnString(sql);
}

int NickServ::MemoNumber(const std::string& nick) {
	int i = 0;
	Memos::iterator it = MemoMsg.begin();
	for(; it != MemoMsg.end(); ++it)
		if (strcasecmp((*it)->receptor.c_str(), nick.c_str()) == 0)
			i++;
	return i;
}

void NickServ::checkmemos(LocalUser* user) {
	if (user->getMode('r') == false)
		return;
    Memos::iterator it = MemoMsg.begin();
    while (it != MemoMsg.end()) {
		if (strcasecmp((*it)->receptor.c_str(), user->mNickName.c_str()) == 0) {
			struct tm tm;
			localtime_r(&(*it)->time, &tm);
			char date[32];
			strftime(date, sizeof(date), "%r %d-%m-%Y", &tm);
			string fecha = date;
			user->Send(":" + config->Getvalue("nickserv") + " PRIVMSG " + user->mNickName + " :" + Utils::make_string(user->mLang, "Memo from: %s Received %s Message: %s", (*it)->sender.c_str(), fecha.c_str(), (*it)->mensaje.c_str()));
			it = MemoMsg.erase(it);
		} else
			++it;
    }
    Server::Send("MEMODEL " + user->mNickName);
}
