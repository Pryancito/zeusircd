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

using namespace std;

class CMD_HS : public Module
{
	public:
	CMD_HS() : Module("HS", 50, false) {};
	~CMD_HS() {};
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
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /hs register|drop|transfer|request|accept|off|list ]");
				return;
			} else if (split.size() > 1) {
				std::string comando = split[1];
				std::transform(comando.begin(), comando.end(), comando.begin(), ::toupper);
				if (comando == "REGISTER") {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /hs register <virtual{/host}> ]");
					return;
				} else if (comando == "DROP") {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /hs drop <virtual{/host}> ]");
					return;
				} else if (comando == "TRANSFER") {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /hs transfer <virtual/host> <nick> ]");
					return;
				} else if (comando == "REQUEST") {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /hs request <virtual/host> ]");
					return;
				} else if (comando == "ACCEPT") {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /hs accept <nick> ]");
					return;
				} else if (comando == "OFF") {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /hs off ]");
					return;
				} else if (comando == "LIST") {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /hs list [*search*] ]");
					return;
				} else {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no help for that command."));
					return;
				}
			}
		} else if (cmd == "REGISTER") {
			if (split.size() < 2) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				string owner;
				if (split.size() == 2)
					owner = user->mNickName;
				else if (user->getMode('o') == true)
					owner = split[2];
				else
					owner = user->mNickName;
					
				if (Utils::checknick(owner) == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick contains no-valid characters."));
					return;
				} else if (NickServ::IsRegistered(owner) == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", owner.c_str()));
					return;
				} else if (HostServ::CheckPath(split[1]) == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The path %s is not valid.", split[1].c_str()));
					return;
				} else if (HostServ::Owns(user, split[1]) == false && split[1].find("/") != std::string::npos) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not own the path %s", split[1].c_str()));
					return;
				} else if (HostServ::HowManyPaths(owner) >= 40) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s can not register more paths.", owner.c_str()));
					return;
				} else {
					string sql = "SELECT PATH from PATHS WHERE PATH='" + split[1] + "';";
					if (strcasecmp(DB::SQLiteReturnString(sql).c_str(), split[1].c_str()) == 0) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The path %s is already registered.", split[1].c_str()));
						return;
					}
					sql = "SELECT VHOST from NICKS WHERE VHOST='" + split[1] + "';";
					if (strcasecmp(DB::SQLiteReturnString(sql).c_str(), split[1].c_str()) == 0) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The path %s is already registered.", split[1].c_str()));
						return;
					}
					sql = "INSERT INTO PATHS VALUES ('" + owner + "', '" + split[1] + "');";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Path %s can not be registered.", split[1].c_str()));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Path %s has been registered by %s.", split[1].c_str(), owner.c_str()));
					return;
				}
			}
		} else if (cmd == "DROP") {
			if (split.size() < 2) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				if (HostServ::CheckPath(split[1]) == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The path %s is not valid.", split[1].c_str()));
					return;
				} else if (HostServ::Owns(user, split[1]) == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not own the path %s", split[1].c_str()));
					return;
				} else {
					if (HostServ::DeletePath(split[1]) == true)
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Path %s has been deleted.", split[1].c_str()));
					else
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Path %s can not be deleted.", split[1].c_str()));
					return;
				}
			}
		} else if (cmd == "TRANSFER") {
			if (split.size() < 3) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else {
				string owner = split[2];
				if (Utils::checknick(owner) == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick contains no-valid characters."));
					return;
				} else if (NickServ::IsRegistered(owner) == 0) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", owner.c_str()));
					return;
				} else if (user->getMode('r') == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
					return;
				} else if (HostServ::CheckPath(split[1]) == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The path %s is not valid.", split[1].c_str()));
					return;
				} else if (HostServ::Owns(user, split[1]) == false && split[1].find("/") != std::string::npos) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not own the path %s", split[1].c_str()));
					return;
				} else if (HostServ::HowManyPaths(owner) >= 40) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s can not register more paths.", owner.c_str()));
					return;
				} else {
					string sql = "UPDATE PATHS SET OWNER='" + owner + "' WHERE PATH='" + split[1] + "';";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The owner of path %s can not be changed.", split[1].c_str()));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The owner of path %s has changed to: %s.", split[1].c_str(), owner.c_str()));
					return;
				}
			}
		} else if (cmd == "REQUEST") {
			if (split.size() < 2) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else {
				if (HostServ::CheckPath(split[1]) == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The path %s is not valid.", split[1].c_str()));
					return;
				} else if (HostServ::GotRequest(user->mNickName) == true && strcasecmp(split[1].c_str(), "OFF") != 0) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "You already have a vHost request."));
					return;
				} else if (HostServ::PathIsInvalid(split[1]) == true && strcasecmp(split[1].c_str(), "OFF") != 0) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The path %s is not valid.", split[1].c_str()));
					return;
				} else if (HostServ::IsReqRegistered(split[1]) == false && strcasecmp(split[1].c_str(), "OFF") != 0) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The path %s is not valid.", split[1].c_str()));
					return;
				} else if (user->getMode('r') == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
					return;
				} else if (strcasecmp(split[1].c_str(), "OFF") == 0) {
					if (HostServ::GotRequest(user->mNickName) == false) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not have a vHost request."));
						return;
					}
					string sql = "DELETE FROM REQUEST WHERE OWNER='" + user->mNickName + "';";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your request can not be deleted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your request has been deleted."));
					return;
				} else {
					string sql = "SELECT VHOST from NICKS WHERE VHOST='" + split[1] + "';";
					if (strcasecmp(DB::SQLiteReturnString(sql).c_str(), split[1].c_str()) == 0) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The path %s is already registered.", split[1].c_str()));
						return;
					}
					sql = "INSERT INTO REQUEST VALUES ('" + user->mNickName + "', '" + split[1] + "', " + std::to_string(time(0)) + ");";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your request can not be registered."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your request has been registered successfully."));
					return;
				}
			}
		} else if (cmd == "ACCEPT") {
			if (split.size() < 2) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else {
				if (Utils::checknick(split[1]) == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick contains no-valid characters."));
					return;
				} else if (NickServ::IsRegistered(split[1]) == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", split[1].c_str()));
					return;
				} else if (user->getMode('r') == false) {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
					return;
				} else if (HostServ::GotRequest(split[1]) == false) {
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s does not have a vHost request.", split[1].c_str()));
					return;
				} else {
					string sql = "SELECT PATH from REQUEST WHERE OWNER='" + split[1] + "';";
					string path = DB::SQLiteReturnString(sql);
					sql = "DELETE FROM REQUEST WHERE OWNER='" + split[1] + "';";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your request can not be finished."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					sql = "UPDATE NICKS SET VHOST='" + path + "' WHERE NICKNAME='" + split[1] + "';";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your request can not be finished."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your request has finished successfully."));
					if (User::FindUser(split[1]) == true) {
						User*  target = User::GetUser(split[1]);
						if (target) {
							target->Cycle();
						}
						Server::Send("VHOST " + split[1]);
					}
					return;
				}
			}
		} else if (cmd == "OFF") {
			if (Server::HUBExiste() == false) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else if (NickServ::GetvHost(user->mNickName) == "" && split.size() != 2) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your nick does not have a vHost set."));
				return;
			} else {
				string sql;
				if (user->getMode('o') == true && split.size() == 2) {
					if (NickServ::IsRegistered(split[1]) == true) {
						sql = "UPDATE NICKS SET VHOST='' WHERE NICKNAME='" + split[1] + "';";
						if (DB::SQLiteNoReturn(sql) == false) {
							user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your deletion of vHost cannot be ended."));
							return;
						}
						if (config["database"]["cluster"].as<bool>() == false) {
							sql = "DB " + DB::GenerateID() + " " + sql;
							DB::AlmacenaDB(sql);
							Server::Send(sql);
						}
						if (User::FindUser(split[1]) == true) {
							User*  target = User::GetUser(split[1]);
							if (target) {
								target->Cycle();
							}
							Server::Send("VHOST " + split[1]);
						}
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your request has finished successfully."));
					}
				} else {
					sql = "UPDATE NICKS SET VHOST='' WHERE NICKNAME='" + user->mNickName + "';";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your deletion of vHost cannot be ended."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your request has finished successfully."));
					user->Cycle();
				}
				return;
			}
		} else if (cmd == "LIST") {
			if (user->getMode('r') == false) {
				user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				if (split.size() == 1) {
					string sql = "SELECT PATH from PATHS WHERE OWNER='" + user->mNickName + "';";
					vector <string> paths = DB::SQLiteReturnVector(sql);
					for (unsigned int i = 0; i < paths.size(); i++) {
						vector<vector<string> > result;
						sql = "SELECT OWNER, PATH from REQUEST WHERE PATH LIKE '" + paths[i] + "%';";
						result = DB::SQLiteReturnVectorVector(sql);
						for(vector<vector<string> >::iterator it = result.begin(); it != result.end(); ++it)
						{
							vector<string> row = *it;
							user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :<" + row.at(0) + "> PATH: " + row.at(1));
						}
					}
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "End of LIST."));
					return;
				} else if (split.size() == 2) {
					string search = split[1];
					std::transform(search.begin(), search.end(), search.begin(), ::tolower);
					vector<vector<string> > result;
					string sql = "SELECT PATH, OWNER FROM PATHS ORDER BY OWNER;";
					result = DB::SQLiteReturnVectorVector(sql);
					for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
					{
						vector<string> row = *it;
						string path = row.at(0);
						std::transform(path.begin(), path.end(), path.begin(), ::tolower);
						if (Utils::Match(search.c_str(), path.c_str()) == 1)
							user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :<" + row.at(1) + "> PATH: " + row.at(0));
					}
					user->deliver(":" + config["hostserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "End of LIST."));
					return;
				}
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_HS);
}
