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

extern OperSet miRCOps;

class CMD_OS : public Module
{
	public:
	CMD_OS() : Module("OS", 50, false) {};
	~CMD_OS() {};
	virtual void command(User *user, std::string message) override {
	
		if (user->getMode('o') == false) {
			user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not have access."));
			return;
		}
	
		message=message.substr(message.find_first_of(" \t")+1);
		
		std::vector<std::string> split;
		Utils::split(message, split, " ");
		
		if (split.size() == 0)
			return;

		std::string cmd = split[0];
		std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
		
		if (cmd == "HELP") {
			if (split.size() == 1) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /operserv gline|tgline|kill|drop|setpass|spam|oper|exceptions ]");
				return;
			} else if (split.size() > 1) {
				std::string comando = split[1];
				std::transform(comando.begin(), comando.end(), comando.begin(), ::toupper);
				if (comando == "GLINE") {
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /operserv gline <add|del|list> [ip] [reason] ]");
					return;
				} else if (comando == "TGLINE") {
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /operserv tgline <add|del|list> [ip] [time] [reason] ]");
					return;
				} else if (comando == "KILL") {
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /operserv kill <nick> ]");
					return;
				} else if (comando == "DROP") {
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /operserv drop <nick|#channel> ]");
					return;
				} else if (comando == "SETPASS") {
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /operserv setpass <nick> <password> ]");
					return;
				} else if (comando == "SPAM") {
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /operserv spam <add|del|list> [mask] [CPNE] [reason] ]");
					return;
				} else if (comando == "OPER") {
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /operserv oper <add|del|list> [nick] ]");
					return;
				} else if (comando == "EXCEPTIONS") {
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :[ /operserv exceptions <add|del|list> [ip] [clon|dnsbl|channel|geoip] [amount] ]");
					return;
				} else {
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no help for that command."));
					return;
				}
			}
		} else if (cmd == "GLINE") {
			if (split.size() < 2) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				if (strcasecmp(split[1].c_str(), "ADD") == 0) {
					if (split.size() < 4) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					} else if (OperServ::IsGlined(split[2]) == true) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The GLINE already exists."));
						return;
					} else if (DB::EscapeChar(split[2]) == true) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The GLINE contains non-valid characters."));
						return;
					}
					int length = 7 + split[1].length() + split[2].length();
					std::string motivo = message.substr(length);
					std::string sql = "INSERT INTO GLINE VALUES ('" + split[2] + "', '" + motivo + "', '" + user->mNickName + "');";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record can not be inserted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					for (auto it = Users.begin(); it != Users.end();) {
						if ((*it).second->mHost == split[2]) {
							if ((*it).second->is_local == true)
								(*it).second->Exit(true);
							else {
								(*it).second->QUIT();
								Server::Send("SKILL " + (*it).second->mNickName);
							}
							it = Users.erase(it);
							continue;
						}
						it++;
					}
					Oper oper;
					oper.GlobOPs("Se ha insertado el GLINE a la IP " + split[2] + " por " + user->mNickName + ". Motivo: " + motivo);
				} else if (strcasecmp(split[1].c_str(), "DEL") == 0) {
					if (split.size() < 3) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					}
					if (OperServ::IsGlined(split[2]) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is not GLINE with this IP."));
						return;
					}
					std::string sql = "DELETE FROM GLINE WHERE IP='" + split[2] + "';";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The GLINE has been removed."));
				} else if (strcasecmp(split[1].c_str(), "LIST") == 0) {
					vector<vector<string> > result;
					string sql = "SELECT IP, NICK, MOTIVO FROM GLINE ORDER BY IP;";
					result = DB::SQLiteReturnVectorVector(sql);
					if (result.size() == 0) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no GLINES."));
						return;
					}
					for(vector<vector<string> >::iterator it = result.begin(); it != result.end(); ++it)
					{
						vector<string> row = *it;
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "\002%s\002 by %s. Reason: %s", row.at(0).c_str(), row.at(1).c_str(), row.at(2).c_str()));
					}
					return;
				}
				return;
			}
		} else if (cmd == "TGLINE") {
			if (split.size() < 2) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				if (strcasecmp(split[1].c_str(), "ADD") == 0) {
					if (split.size() < 5) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					} else if (OperServ::IsTGlined(split[2]) == true) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The TGLINE already exists."));
						return;
					} else if (DB::EscapeChar(split[2]) == true) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The TGLINE contains non-valid characters."));
						return;
					}
					int length = 10 + split[1].length() + split[2].length() + split[3].length();
					std::string motivo = message.substr(length);
					time_t tiempo = Utils::UnixTime(split[3]);
					if (tiempo < 1) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The time is wrong."));
						return;
					}
					std::string sql = "INSERT INTO TGLINE VALUES ('" + split[2] + "', '" + motivo + "', '" + user->mNickName + "', " + std::to_string(time(0)) + ", " + std::to_string(tiempo) + ");";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record can not be inserted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					auto it = Users.begin();
					for (; it != Users.end(); it++) {
						if ((*it).second->mHost == split[2]) {
							if ((*it).second->is_local == true)
								(*it).second->Exit(true);
							else {
								(*it).second->QUIT();
								Server::Send("SKILL " + (*it).second->mNickName);
							}
						}
					}
					Oper oper;
					oper.GlobOPs("Se ha insertado el TGLINE a la IP " + split[2] + " por " + user->mNickName + ". Motivo: " + motivo);
				} else if (strcasecmp(split[1].c_str(), "DEL") == 0) {
					if (split.size() < 3) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					}
					if (OperServ::IsTGlined(split[2]) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is not TGLINE with this IP."));
						return;
					}
					std::string sql = "DELETE FROM TGLINE WHERE IP='" + split[2] + "';";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The TGLINE has been removed."));
				} else if (strcasecmp(split[1].c_str(), "LIST") == 0) {
					vector<vector<string> > result;
					string sql = "SELECT IP, NICK, MOTIVO, TIME, EXPIRE FROM TGLINE ORDER BY IP;";
					result = DB::SQLiteReturnVectorVector(sql);
					if (result.size() == 0) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no TGLINES."));
						return;
					}
					for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
					{
						vector<string> row = *it;
						std::string expire;
						time_t tiempo = (time_t ) stoi(row.at(3)) + (time_t ) stoi(row.at(4)) - time(0);
						if (tiempo < 1)
							expire = "now";
						else
							expire = Utils::PartialTime(tiempo);
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "\002%s\002 by %s. Expires on: %s. Reason: %s", row.at(0).c_str(), row.at(1).c_str(), expire.c_str(), row.at(2).c_str()));
					}
					return;
				}
				return;
			}
		} else if (cmd == "KILL") {
			Oper oper;
			if (split.size() < 2) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			}
			if (User::FindUser(split[1]) == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is offline.", split[1].c_str()));
				return;
			}
			else {
				User *u = User::GetUser(split[1]);
				if (u->is_local == true)
					u->Exit(true);
				else {
					u->QUIT();
					Server::Send("SKILL " + u->mNickName);
				}
				oper.GlobOPs(Utils::make_string("", "The nick %s has been KILLed by %s.", u->mNickName.c_str(), user->mNickName.c_str()));
			}
			return;
		} else if (cmd == "DROP") {
			if (split.size() < 2) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (NickServ::IsRegistered(split[1]) == 0 && ChanServ::IsRegistered(split[1]) == 0) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick/channel is not registered."));
				return;
			} else if (Server::HUBExiste() == 0) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else if (NickServ::IsRegistered(split[1]) == 1) {
				std::string sql = "DELETE FROM NICKS WHERE NICKNAME='" + split[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + split[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM CANALES WHERE OWNER='" + split[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM ACCESS WHERE USUARIO='" + split[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				if (User::FindUser(split[1]) == true) {
					User *u = User::GetUser(split[1]);
					if (u->is_local == true) {
						u->deliver(":" + config["serverName"].as<std::string>() + " MODE " + user->mNickName + " -r");
						u->setMode('r', false);
					}
					Server::Send("UMODE " + split[1] + " -r");
				}
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s has been deleted.", split[1].c_str()));
				return;
			} else if (ChanServ::IsRegistered(split[1]) == 1) {
				std::string sql = "DELETE FROM CANALES WHERE NOMBRE='" + split[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":CHaN!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM ACCESS WHERE CANAL='" + split[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM AKICK WHERE CANAL='" + split[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				user->deliver(":CHaN!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel %s has been deleted.", split[1].c_str()));
				Channel* chan = Channel::GetChannel(split[1]);
				if (chan) {
					if (chan->getMode('r') == true) {
						chan->setMode('r', false);
						chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name + " -r");
						Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name + " -r");
					}
				}
			}
		} else if (cmd == "SETPASS") {
			if (split.size() < 3) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (NickServ::IsRegistered(split[1]) == 0) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", split[1].c_str()));
				return;
			} else if (Server::HUBExiste() == 0) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else if (NickServ::IsRegistered(split[1]) == 1) {
				string sql = "UPDATE NICKS SET PASS='" + sha256(split[2]) + "' WHERE NICKNAME='" + split[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->deliver(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password for nick %s cannot be changed. Contact with an iRCop.", split[1].c_str()));
					return;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password for nick %s has been changed to: %s", split[1].c_str(), split[2].c_str()));
				return;
			}
		} else if (cmd == "SPAM") {
			if (split.size() < 2) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				if (strcasecmp(split[1].c_str(), "ADD") == 0) {
					Oper oper;
					if (split.size() < 5) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					} else if (OperServ::IsSpammed(split[2]) == true && (split[3] != "E" && split[3] != "e")) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The SPAM already exists."));
						return;
					} else if (DB::EscapeChar(split[2]) == true) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The SPAM contains non-valid characters."));
						return;
					}
					std::transform(split[3].begin(), split[3].end(), split[3].begin(), ::tolower);
					std::string reason;
					for (unsigned int i = 4; i < split.size(); i++) reason.append(split[i] + " ");
					std::string sql = "INSERT INTO SPAM VALUES ('" + split[2] + "', '" + user->mNickName + "', '" + reason + "', '" + split[3] + "');";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record can not be inserted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					oper.GlobOPs(Utils::make_string("", "SPAM has been added to MASK: %s by nick %s.", split[2].c_str(), user->mNickName.c_str()));
				} else if (strcasecmp(split[1].c_str(), "DEL") == 0) {
					Oper oper;
					if (split.size() < 3) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					}
					if (OperServ::IsSpammed(split[2]) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is not SPAM with this MASK."));
						return;
					}
					std::string sql = "DELETE FROM SPAM WHERE MASK='" + split[2] + "';";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					oper.GlobOPs(Utils::make_string("", "SPAM has been deleted to MASK: %s by nick %s.", split[2].c_str(), user->mNickName.c_str()));
				} else if (strcasecmp(split[1].c_str(), "LIST") == 0) {
					vector<vector<string> > result;
					string sql = "SELECT MASK, WHO, TARGET, MOTIVO FROM SPAM ORDER BY WHO;";
					result = DB::SQLiteReturnVectorVector(sql);
					if (result.size() == 0)
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no SPAM."));
					for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
					{
						vector<string> row = *it;
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "\002%s\002 by %s. Flags: %s Reason: %s", row.at(0).c_str(), row.at(1).c_str(), row.at(2).c_str(), row.at(3).c_str()));
					}
					return;
				}
				return;
			}
		} else if (cmd == "OPER") {
			if (split.size() < 2) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				if (strcasecmp(split[1].c_str(), "ADD") == 0) {
					Oper oper;
					if (split.size() < 3) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					} if (NickServ::IsRegistered(split[2]) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", split[2].c_str()));
						return;
					} else if (OperServ::IsOper(split[2]) == true) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The iRCop already exists."));
						return;
					}
					std::string sql = "INSERT INTO OPERS VALUES ('" + split[2] + "', '" + user->mNickName + "', " + std::to_string(time(0)) + ");";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record can not be inserted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					if (User::FindUser(split[2]) == true) {
						User *u = User::GetUser(split[2]);
						if (u->is_local == true) {
							if (u->getMode('o') == false) {
								u->deliver(":" + config["serverName"].as<std::string>() + " MODE " + u->mNickName + " +o");
								u->setMode('o', true);
								Server::Send("UMODE " + u->mNickName + " +o");
								miRCOps.insert(u->mNickName);
							}
						} else {
							if (u->getMode('o') == false) {
								u->setMode('o', true);
								Server::Send("UMODE " + u->mNickName + " +o");
								miRCOps.insert(u->mNickName);
							}
						}
					}
					oper.GlobOPs(Utils::make_string("", "OPER %s inserted by nick: %s.", split[2].c_str(), user->mNickName.c_str()));
				} else if (strcasecmp(split[1].c_str(), "DEL") == 0) {
					Oper oper;
					if (split.size() < 3) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					}
					if (OperServ::IsOper(split[2]) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is not OPER with such nick."));
						return;
					}
					std::string sql = "DELETE FROM OPERS WHERE NICK='" + split[2] + "';";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					if (User::FindUser(split[2]) == true) {
						User *u = User::GetUser(split[2]);
						if (u->is_local == true) {
							if (u->getMode('o') == true) {
								u->deliver(":" + config["serverName"].as<std::string>() + " MODE " + u->mNickName + " -o");
								u->setMode('o', false);
								Server::Send("UMODE " + u->mNickName + " -o");
								miRCOps.erase(u->mNickName);
							}
						} else {
							if (u->getMode('o') == true) {
								u->setMode('o', false);
								Server::Send("UMODE " + u->mNickName + " -o");
								miRCOps.erase(u->mNickName);
							}
						}
					}
					oper.GlobOPs(Utils::make_string("", "OPER %s deleted by nick: %s.", split[2].c_str(), user->mNickName.c_str()));
				} else if (strcasecmp(split[1].c_str(), "LIST") == 0) {
					vector<vector<string> > result;
					string sql = "SELECT NICK, OPERBY, TIEMPO FROM OPERS ORDER BY NICK;";
					result = DB::SQLiteReturnVectorVector(sql);
					if (result.size() == 0) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no OPERs."));
						return;
					}
					for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
					{
						vector<string> row = *it;
						time_t time = stoi(row.at(2));
						std::string cuando = Utils::Time(time);
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "\002%s\002 opered by %s since: %s", row.at(0).c_str(), row.at(1).c_str(), cuando.c_str()));
					}
				}
			}
		} else if (cmd == "EXCEPTIONS") {
			if (split.size() < 2) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
				return;
			} else if (Server::HUBExiste() == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
				return;
			} else if (user->getMode('r') == false) {
				user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
				return;
			} else {
				if (strcasecmp(split[1].c_str(), "ADD") == 0) {
					Oper oper;
					if (split.size() < 5) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					} else if (strcasecmp(split[3].c_str(), "clon") != 0 && strcasecmp(split[3].c_str(), "dnsbl") != 0 && strcasecmp(split[3].c_str(), "channel") != 0 && strcasecmp(split[3].c_str(), "geoip") != 0) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Incorrect EXCEPTION ( only allowed: clon, dnsbl, channel, geoip )"));
						return;
					} else if (OperServ::IsException(split[2], split[3]) == true) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The exception already exists."));
						return;
					} else if (split[4].empty() || split[4].find_first_not_of("0123456789") != string::npos) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Incorrect EXCEPTION ( the parameter must be a number )"));
						return;
					}
					std::transform(split[3].begin(), split[3].end(), split[3].begin(), ::tolower);
					std::string sql = "INSERT INTO EXCEPTIONS VALUES ('" + split[2] + "', '" + split[3] + "', " + split[4] + ", '" + user->mNickName + "', " + std::to_string(time(0)) + ");";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record can not be inserted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					oper.GlobOPs(Utils::make_string("", "EXCEPTION %s inserted by nick: %s.", split[2].c_str(), user->mNickName.c_str()));
				} else if (strcasecmp(split[1].c_str(), "DEL") == 0) {
					Oper oper;
					if (split.size() < 4) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					}
					std::transform(split[3].begin(), split[3].end(), split[3].begin(), ::tolower);
					if (OperServ::IsException(split[2], split[3]) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is not EXCEPTION with such IP."));
						return;
					}
					std::string sql = "DELETE FROM EXCEPTIONS WHERE IP='" + split[2] + "'  AND OPTION='" + split[3] + "';";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
						return;
					}
					if (config["database"]["cluster"].as<bool>() == false) {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Server::Send(sql);
					}
					oper.GlobOPs(Utils::make_string("", "EXCEPTION %s deleted by nick: %s.", split[2].c_str(), user->mNickName.c_str()));
				} else if (strcasecmp(split[1].c_str(), "LIST") == 0) {
					if (split.size() < 3) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
						return;
					}
					vector<vector<string> > result;
					std::replace( split[2].begin(), split[2].end(), '*', '%');
					string sql = "SELECT IP, ADDED, DATE, OPTION, VALUE FROM EXCEPTIONS WHERE IP LIKE '" + split[2] + "'  ORDER BY IP;";
					result = DB::SQLiteReturnVectorVector(sql);
					if (result.size() == 0) {
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no EXCEPTIONS."));
						return;
					}
					for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
					{
						vector<string> row = *it;
						time_t time = stoi(row.at(2));
						std::string cuando = Utils::Time(time);
						user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "\002%s\002 added by %s option: %s value: %s since: %s", row.at(0).c_str(), row.at(1).c_str(), row.at(3).c_str(), row.at(4).c_str(), cuando.c_str()));
					}
				}
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_OS);
}
