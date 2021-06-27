#include "ZeusiRCd.h"
#include "module.h"
#include "db.h"
#include "Server.h"
#include "oper.h"
#include "Utils.h"
#include "services.h"
#include "base64.h"
#include "Config.h"
#include "sha256.h"

#include <regex>

using namespace std;

extern Memos MemoMsg;
extern std::map<std::string, unsigned int> bForce;

class CMD_NS : public Module
{
	public:
	CMD_NS() : Module("NS", 50, false) {};
	~CMD_NS() {};
	virtual void command(User *user, std::string message) override {
		message=message.substr(message.find_first_of(" \t")+1);
		std::vector<std::string> split;
		Utils::split(message, split, " ");
		
		if (split.size() == 0)
			return;

		std::string cmd = split[0];
		std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
		
		if (cmd == "HELP") {
			if (split.size() == 1) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv register|drop|email|url|noaccess|nomemo|noop|showmail|onlyreg|password|lang|nocolor ]");
				return;
			} else if (split.size() > 1) {
				std::string comando = split[1];
				std::transform(comando.begin(), comando.end(), comando.begin(), ::toupper);
				if (comando == "REGISTER") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv register <yourpassword> ]");
					return;
				} else if (comando == "DROP") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv drop <yourpassword> ]");
					return;
				} else if (comando == "EMAIL") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv email <your@email.tld|off> ]");
					return;
				} else if (comando == "URL") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv url <your.web.site|off> ]");
					return;
				} else if (comando == "NOACCESS") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv noaccess <nick> ]");
					return;
				} else if (comando == "NOMEMO") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv nomemo <on|off> ]");
					return;
				} else if (comando == "NOOP") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv noop <on|off> ]");
					return;
				} else if (comando == "SHOWMAIL") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv showmail <on|off> ]");
					return;
				} else if (comando == "ONLYREG") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv onlyreg <on|off> ]");
					return;
				} else if (comando == "PASSWORD") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv password <newpassword> ]");
					return;
				} else if (comando == "LANG") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv lang <language> ]");
					return;
				} else if (comando == "NOCOLOR") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /nickserv nocolor <on|off> ]");
					return;
				} else {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no help for that command."));
					return;
				}
			}
		} else if (cmd == "REGISTER") {
			if (split.size() < 2) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (user->getMode('r') == true) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is already registered.", user->mNickName.c_str()));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else {
				if (DB::EscapeChar(split[1]) == true || split[1].find(":") != std::string::npos || split[1].find("!") != std::string::npos ) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password contains no valid characters (!:;')."));
					return;
				}
				string sql = "INSERT INTO NICKS VALUES ('" + user->mNickName + "', '" + sha256(split[1]) + "', '', '', '',  " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be registered. Please contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "INSERT INTO OPTIONS (NICKNAME, LANG) VALUES ('" + user->mNickName + "', '" + config["language"].as<std::string>() + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be registered. Please contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s has been registered.", user->mNickName.c_str()));
				if (user->getMode('r') == false) {
					user->deliver(":" + config["serverName"].as<std::string>() + " MODE " + user->mNickName + " +r");
					user->setMode('r', true);
					Server::Send("UMODE " + user->mNickName + " +r");
				}
				return;
			}
		} else if (cmd == "DROP") {
			if (split.size() < 2) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else if (NickServ::Login(user->mNickName, split[1]) == false) {
				bForce[user->mNickName]++;
				if ((bForce.find(user->mNickName)) != bForce.end()) {
					if (bForce.count(user->mNickName) >= 7) {
						user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Too much identify attempts for this nick. Try in 1 hour."));
						return;
					}
				} else
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Wrong password."));
				Utils::log(Utils::make_string("", "Wrong password for nick: %s entered password: %s", user->mNickName.c_str(), split[1].c_str()));
				return;
			} else {
				string sql = "DELETE FROM NICKS WHERE NICKNAME='" + user->mNickName + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + user->mNickName + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM CANALES WHERE OWNER='" + user->mNickName + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM ACCESS WHERE USUARIO='" + user->mNickName + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
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
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM OPERS WHERE NICK='" + user->mNickName + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s cannot be deleted. Please contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s has been deleted.", user->mNickName.c_str()));
				if (user->getMode('r') == true) {
					user->deliver(":" + config["serverName"].as<std::string>() + " MODE " + user->mNickName + " -r");
					user->setMode('r', false);
					Server::Send("UMODE " + user->mNickName + " -r");
				}
				return;
			}
		} else if (cmd == "EMAIL") {
			if (split.size() < 2) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
				return;
			} else if (Server::HUBExiste() == 0) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				string email;
				if (strcasecmp(split[1].c_str(), "OFF") == 0) {
					email = "";
				} else {
					email = split[1];
				}
				if (std::regex_match(email, std::regex("^[_a-z0-9-]+(.[_a-z0-9-]+)*@[a-z0-9-]+(.[a-z0-9-]+)*(.[a-z]{2,32})$")) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The email seems to be wrong."));
					return;
				}
				string sql = "UPDATE NICKS SET EMAIL='" + email + "' WHERE NICKNAME='" + user->mNickName + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The e-mail for nick %s cannot be changed. Contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				if (email.length() > 0)
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The e-mail for nick %s has been changed.", user->mNickName.c_str()));
				else
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The e-mail for nick %s has been deleted.", user->mNickName.c_str()));
				return;
			}
		} else if (cmd == "URL") {
			if (split.size() < 2) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
				return;
			} else if (Server::HUBExiste() == 0) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				string url;
				if (strcasecmp(split[1].c_str(), "OFF") == 0)
					url = "";
				else
					url = split[1];
				if (std::regex_match(url, std::regex("(ftp|http|https)://\\w+(\\.\\w+)+\\w+(\\/\\w+)*")) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The url seems to be wrong."));
					return;
				}
				string sql = "UPDATE NICKS SET URL='" + url + "' WHERE NICKNAME='" + user->mNickName + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The url for nick %s cannot be changed. Contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				if (url.length() > 0)
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your URL has changed."));
				else
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your URL has been deleted."));
				return;
			}
		} else if (cmd == "NOACCESS" || cmd == "SHOWMAIL" || cmd == "NOMEMO" || cmd == "NOOP" || cmd == "ONLYREG" || cmd == "NOCOLOR") {
			if (split.size() < 2) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
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
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The option %s cannot be changed.", cmd.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				if (option == 1)
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The option %s has been setted.", cmd.c_str()));
				else
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The option %s has been deleted.", cmd.c_str()));
				return;
			}
		} else if (cmd == "PASSWORD") {
			if (split.size() < 2) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				if (DB::EscapeChar(split[1]) == true || split[1].find(":") != std::string::npos || split[1].find("!") != std::string::npos ) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password contains no valid characters (!:;')."));
					return;
				}
				string sql = "UPDATE NICKS SET PASS='" + sha256(split[1]) + "' WHERE NICKNAME='" + user->mNickName + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password for nick %s cannot be changed. Contact with an iRCop.", user->mNickName.c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password for nick %s has been changed to: %s", user->mNickName.c_str(), split[1].c_str()));
				return;
			}
		} else if (cmd == "LANG") {
			if (split.size() < 2) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", user->mNickName.c_str()));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				std::string lang = split[1];
				std::transform(lang.begin(), lang.end(), lang.begin(), ::tolower);
				if (lang != "es" && lang != "en" && lang != "ca" && lang != "gl") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The language is not valid, the options are: %s.", "es, en, ca, gl"));
					return;
				}
				string sql = "UPDATE OPTIONS SET LANG='" + lang + "' WHERE NICKNAME='" + user->mNickName + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The language cannot be setted."));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				user->mLang = lang;
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The language has been setted to: %s.", lang.c_str()));
				return;
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_NS);
}
