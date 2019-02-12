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
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				
				UserMap usermap = Mainframe::instance()->users();
				UserMap::iterator it = usermap.begin();
				for (; it != usermap.end(); ++it) {
					if (!it->second)
						continue;
					else if (it->second->host() == x[2] && it->second->server() == config->Getvalue("serverName"))
						it->second->session()->close();
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
				std::string sql = "DELETE FROM GLINE WHERE IP='" + x[2] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
		if (target->server() == config->Getvalue("serverName"))
			target->session()->close();
		else
			target->QUIT();
		Servidor::sendall("QUIT " + target->nick());
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
			std::string sql = "DELETE FROM NICKS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
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
			sql = "DELETE FROM CANALES WHERE OWNER='" + x[1] + "' COLLATE NOCASE;";
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
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s has been deleted.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == 1) {
			std::string sql = "DELETE FROM CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":CHaN!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
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
			user->session()->send(":CHaN!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s has been deleted.", x[1].c_str()) + config->EOFMessage);
			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			if (chan->getMode('r') == true) {
				chan->setMode('r', false);
				chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -r" + config->EOFMessage);
				Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -r");
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
			string sql = "UPDATE NICKS SET PASS='" + sha256(x[2]) + "' WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The password for nick %s cannot be changed. Contact with an iRCop.", x[1].c_str()) + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
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
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
				std::string sql = "DELETE FROM SPAM WHERE MASK='" + x[2] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				User* target = Mainframe::instance()->getUserByName(x[2]);
				if (target) {
					if (target->getMode('o') == false && target->server() == config->Getvalue("serverName")) {
						target->session()->send(":" + config->Getvalue("serverName") + " MODE " + target->nick() + " +o" + config->EOFMessage);
						target->setMode('o', true);
						Servidor::sendall("UMODE " + target->nick() + " +o");
					} else if (target->getMode('o') == false && target->server() != config->Getvalue("serverName")) {
						target->setMode('o', true);
						Servidor::sendall("UMODE " + target->nick() + " +o");
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
				std::string sql = "DELETE FROM OPERS WHERE NICK='" + x[2] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
