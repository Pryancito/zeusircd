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

#include <boost/algorithm/string/replace.hpp>

#include "db.h"
#include "server.h"
#include "oper.h"
#include "mainframe.h"
#include "utils.h"
#include "services.h"
#include "sha256.h"
#include "Bayes.h"

using namespace std;

extern OperSet miRCOps;
extern BayesClassifier *bayes;

void OperServ::Message(User *user, string message) {
	StrVec x;
	boost::trim_right(message);
	boost::split(x, message, boost::is_any_of(" \t"), boost::token_compress_on);
	std::string cmd = x[0];
	boost::to_upper(cmd);
	
	if (cmd == "HELP") {
		user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :[ /operserv gline|kill|drop|setpass|spam|oper|exceptions ]" + config->EOFMessage);
		return;
	} else if (cmd == "GLINE") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			if (boost::iequals(x[1], "ADD")) {
				if (x.size() < 4) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				} else if (OperServ::IsGlined(x[2]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The GLINE already exists.") + config->EOFMessage);
					return;
				} else if (DB::EscapeChar(x[2]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The GLINE contains non-valid characters.") + config->EOFMessage);
					return;
				}
				int length = 7 + x[1].length() + x[2].length();
				std::string motivo = message.substr(length);
				std::string sql = "INSERT INTO GLINE VALUES ('" + x[2] + "', '" + motivo + "', '" + user->nick() + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record can not be inserted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
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
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}
				if (OperServ::IsGlined(x[2]) == 0) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "There is not GLINE with this IP.") + config->EOFMessage);
					return;
				}
				std::string sql = "DELETE FROM GLINE WHERE IP='" + x[2] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The GLINE has been removed.") + config->EOFMessage);
			} else if (boost::iequals(x[1], "LIST")) {
				vector<vector<string> > result;
				string sql = "SELECT IP, NICK, MOTIVO FROM GLINE ORDER BY IP;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "There is no GLINES.") + config->EOFMessage);
					return;
				}
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "\002%s\002 by %s. Reason: %s", row.at(0).c_str(), row.at(1).c_str(), row.at(2).c_str()) + config->EOFMessage);
				}
				return;
			}
			return;
		}
	} else if (cmd == "KILL") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		}
		User* target = Mainframe::instance()->getUserByName(x[1]);
		if (!target) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is offline.", x[1].c_str()) + config->EOFMessage);
			return;
		}
		Oper oper;
		if (target) {
			if (target->server() == config->Getvalue("serverName")) {
				target->cmdQuit();
				oper.GlobOPs(Utils::make_string("", "The nick %s has been KILLed by %s.", target->nick().c_str(), user->nick().c_str()));
			} else {
				target->QUIT();
				oper.GlobOPs(Utils::make_string("", "The nick %s has been KILLed by %s.", target->nick().c_str(), user->nick().c_str()));
				Servidor::sendall("QUIT " + target->nick());
			}
		}
		return;
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(x[1]) == 0 && ChanServ::IsRegistered(x[1]) == 0) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick/channel is not registered.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(x[1]) == 1) {
			std::string sql = "DELETE FROM NICKS WHERE NICKNAME='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			sql = "DELETE FROM CANALES WHERE OWNER='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			sql = "DELETE FROM ACCESS WHERE USUARIO='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			User* target = Mainframe::instance()->getUserByName(x[1]);
			if (target) {
				if (target->server() == config->Getvalue("serverName")) {
					target->session()->send(":" + config->Getvalue("serverName") + " MODE " + user->nick() + " -r" + config->EOFMessage);
					target->setMode('r', false);
				} else
					Servidor::sendall("UMODE " + target->nick() + " -r");
			}
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s has been deleted.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == 1) {
			std::string sql = "DELETE FROM CANALES WHERE NOMBRE='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":CHaN!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			sql = "DELETE FROM ACCESS WHERE CANAL='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			sql = "DELETE FROM AKICK WHERE CANAL='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			user->session()->send(":CHaN!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s has been deleted.", x[1].c_str()) + config->EOFMessage);
			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			if (chan) {
				if (chan->getMode('r') == true) {
					chan->setMode('r', false);
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -r" + config->EOFMessage);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -r");
				}
			}
		}
	} else if (cmd == "SETPASS") {
		if (x.size() < 3) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(x[1]) == 0) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(x[1]) == 1) {
			string sql = "UPDATE NICKS SET PASS='" + sha256(x[2]) + "' WHERE NICKNAME='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The password for nick %s cannot be changed. Contact with an iRCop.", x[1].c_str()) + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The password for nick %s has been changed to: %s", x[1].c_str(), x[2].c_str()) + config->EOFMessage);
			return;
		}
	} else if (cmd == "SPAM") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			if (boost::iequals(x[1], "ADD")) {
				Oper oper;
				if (x.size() < 5) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				} else if (OperServ::IsSpammed(x[2]) == true && (x[3] != "E" && x[3] != "e")) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The SPAM already exists.") + config->EOFMessage);
					return;
				} else if (DB::EscapeChar(x[2]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The SPAM contains non-valid characters.") + config->EOFMessage);
					return;
				}
				boost::to_lower(x[3]);
				std::string reason;
				for (unsigned int i = 4; i < x.size(); i++) reason.append(x[i] + " ");
				std::string sql = "INSERT INTO SPAM VALUES ('" + x[2] + "', '" + user->nick() + "', '" + reason + "', '" + x[3] + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record can not be inserted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				if (x[3] == "E" || x[3] == "e")
					bayes->learn(0, x[2].c_str());
				else
					bayes->learn(1, x[2].c_str());
				oper.GlobOPs(Utils::make_string("", "SPAM has been added to MASK: %s by nick %s.", x[2].c_str(), user->nick().c_str()));
			} else if (boost::iequals(x[1], "DEL")) {
				Oper oper;
				if (x.size() < 3) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}
				if (OperServ::IsSpammed(x[2]) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "There is not SPAM with this MASK.") + config->EOFMessage);
					return;
				}
				std::string sql = "DELETE FROM SPAM WHERE MASK='" + x[2] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				bayes->learn(0, x[2].c_str());
				oper.GlobOPs(Utils::make_string("", "SPAM has been deleted to MASK: %s by nick %s.", x[2].c_str(), user->nick().c_str()));
			} else if (boost::iequals(x[1], "LIST")) {
				vector<vector<string> > result;
				string sql = "SELECT MASK, WHO, TARGET, MOTIVO FROM SPAM ORDER BY WHO;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0)
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "There is no SPAM.") + config->EOFMessage);
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "\002%s\002 by %s. Flags: %s Reason: %s", row.at(0).c_str(), row.at(1).c_str(), row.at(2).c_str(), row.at(3).c_str()) + config->EOFMessage);
				}
				return;
			}
			return;
		}
	} else if (cmd == "OPER") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			if (boost::iequals(x[1], "ADD")) {
				Oper oper;
				if (x.size() < 3) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				} if (NickServ::IsRegistered(x[2]) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", x[2].c_str()) + config->EOFMessage);
					return;
				} else if (OperServ::IsOper(x[2]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The iRCop already exists.") + config->EOFMessage);
					return;
				}
				std::string sql = "INSERT INTO OPERS VALUES ('" + x[2] + "', '" + user->nick() + "', " + std::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record can not be inserted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				User* target = Mainframe::instance()->getUserByName(x[2]);
				if (target) {
					if (target->getMode('o') == false && target->server() == config->Getvalue("serverName")) {
						target->session()->send(":" + config->Getvalue("serverName") + " MODE " + target->nick() + " +o" + config->EOFMessage);
						target->setMode('o', true);
						Servidor::sendall("UMODE " + target->nick() + " +o");
						miRCOps.insert(target);
					} else if (target->getMode('o') == false && target->server() != config->Getvalue("serverName")) {
						target->setMode('o', true);
						Servidor::sendall("UMODE " + target->nick() + " +o");
						miRCOps.insert(target);
					}
				}
				oper.GlobOPs(Utils::make_string("", "OPER %s inserted by nick: %s.", x[2].c_str(), user->nick().c_str()));
			} else if (boost::iequals(x[1], "DEL")) {
				Oper oper;
				if (x.size() < 3) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}
				if (OperServ::IsOper(x[2]) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "There is not OPER with such nick.") + config->EOFMessage);
					return;
				}
				std::string sql = "DELETE FROM OPERS WHERE NICK='" + x[2] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				User* target = Mainframe::instance()->getUserByName(x[2]);
				if (target) {
					if (target->getMode('o') == true && target->server() == config->Getvalue("serverName")) {
						target->session()->send(":" + config->Getvalue("serverName") + " MODE " + target->nick() + " -o" + config->EOFMessage);
						target->setMode('o', false);
						Servidor::sendall("UMODE " + target->nick() + " -o");
						miRCOps.erase(target);
					} else if (target->getMode('o') == true && target->server() != config->Getvalue("serverName")) {
						target->setMode('o', false);
						Servidor::sendall("UMODE " + target->nick() + " -o");
						miRCOps.erase(target);
					}
				}
				oper.GlobOPs(Utils::make_string("", "OPER %s deleted by nick: %s.", x[2].c_str(), user->nick().c_str()));
			} else if (boost::iequals(x[1], "LIST")) {
				vector<vector<string> > result;
				string sql = "SELECT NICK, OPERBY, TIEMPO FROM OPERS ORDER BY NICK;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "There is no OPERs.") + config->EOFMessage);
					return;
				}
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					time_t time = stoi(row.at(2));
					std::string cuando = Utils::Time(time);
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "\002%s\002 opered by %s since: %s", row.at(0).c_str(), row.at(1).c_str(), cuando.c_str()) + config->EOFMessage);
				}
			}
		}
	} else if (cmd == "EXCEPTIONS") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			if (boost::iequals(x[1], "ADD")) {
				Oper oper;
				if (x.size() < 5) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}  else if (boost::iequals(x[3], "clon") == false && boost::iequals(x[3], "dnsbl") == false && boost::iequals(x[3], "channel") == false && boost::iequals(x[3], "geoip") == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Incorrect EXCEPTION ( only allowed: clon, dnsbl, channel, geoip )") + config->EOFMessage);
					return;
				} else if (OperServ::IsException(x[2], x[3]) == true) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The exception already exists.") + config->EOFMessage);
					return;
				} else if (x[4].empty() || x[4].find_first_not_of("0123456789") != string::npos) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Incorrect EXCEPTION ( the parameter must be a number )") + config->EOFMessage);
					return;
				}
				boost::to_lower(x[3]);
				std::string sql = "INSERT INTO EXCEPTIONS VALUES ('" + x[2] + "', '" + x[3] + "', " + x[4] + ", '" + user->nick() + "', " + std::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record can not be inserted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				oper.GlobOPs(Utils::make_string("", "EXCEPTION %s inserted by nick: %s.", x[2].c_str(), user->nick().c_str()));
			} else if (boost::iequals(x[1], "DEL")) {
				Oper oper;
				if (x.size() < 4) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}
				boost::to_lower(x[3]);
				if (OperServ::IsException(x[2], x[3]) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "There is not EXCEPTION with such IP.") + config->EOFMessage);
					return;
				}
				std::string sql = "DELETE FROM EXCEPTIONS WHERE IP='" + x[2] + "'  AND OPTION='" + x[3] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				oper.GlobOPs(Utils::make_string("", "EXCEPTION %s deleted by nick: %s.", x[2].c_str(), user->nick().c_str()));
			} else if (boost::iequals(x[1], "LIST")) {
				if (x.size() < 2) {
                                        user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
                                        return;
                                }
				vector<vector<string> > result;
				boost::replace_all(x[2], "*", "%");
				string sql = "SELECT IP, ADDED, DATE, OPTION, VALUE FROM EXCEPTIONS WHERE IP LIKE '" + x[2] + "'  ORDER BY IP;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "There is no EXCEPTIONS.") + config->EOFMessage);
					return;
				}
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					time_t time = stoi(row.at(2));
					std::string cuando = Utils::Time(time);
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "\002%s\002 added by %s option: %s value: %s since: %s", row.at(0).c_str(), row.at(1).c_str(), row.at(3).c_str(), row.at(4).c_str(), cuando.c_str()) + config->EOFMessage);
				}
			}
		}
	}
}

bool OperServ::IsGlined(string ip) {
	std::string sql = "SELECT IP from GLINE WHERE IP='" + ip + "';";
	std::string retorno = DB::SQLiteReturnString(sql);
	return (boost::iequals(retorno, ip));
}

std::string OperServ::ReasonGlined(const string &ip) {
	std::string sql = "SELECT MOTIVO from GLINE WHERE IP='" + ip + "';";
	return DB::SQLiteReturnString(sql);
}

bool OperServ::IsOper(string nick) {
	std::string sql = "SELECT NICK from OPERS WHERE NICK='" + nick + "';";
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
		if (vect[i] == mask)
			return true;
	}
	return false;
}

bool OperServ::IsSpam(string text) {
	if (config->Getvalue("antispam") == "false" || config->Getvalue("antispam") == "0")
		return false;
	std::string score = bayes->score(text.c_str()).str(false);
	double puntos = std::stod(score);
	return (puntos > 0.90);
}

int OperServ::IsException(std::string ip, std::string option) {
	boost::to_lower(option);
	std::string sql = "SELECT VALUE from EXCEPTIONS WHERE IP='" + ip + "'  AND OPTION='" + option + "';";
	return DB::SQLiteReturnInt(sql);
}

bool OperServ::CanGeoIP(std::string ip) {
	StrVec vect;
	int valor = IsException(ip, "geoip");
	if (valor > 0)
		return true;
	std::string allowed = config->Getvalue("GeoIP-ALLOWED");
	std::string country = Utils::GetGeoIP(ip);
	if (allowed.length() > 0) {
		boost::split(vect, allowed, boost::is_any_of(" ,"), boost::token_compress_on);
		for (unsigned int i = 0; i < vect.size(); i++)
			if (boost::iequals(country, vect[i]) == true)
				return true;
		return false;
	}
	std::string denied = config->Getvalue("GeoIP-DENIED");
	if (denied.length() > 0) {
		boost::split(vect, denied, boost::is_any_of(" ,"), boost::token_compress_on);
		for (unsigned int i = 0; i < vect.size(); i++)
			if (boost::iequals(country, vect[i]) == true)
				return false;
		return true;
	}
	return true;
}
