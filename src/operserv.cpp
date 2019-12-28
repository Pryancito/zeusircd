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

#include "ZeusBaseClass.h"
#include "db.h"
#include "Server.h"
#include "oper.h"
#include "Utils.h"
#include "services.h"
#include "base64.h"
#include "Config.h"
#include "sha256.h"
#include "mainframe.h"

using namespace std;

extern OperSet miRCOps;

void OperServ::Message(LocalUser *user, string message) {
	std::vector<std::string> split;
	Config::split(message, split, " \t");
	
	if (split.size() == 0)
		return;

	std::string cmd = split[0];
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
	
	if (cmd == "HELP") {
		if (split.size() == 1) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :[ /operserv gline|tgline|kill|drop|setpass|spam|oper|exceptions ]");
			return;
		} else if (split.size() > 1) {
			std::string comando = split[1];
			std::transform(comando.begin(), comando.end(), comando.begin(), ::toupper);
			if (comando == "GLINE") {
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :[ /operserv gline <add|del|list> [ip] [reason] ]");
				return;
			} else if (comando == "TGLINE") {
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :[ /operserv tgline <add|del|list> [ip] [time] [reason] ]");
				return;
			} else if (comando == "KILL") {
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :[ /operserv kill <nick> ]");
				return;
			} else if (comando == "DROP") {
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :[ /operserv drop <nick|#channel> ]");
				return;
			} else if (comando == "SETPASS") {
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :[ /operserv setpass <nick> <password> ]");
				return;
			} else if (comando == "SPAM") {
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :[ /operserv spam <add|del|list> [mask] [CPNE] [reason] ]");
				return;
			} else if (comando == "OPER") {
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :[ /operserv oper <add|del|list> [nick] ]");
				return;
			} else if (comando == "EXCEPTIONS") {
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :[ /operserv exceptions <add|del|list> [ip] [clon|dnsbl|channel|geoip] [amount] ]");
				return;
			} else {
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no help for that command."));
				return;
			}
		}
	} else if (cmd == "GLINE") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (Server::HUBExiste() == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else {
			if (strcasecmp(split[1].c_str(), "ADD") == 0) {
				if (split.size() < 4) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				} else if (OperServ::IsGlined(split[2]) == true) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The GLINE already exists."));
					return;
				} else if (DB::EscapeChar(split[2]) == true) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The GLINE contains non-valid characters."));
					return;
				}
				int length = 7 + split[1].length() + split[2].length();
				std::string motivo = message.substr(length);
				std::string sql = "INSERT INTO GLINE VALUES ('" + split[2] + "', '" + motivo + "', '" + user->mNickName + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record can not be inserted."));
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				auto lusers = Mainframe::instance()->LocalUsers();
				for (auto it = lusers.begin(); it != lusers.end();) {
					if ((*it).second->mHost == split[2]) {
						(*it).second->Exit(true);
						it = lusers.erase(it);
						continue;
					}
					it++;
				}
				auto rusers = Mainframe::instance()->RemoteUsers();
				for (auto it2 = rusers.begin(); it2 != rusers.end();) {
					if ((*it2).second->mHost == split[2]) {
						Server::Send("SKILL " + (*it2).second->mNickName);
						it2 = rusers.erase(it2);
						continue;
					}
					it2++;
				}
				Oper oper;
				oper.GlobOPs("Se ha insertado el GLINE a la IP " + split[2] + " por " + user->mNickName + ". Motivo: " + motivo);
			} else if (strcasecmp(split[1].c_str(), "DEL") == 0) {
				if (split.size() < 3) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				}
				if (OperServ::IsGlined(split[2]) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is not GLINE with this IP."));
					return;
				}
				std::string sql = "DELETE FROM GLINE WHERE IP='" + split[2] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The GLINE has been removed."));
			} else if (strcasecmp(split[1].c_str(), "LIST") == 0) {
				vector<vector<string> > result;
				string sql = "SELECT IP, NICK, MOTIVO FROM GLINE ORDER BY IP;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no GLINES."));
					return;
				}
				for(vector<vector<string> >::iterator it = result.begin(); it != result.end(); ++it)
				{
					vector<string> row = *it;
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "\002%s\002 by %s. Reason: %s", row.at(0).c_str(), row.at(1).c_str(), row.at(2).c_str()));
				}
				return;
			}
			return;
		}
	} else if (cmd == "TGLINE") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (Server::HUBExiste() == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else {
			if (strcasecmp(split[1].c_str(), "ADD") == 0) {
				if (split.size() < 5) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				} else if (OperServ::IsTGlined(split[2]) == true) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The TGLINE already exists."));
					return;
				} else if (DB::EscapeChar(split[2]) == true) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The TGLINE contains non-valid characters."));
					return;
				}
				int length = 10 + split[1].length() + split[2].length() + split[3].length();
				std::string motivo = message.substr(length);
				time_t tiempo = Utils::UnixTime(split[3]);
				if (tiempo < 1) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The time is wrong."));
					return;
				}
				std::string sql = "INSERT INTO TGLINE VALUES ('" + split[2] + "', '" + motivo + "', '" + user->mNickName + "', " + std::to_string(time(0)) + ", " + std::to_string(tiempo) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record can not be inserted."));
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				auto local = Mainframe::instance()->LocalUsers();
				auto it = local.begin();
				for (; it != local.end(); it++) {
					if ((*it).second->mHost == split[2])
						(*it).second->Exit(true);
				}
				auto remote = Mainframe::instance()->RemoteUsers();
				auto it2 = remote.begin();
				for (; it2 != remote.end(); it2++) {
					if ((*it2).second->mHost == split[2])
						Server::Send("SKILL " + (*it2).second->mNickName);
				}
				Oper oper;
				oper.GlobOPs("Se ha insertado el TGLINE a la IP " + split[2] + " por " + user->mNickName + ". Motivo: " + motivo);
			} else if (strcasecmp(split[1].c_str(), "DEL") == 0) {
				if (split.size() < 3) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				}
				if (OperServ::IsTGlined(split[2]) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is not TGLINE with this IP."));
					return;
				}
				std::string sql = "DELETE FROM TGLINE WHERE IP='" + split[2] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The TGLINE has been removed."));
			} else if (strcasecmp(split[1].c_str(), "LIST") == 0) {
				vector<vector<string> > result;
				string sql = "SELECT IP, NICK, MOTIVO, TIME, EXPIRE FROM TGLINE ORDER BY IP;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no TGLINES."));
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
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "\002%s\002 by %s. Expires on: %s. Reason: %s", row.at(0).c_str(), row.at(1).c_str(), expire.c_str(), row.at(2).c_str()));
				}
				return;
			}
			return;
		}
	} else if (cmd == "KILL") {
		Oper oper;
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		}
		if (Mainframe::instance()->doesNicknameExists(split[1]) == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is offline.", split[1].c_str()));
			return;
		}
		else {
			LocalUser *lu = Mainframe::instance()->getLocalUserByName(split[1]);
			if (lu != nullptr) {
				lu->Exit(true);
				oper.GlobOPs(Utils::make_string("", "The nick %s has been KILLed by %s.", lu->mNickName.c_str(), user->mNickName.c_str()));
			} else {
				RemoteUser *ru = Mainframe::instance()->getRemoteUserByName(split[1]);
				if (ru != nullptr) {
					oper.GlobOPs(Utils::make_string("", "The nick %s has been KILLed by %s.", ru->mNickName.c_str(), user->mNickName.c_str()));
					ru->QUIT();
					Server::Send("SKILL " + ru->mNickName);
				}
			}
		}
		return;
	} else if (cmd == "DROP") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (NickServ::IsRegistered(split[1]) == 0 && ChanServ::IsRegistered(split[1]) == 0) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick/channel is not registered."));
			return;
		} else if (Server::HUBExiste() == 0) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else if (NickServ::IsRegistered(split[1]) == 1) {
			std::string sql = "DELETE FROM NICKS WHERE NICKNAME='" + split[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + split[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "DELETE FROM CANALES WHERE OWNER='" + split[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "DELETE FROM ACCESS WHERE USUARIO='" + split[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			if (Mainframe::instance()->doesNicknameExists(split[1]) == true) {
				LocalUser *u = Mainframe::instance()->getLocalUserByName(split[1]);
				if (u != nullptr) {
					u->Send(":" + config->Getvalue("serverName") + " MODE " + user->mNickName + " -r");
					u->setMode('r', false);
				}
				Server::Send("UMODE " + split[1] + " -r");
			}
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s has been deleted.", split[1].c_str()));
			return;
		} else if (ChanServ::IsRegistered(split[1]) == 1) {
			std::string sql = "DELETE FROM CANALES WHERE NOMBRE='" + split[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":CHaN!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "DELETE FROM ACCESS WHERE CANAL='" + split[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			sql = "DELETE FROM AKICK WHERE CANAL='" + split[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			user->Send(":CHaN!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel %s has been deleted.", split[1].c_str()));
			Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
			if (chan) {
				if (chan->getMode('r') == true) {
					chan->setMode('r', false);
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -r");
					Server::Send("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -r");
				}
			}
		}
	} else if (cmd == "SETPASS") {
		if (split.size() < 3) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (NickServ::IsRegistered(split[1]) == 0) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", split[1].c_str()));
			return;
		} else if (Server::HUBExiste() == 0) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else if (NickServ::IsRegistered(split[1]) == 1) {
			string sql = "UPDATE NICKS SET PASS='" + sha256(split[2]) + "' WHERE NICKNAME='" + split[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->Send(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password for nick %s cannot be changed. Contact with an iRCop.", split[1].c_str()));
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The password for nick %s has been changed to: %s", split[1].c_str(), split[2].c_str()));
			return;
		}
	} else if (cmd == "SPAM") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (Server::HUBExiste() == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else {
			if (strcasecmp(split[1].c_str(), "ADD") == 0) {
				Oper oper;
				if (split.size() < 5) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				} else if (OperServ::IsSpammed(split[2]) == true && (split[3] != "E" && split[3] != "e")) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The SPAM already exists."));
					return;
				} else if (DB::EscapeChar(split[2]) == true) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The SPAM contains non-valid characters."));
					return;
				}
				std::transform(split[3].begin(), split[3].end(), split[3].begin(), ::tolower);
				std::string reason;
				for (unsigned int i = 4; i < split.size(); i++) reason.append(split[i] + " ");
				std::string sql = "INSERT INTO SPAM VALUES ('" + split[2] + "', '" + user->mNickName + "', '" + reason + "', '" + split[3] + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record can not be inserted."));
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				oper.GlobOPs(Utils::make_string("", "SPAM has been added to MASK: %s by nick %s.", split[2].c_str(), user->mNickName.c_str()));
			} else if (strcasecmp(split[1].c_str(), "DEL") == 0) {
				Oper oper;
				if (split.size() < 3) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				}
				if (OperServ::IsSpammed(split[2]) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is not SPAM with this MASK."));
					return;
				}
				std::string sql = "DELETE FROM SPAM WHERE MASK='" + split[2] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config->Getvalue("cluster") == "false") {
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
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no SPAM."));
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "\002%s\002 by %s. Flags: %s Reason: %s", row.at(0).c_str(), row.at(1).c_str(), row.at(2).c_str(), row.at(3).c_str()));
				}
				return;
			}
			return;
		}
	} else if (cmd == "OPER") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (Server::HUBExiste() == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else {
			if (strcasecmp(split[1].c_str(), "ADD") == 0) {
				Oper oper;
				if (split.size() < 3) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				} if (NickServ::IsRegistered(split[2]) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s is not registered.", split[2].c_str()));
					return;
				} else if (OperServ::IsOper(split[2]) == true) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The iRCop already exists."));
					return;
				}
				std::string sql = "INSERT INTO OPERS VALUES ('" + split[2] + "', '" + user->mNickName + "', " + std::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record can not be inserted."));
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				if (Mainframe::instance()->doesNicknameExists(split[2]) == true) {
					LocalUser *u = Mainframe::instance()->getLocalUserByName(split[2]);
					if (u != nullptr) {
						if (u->getMode('o') == false) {
							u->Send(":" + config->Getvalue("serverName") + " MODE " + u->mNickName + " +o");
							u->setMode('o', true);
							Server::Send("UMODE " + u->mNickName + " +o");
							miRCOps.insert(u->mNickName);
						}
					} else {
						RemoteUser *u = Mainframe::instance()->getRemoteUserByName(split[2]);
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
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				}
				if (OperServ::IsOper(split[2]) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is not OPER with such nick."));
					return;
				}
				std::string sql = "DELETE FROM OPERS WHERE NICK='" + split[2] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				if (Mainframe::instance()->doesNicknameExists(split[2]) == true) {
					LocalUser *u = Mainframe::instance()->getLocalUserByName(split[2]);
					if (u != nullptr) {
						if (u->getMode('o') == true) {
							u->Send(":" + config->Getvalue("serverName") + " MODE " + u->mNickName + " -o");
							u->setMode('o', false);
							Server::Send("UMODE " + u->mNickName + " -o");
							miRCOps.erase(u->mNickName);
						}
					} else {
						RemoteUser *u = Mainframe::instance()->getRemoteUserByName(split[2]);
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
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no OPERs."));
					return;
				}
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					time_t time = stoi(row.at(2));
					std::string cuando = Utils::Time(time);
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "\002%s\002 opered by %s since: %s", row.at(0).c_str(), row.at(1).c_str(), cuando.c_str()));
				}
			}
		}
	} else if (cmd == "EXCEPTIONS") {
		if (split.size() < 2) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (Server::HUBExiste() == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The HUB doesnt exists, DBs are in read-only mode."));
			return;
		} else if (user->getMode('r') == false) {
			user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		} else {
			if (strcasecmp(split[1].c_str(), "ADD") == 0) {
				Oper oper;
				if (split.size() < 5) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				} else if (strcasecmp(split[3].c_str(), "clon") != 0 && strcasecmp(split[3].c_str(), "dnsbl") != 0 && strcasecmp(split[3].c_str(), "channel") != 0 && strcasecmp(split[3].c_str(), "geoip") != 0) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Incorrect EXCEPTION ( only allowed: clon, dnsbl, channel, geoip )"));
					return;
				} else if (OperServ::IsException(split[2], split[3]) == true) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The exception already exists."));
					return;
				} else if (split[4].empty() || split[4].find_first_not_of("0123456789") != string::npos) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "Incorrect EXCEPTION ( the parameter must be a number )"));
					return;
				}
				std::transform(split[3].begin(), split[3].end(), split[3].begin(), ::tolower);
				std::string sql = "INSERT INTO EXCEPTIONS VALUES ('" + split[2] + "', '" + split[3] + "', " + split[4] + ", '" + user->mNickName + "', " + std::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record can not be inserted."));
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				oper.GlobOPs(Utils::make_string("", "EXCEPTION %s inserted by nick: %s.", split[2].c_str(), user->mNickName.c_str()));
			} else if (strcasecmp(split[1].c_str(), "DEL") == 0) {
				Oper oper;
				if (split.size() < 4) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				}
				std::transform(split[3].begin(), split[3].end(), split[3].begin(), ::tolower);
				if (OperServ::IsException(split[2], split[3]) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is not EXCEPTION with such IP."));
					return;
				}
				std::string sql = "DELETE FROM EXCEPTIONS WHERE IP='" + split[2] + "'  AND OPTION='" + split[3] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The record cannot be deleted."));
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				oper.GlobOPs(Utils::make_string("", "EXCEPTION %s deleted by nick: %s.", split[2].c_str(), user->mNickName.c_str()));
			} else if (strcasecmp(split[1].c_str(), "LIST") == 0) {
				if (split.size() < 3) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
					return;
				}
				vector<vector<string> > result;
				std::replace( split[2].begin(), split[2].end(), '*', '%');
				string sql = "SELECT IP, ADDED, DATE, OPTION, VALUE FROM EXCEPTIONS WHERE IP LIKE '" + split[2] + "'  ORDER BY IP;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0) {
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "There is no EXCEPTIONS."));
					return;
				}
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					time_t time = stoi(row.at(2));
					std::string cuando = Utils::Time(time);
					user->Send(":" + config->Getvalue("operserv") + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "\002%s\002 added by %s option: %s value: %s since: %s", row.at(0).c_str(), row.at(1).c_str(), row.at(3).c_str(), row.at(4).c_str(), cuando.c_str()));
				}
			}
		}
	}
}

bool OperServ::IsGlined(string ip) {
	std::string sql = "SELECT IP from GLINE WHERE IP='" + ip + "';";
	std::string retorno = DB::SQLiteReturnString(sql);
	if (strcasecmp(retorno.c_str(), ip.c_str()) == 0)
		return true;
	else
		return false;
}

std::string OperServ::ReasonGlined(const string &ip) {
	std::string sql = "SELECT MOTIVO from GLINE WHERE IP='" + ip + "';";
	return DB::SQLiteReturnString(sql);
}

bool OperServ::IsTGlined(string ip) {
	std::string sql = "SELECT IP from TGLINE WHERE IP='" + ip + "';";
	std::string retorno = DB::SQLiteReturnString(sql);
	if (strcasecmp(retorno.c_str(), ip.c_str()) == 0)
		return true;
	else
		return false;
}

void OperServ::ExpireTGline () {
	std::string sql = "SELECT IP from TGLINE WHERE (TIME + EXPIRE) < " + std::to_string(time(0)) + ";";
	std::vector<std::string> ips = DB::SQLiteReturnVector(sql);

	for (unsigned int i = 0; i < ips.size(); i++) {
		Oper oper;
		std::string sql = "DELETE FROM TGLINE WHERE IP='" + ips[i] + "';";
		if (DB::SQLiteNoReturn(sql) == true)
			oper.GlobOPs("The TGLINE for ip: " + ips[i] + " has expired now.");
		else
			oper.GlobOPs("Error expiring TGLINE for ip: " + ips[i] + ".");
		if (config->Getvalue("cluster") == "false") {
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Server::Send(sql);
		}
	}
}

std::string OperServ::ReasonTGlined(const string &ip) {
	std::string sql = "SELECT MOTIVO from TGLINE WHERE IP='" + ip + "';";
	return DB::SQLiteReturnString(sql);
}

bool OperServ::IsOper(string nick) {
	std::string sql = "SELECT NICK from OPERS WHERE NICK='" + nick + "';";
	std::string retorno = DB::SQLiteReturnString(sql);
	if (strcasecmp(retorno.c_str(), nick.c_str()) == 0)
		return true;
	else
		return false;
}

bool OperServ::IsSpammed(string mask) {
	std::vector<std::string> vect;
	std::string sql = "SELECT MASK from SPAM;";
	vect = DB::SQLiteReturnVector(sql);
	std::transform(mask.begin(), mask.end(), mask.begin(), ::tolower);
	for (unsigned int i = 0; i < vect.size(); i++) {
		std::transform(vect[i].begin(), vect[i].end(), vect[i].begin(), ::tolower);
		if (Utils::Match(vect[i].c_str(), mask.c_str()) == true)
			return true;
	}
	return false;
}

bool OperServ::IsSpam(string mask, string flags) {
	std::vector<std::string> vect;
	std::transform(flags.begin(), flags.end(), flags.begin(), ::tolower);
	std::string sql = "SELECT MASK from SPAM WHERE TARGET LIKE '%" + flags + "%' COLLATE NOCASE;";
	vect = DB::SQLiteReturnVector(sql);
	std::transform(mask.begin(), mask.end(), mask.begin(), ::tolower);
	for (unsigned int i = 0; i < vect.size(); i++) {
		std::transform(vect[i].begin(), vect[i].end(), vect[i].begin(), ::tolower);
		if (Utils::Match(vect[i].c_str(), mask.c_str()) == true)
			return true;
	}
	return false;
}

int OperServ::IsException(std::string ip, std::string option) {
	std::transform(option.begin(), option.end(), option.begin(), ::tolower);
	std::string sql = "SELECT VALUE from EXCEPTIONS WHERE IP='" + ip + "'  AND OPTION='" + option + "';";
	return DB::SQLiteReturnInt(sql);
}

bool OperServ::CanGeoIP(std::string ip) {
	std::vector<std::string> vect;
	int valor = IsException(ip, "geoip");
	if (valor > 0)
		return true;
	std::string allowed = config->Getvalue("GeoIP-ALLOWED");
	std::string country = Utils::GetGeoIP(ip);
	if (allowed.length() > 0) {
		Config::split(allowed, vect, " ,");
		for (unsigned int i = 0; i < vect.size(); i++)
			if (strcasecmp(country.c_str(), vect[i].c_str()) == 0)
				return true;
		return false;
	}
	std::string denied = config->Getvalue("GeoIP-DENIED");
	if (denied.length() > 0) {
		Config::split(denied, vect, " ,");
		for (unsigned int i = 0; i < vect.size(); i++)
			if (strcasecmp(country.c_str(), vect[i].c_str()) == 0)
				return false;
		return true;
	}
	return true;
}
