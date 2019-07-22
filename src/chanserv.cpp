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
#include "base64.h"

using namespace std;

void ChanServ::Message(User *user, string message) {
	StrVec x;
	boost::trim_right(message);
	boost::split(x, message, boost::is_any_of(" \t"), boost::token_compress_on);
	std::string cmd = x[0];
	boost::to_upper(cmd);
	
	if (cmd == "HELP") {
		user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :[ /chanserv register|drop|vop|hop|aop|sop|topic|key|akick|op|deop|halfop|dehalfop|voice|devoice|transfer ]" + config->EOFMessage);
		return;
	} else if (cmd == "REGISTER") {
		if (x.size() < 2) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == true) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is already registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (ChanServ::CanRegister(user, x[1]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You need to be into the channel and got @ to make %s.", "REGISTER") + config->EOFMessage);
			return;
		} else {
			string sql = "INSERT INTO CANALES VALUES ('" + x[1] + "', '" + user->nick() + "', '+r', '', '" + Base64::Encode(Utils::make_string("", "The channel has been registered.")) + "',  " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s cannot be registered. Please contact with an iRCop.", x[1].c_str()) + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			sql = "INSERT INTO CMODES (CANAL) VALUES ('" + x[1] + "');";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s cannot be registered. Please contact with an iRCop.", x[1].c_str()) + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string("", "The channel %s has been registered.", x[1].c_str()) + config->EOFMessage);
			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			if (chan) {
				if (chan->getMode('r') == false) {
					chan->setMode('r', true);
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +r" + config->EOFMessage);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +r");
				}
			}
			return;
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (ChanServ::CanRegister(user, x[1]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You need to be into the channel and got @ to make %s.", "DROP") + config->EOFMessage);
			return;
		} else if (ChanServ::IsFounder(user->nick(), x[1]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You are not the founder of the channel.") + config->EOFMessage);
			return;
		} else {
			string sql = "DELETE FROM CANALES WHERE NOMBRE='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s cannot be deleted. Please contact with an iRCop.", x[1].c_str()) + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			sql = "DELETE FROM ACCESS WHERE CANAL='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s cannot be deleted. Please contact with an iRCop.", x[1].c_str()) + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			sql = "DELETE FROM AKICK WHERE CANAL='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s cannot be deleted. Please contact with an iRCop.", x[1].c_str()) + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			sql = "DELETE FROM CMODES WHERE CANAL='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s cannot be deleted. Please contact with an iRCop.", x[1].c_str()) + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s has been deleted.", x[1].c_str()) + config->EOFMessage);
			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			if (chan) {
				if (chan->getMode('r') == true) {
					chan->setMode('r', false);
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -r" + config->EOFMessage);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -r");
				}
			}
		}
	} else if (cmd == "VOP" || cmd == "HOP" || cmd == "AOP" || cmd == "SOP") {
		if (x.size() < 3) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is not registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 4) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have enough access.") + config->EOFMessage);
			return;
		} else {
			if (boost::iequals(x[2], "ADD")) {
				if (x.size() < 4) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				} else if (NickServ::IsRegistered(x[3]) == false) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", x[3].c_str()) + config->EOFMessage);
					return;
				} else if (NickServ::GetOption("NOACCESS", x[3]) == true) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The option NOACCESS is enabled for this user.") + config->EOFMessage);
					return;
				}
				if (ChanServ::Access(x[3], x[1]) != 0) {
					string acceso;
					switch (ChanServ::Access(x[3], x[1])) {
						case 1: acceso = "VOP"; break;
						case 2: acceso = "HOP"; break;
						case 3: acceso = "AOP"; break;
						case 4: acceso = "SOP"; break;
						case 5: acceso = "FUNDADOR"; break;
						default: acceso = "NINGUNO"; break;
					}
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nickname has already access: %s", acceso.c_str()) + config->EOFMessage);
					return;
				} else {
					string sql = "INSERT INTO ACCESS VALUES ('" + x[1] + "', '" + cmd + "', '" + x[3] + "', '" + user->nick() + "');";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Cannot insert the record.") + config->EOFMessage);
						return;
					}
					if (config->Getvalue("cluster") == "false") {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Servidor::sendall(sql);
					}
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record has been inserted.") + config->EOFMessage);
					User *target = Mainframe::instance()->getUserByName(x[3]);
					if (target)
						ChanServ::CheckModes(target, x[1]);
				}
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "%s has been given to %s", cmd.c_str(), x[3].c_str()) + config->EOFMessage);
			} else if (boost::iequals(x[2], "DEL")) {
				if (x.size() < 4) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}
				if (ChanServ::Access(x[3], x[1]) == 0) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The user do not have access.") + config->EOFMessage);
					return;
				}
				string sql = "DELETE FROM ACCESS WHERE USUARIO='" + x[3] + "'  AND CANAL='" + x[1] + "'  AND ACCESO='" + cmd + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				User *target = Mainframe::instance()->getUserByName(x[3]);
				if (target)
					ChanServ::CheckModes(target, x[1]);
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "%s has been removed to %s", cmd.c_str(), x[3].c_str()) + config->EOFMessage);
			} else if (boost::iequals(x[2], "LIST")) {
				vector<vector<string> > result;
				string sql = "SELECT USUARIO, ADDED FROM ACCESS WHERE CANAL='" + x[1] + "' AND ACCESO='" + cmd + "' ORDER BY USUARIO;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "There is no access of %s", cmd.c_str()) + config->EOFMessage);
					return;
				}
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "\002%s\002 accessed by %s", row.at(0).c_str(), row.at(1).c_str()) + config->EOFMessage);
				}
			}
			return;
		}
	} else if (cmd == "TOPIC") {
		if (x.size() < 3) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is not registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 3) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have enough access.") + config->EOFMessage);
			return;
		} else {
			int pos = 7 + x[1].length();
			string topic = message.substr(pos);
			if (topic.length() > 250) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The topic is too long.") + config->EOFMessage);
				return;
			}
			string sql = "UPDATE CANALES SET TOPIC='" + Base64::Encode(topic) + "' WHERE NOMBRE='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The topic can not be changed.") + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			if (chan) {
				chan->cmdTopic(topic);
				chan->broadcast(":" + config->Getvalue("chanserv") + " 332 " + user->nick() + " " + chan->name() + " :" + topic + config->EOFMessage);
			}
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The topic has changed.") + config->EOFMessage);
			return;
		}
	} else if (cmd == "AKICK") {
		if (x.size() < 3) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is not registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 4) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have enough access.") + config->EOFMessage);
			return;
		} else {
			if (boost::iequals(x[2], "ADD")) {
				if (x.size() < 5) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}
				if (ChanServ::IsAKICK(x[3], x[1]) != 0) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The mask already got AKICK.") + config->EOFMessage);
					return;
				} else {
					int posicion = 4 + cmd.length() + x[1].length() + x[2].length() + x[3].length();
					string motivo = message.substr(posicion);
					if (DB::EscapeChar(motivo) == true || DB::EscapeChar(x[3]) == true) {
						user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The reason or the mask contains no-valid characters.") + config->EOFMessage);
						return;
					}
					string sql = "INSERT INTO AKICK VALUES ('" + x[1] + "', '" + x[3] + "', '" + motivo + "', '" + user->nick() + "');";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The AKICK can not be inserted.") + config->EOFMessage);
						return;
					}
					if (config->Getvalue("cluster") == "false") {
						sql = "DB " + DB::GenerateID() + " " + sql;
						DB::AlmacenaDB(sql);
						Servidor::sendall(sql);
					}
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record has been inserted.") + config->EOFMessage);
				}
			} else if (boost::iequals(x[2], "DEL")) {
				if (x.size() < 4) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}
				if (ChanServ::IsAKICK(x[3], x[1]) == 0) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The user do not have AKICK.") + config->EOFMessage);
					return;
				}
				string sql = "DELETE FROM AKICK WHERE MASCARA='" + x[3] + "'  AND CANAL='" + x[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The record cannot be deleted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The AKICK for %s has been deleted.", x[3].c_str()) + config->EOFMessage);
			} else if (boost::iequals(x[2], "LIST")) {
				vector<vector<string> > result;
				string sql = "SELECT MASCARA, ADDED, MOTIVO FROM AKICK WHERE CANAL='" + x[1] + "' ORDER BY MASCARA;";
				result = DB::SQLiteReturnVectorVector(sql);
				if (result.size() == 0) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "AKICK list of channel %s is empty.", x[1].c_str()) + config->EOFMessage);
					return;
				}
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "\002%s\002 updated by %s. Reason: %s", row.at(0).c_str(), row.at(1).c_str(), row.at(2).c_str()) + config->EOFMessage);
				}
			}
			return;
		}
	} else if (cmd == "OP" || cmd == "DEOP" || cmd == "HALFOP" || cmd == "DEHALFOP" || cmd == "VOICE" || cmd == "DEVOICE") {
		if (x.size() < 3) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is not registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 3) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have enough access.") + config->EOFMessage);
			return;
		} else if (!Mainframe::instance()->getUserByName(x[2])) {
			user->send(":" + config->Getvalue("serverName") + " 401 " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick does not exist.") + config->EOFMessage);
			return;
		} else {
			char modo;
			bool action;
			if (cmd == "OP") {
				modo = 'o';
				action = 1;
			} else if (cmd == "DEOP") {
				modo = 'o';
				action = 0;
			} else if (cmd == "HALFOP") {
				modo = 'h';
				action = 1;
			} else if (cmd == "DEHALFOP") {
				modo = 'h';
				action = 0;
			} else if (cmd == "VOICE") {
				modo = 'v';
				action = 1;
			} else if (cmd == "DEVOICE") {
				modo = 'v';
				action = 0;
			} else
				return;

			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			User *target = Mainframe::instance()->getUserByName(x[2]);
			
			if (!chan || !target)
				return;
			if (chan->isVoice(target)) {
				if (modo == 'h' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
					chan->delVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + target->nick() + config->EOFMessage);
					chan->giveHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +h " + target->nick());
				} else if (modo == 'o' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
					chan->delVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
					chan->giveOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + target->nick());
				} else if (modo == 'v' && action == 1) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick already got %s.", "VOICE") + config->EOFMessage);
				} else if (modo == 'v' && action == 0) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
					chan->delVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + target->nick());
				}
			} else if (chan->isHalfOperator(target)) {
				if (modo == 'v' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
					chan->delHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -h " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +v " + target->nick() + config->EOFMessage);
					chan->giveVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +v " + target->nick());
				} else if (modo == 'o' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
					chan->delHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -h " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
					chan->giveOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + target->nick());
				} else if (modo == 'h' && action == 1) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick already got %s.", "HALFOP") + config->EOFMessage);
				} else if (modo == 'h' && action == 0) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
					chan->delHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -h " + target->nick());
				}
			} else if (chan->isOperator(target)) {
				if (modo == 'v' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
					chan->delOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -o " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +v " + target->nick() + config->EOFMessage);
					chan->giveVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +v " + target->nick());
				} else if (modo == 'h' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
					chan->delOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -o " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + target->nick() + config->EOFMessage);
					chan->giveHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +h " + target->nick());
				} else if (modo == 'o' && action == 1) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick already got %s.", "OP") + config->EOFMessage);
				} else if (modo == 'o' && action == 0) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
					chan->delOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -o " + target->nick());
				}
			} else if (chan->hasUser(target)) {
				if (modo == 'v' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +v " + target->nick() + config->EOFMessage);
					chan->giveVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +v " + target->nick());
				} else if (modo == 'h' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + target->nick() + config->EOFMessage);
					chan->giveHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +h " + target->nick());
				} else if (modo == 'o' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
					chan->giveOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + target->nick());
				} else if (action == 0){
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick do not have any mode.") + config->EOFMessage);
				}
			}
			return;
		}
	} else if (cmd == "KEY") {
		if (x.size() < 3) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is not registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 5) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have enough access.") + config->EOFMessage);
			return;
		} else {
			string key = x[2];
			if (DB::EscapeChar(key) == true) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The key contains no-valid characters.") + config->EOFMessage);
				return;
			}
			if (key.length() > 32) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The key is too long.") + config->EOFMessage);
				return;
			}
			if (boost::iequals(key, "OFF")) {
				string sql = "UPDATE CANALES SET CLAVE='' WHERE NOMBRE='" + x[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The key can not be changed.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The key has been removed.", key.c_str()) + config->EOFMessage);
			} else {
				string sql = "UPDATE CANALES SET CLAVE='" + key + "' WHERE NOMBRE='" + x[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The key can not be changed.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The key has changed to: %s", key.c_str()) + config->EOFMessage);
			}
		}
	} else if (cmd == "MODE") {
		if (x.size() < 3) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is not registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 4) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have enough access.") + config->EOFMessage);
			return;
		} else {
			string mode = x[2].substr(1);
			boost::to_upper(mode);
			if (boost::iequals("LIST", x[2])) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The available modes are: flood, onlyreg, autovoice, moderated, onlysecure, nonickchange, onlyweb, country, onlyaccess") + config->EOFMessage);
				return;
			} else if (!boost::iequals("FLOOD", mode) && !boost::iequals("ONLYREG", mode) && !boost::iequals("AUTOVOICE", mode) &&
						!boost::iequals("MODERATED", mode) && !boost::iequals("ONLYSECURE", mode) && !boost::iequals("NONICKCHANGE", mode) &&
						!boost::iequals("ONLYWEB", mode) && !boost::iequals("COUNTRY", mode) && !boost::iequals("ONLYACCESS", mode)) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Unknown mode.") + config->EOFMessage);
				return;
			} if (x[2][0] == '+') {
				std::string sql;
				if (ChanServ::HasMode(x[1], mode) == true) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s already got the mode %s", x[1].c_str(), mode.c_str()) + config->EOFMessage);
					return;
				} if ((mode == "COUNTRY" || mode == "FLOOD") && x.size() != 4) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The flood mode got parameters.") + config->EOFMessage);
					return;
				} else if (mode == "FLOOD" && (!Utils::isnum(x[3]) || stoi(x[3]) < 1 || stoi(x[3]) > 999)) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The parameter of flood mode is incorrect.") + config->EOFMessage);
					return;
				} else if (mode == "FLOOD")
					sql = "UPDATE CMODES SET " + mode + "=" + x[3] + " WHERE CANAL='" + x[1] + "';";
				else if (mode == "COUNTRY")
					sql = "UPDATE CMODES SET " + mode + "='" + x[3] + "' WHERE CANAL='" + x[1] + "';";
				else
					sql = "UPDATE CMODES SET " + mode + "=1 WHERE CANAL='" + x[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The mode can not be setted.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The mode is setted.") + config->EOFMessage);
				return;
			} else if (x[2][0] == '-') {
				if (ChanServ::HasMode(x[1], mode) == false) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s doesnt got the mode %s", x[1].c_str(), mode.c_str()) + config->EOFMessage);
					return;
				}
				string sql;
				if (mode == "COUNTRY")
					sql = "UPDATE CMODES SET " + mode + "='' WHERE CANAL='" + x[1] + "';";
				else
					sql = "UPDATE CMODES SET " + mode + "=0 WHERE CANAL='" + x[1] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The mode cannot be removed.") + config->EOFMessage);
					return;
				}
				if (config->Getvalue("cluster") == "false") {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
				}
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The mode has been removed.") + config->EOFMessage);
				return;
			}
		}
	} else if (cmd == "TRANSFER") {
		if (x.size() < 3) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(x[2]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The target nick is not registered.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is not registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 5) {
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have enough access to change founder.") + config->EOFMessage);
			return;
		} else {
			string sql = "UPDATE CANALES SET OWNER='" + x[2] + "' WHERE NOMBRE='" + x[1] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The founder cannot be changed.") + config->EOFMessage);
				return;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			user->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The founder has changed to: %s", x[2].c_str()) + config->EOFMessage);
			return;
		}
	}
}

void ChanServ::DoRegister(User *user, Channel *chan) {
	string sql = "SELECT TOPIC FROM CANALES WHERE NOMBRE='" + chan->name() + "';";
	string topic = DB::SQLiteReturnString(sql);
	topic = Base64::Decode(topic);
	sql = "SELECT REGISTERED FROM CANALES WHERE NOMBRE='" + chan->name() + "';";
	int creado = DB::SQLiteReturnInt(sql);
	if (topic != "") {
		chan->cmdTopic(topic);
		user->sendAsServer("332 " + user->nick() + " " + chan->name() + " :" + topic + config->EOFMessage);
		user->sendAsServer("333 " + user->nick() + " " + chan->name() + " " + config->Getvalue("chanserv") + " " + std::to_string(creado) + config->EOFMessage);
	}
	if (chan->getMode('r') == false) {
		chan->setMode('r', true);
		user->send(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +r" + config->EOFMessage);
		Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +r");
	}
}

int ChanServ::HasMode(const string &canal, string mode) {
	boost::to_upper(mode);
	string sql;
	if (mode == "COUNTRY") {
		sql = "SELECT " + mode + " FROM CMODES WHERE CANAL='" + canal + "';";
		return (DB::SQLiteReturnString(sql) != "");
	} else {
		sql = "SELECT " + mode + " FROM CMODES WHERE CANAL='" + canal + "';";
		return (DB::SQLiteReturnInt(sql));
	}
}

void ChanServ::CheckModes(User *user, const string &channel) {
	if (NickServ::GetOption("NOOP", user->nick()) == true)
		return;
	Channel* chan = Mainframe::instance()->getChannelByName(channel);
	if (!chan)
		return;
	int access = ChanServ::Access(user->nick(), chan->name());
	if (HasMode(channel, "AUTOVOICE") && access < 1) access = 1;
	
	if (chan->isVoice(user) == true) {
		if (access < 1) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + user->nick() + config->EOFMessage);
			chan->delVoice(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + user->nick());
		} else if (access == 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + user->nick() + config->EOFMessage);
			chan->delVoice(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + user->nick());
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + user->nick() + config->EOFMessage);
			chan->giveHalfOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +h " + user->nick());
		} else if (access > 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + user->nick() + config->EOFMessage);
			chan->delVoice(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + user->nick());
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + user->nick() + config->EOFMessage);
			chan->giveOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + user->nick());
		}
	} else if (chan->isHalfOperator(user) == true) {
		if (access < 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -h " + user->nick() + config->EOFMessage);
			chan->delHalfOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -h " + user->nick());
		} else if (access > 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -h " + user->nick() + config->EOFMessage);
			chan->delHalfOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -h " + user->nick());
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + user->nick() + config->EOFMessage);
			chan->giveOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + user->nick());
		}
	} else if (chan->isOperator(user) == true) {
		if (access < 3) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -o " + user->nick() + config->EOFMessage);
			chan->delOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -o " + user->nick());
		}
	} else {
		if (access == 1) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +v " + user->nick() + config->EOFMessage);
			chan->giveVoice(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +v " + user->nick());
		} else if (access == 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + user->nick() + config->EOFMessage);
			chan->giveHalfOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +h " + user->nick());
		} else if (access > 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + user->nick() + config->EOFMessage);
			chan->giveOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + user->nick());
		}
	}
}

bool ChanServ::IsRegistered(string channel) {
	string sql = "SELECT NOMBRE from CANALES WHERE NOMBRE='" + channel + "';";
	string retorno = DB::SQLiteReturnString(sql);
	return (boost::iequals(retorno, channel));
}

bool ChanServ::IsFounder(string nickname, string channel) {
	string sql = "SELECT OWNER from CANALES WHERE NOMBRE='" + channel + "';";
	string retorno = DB::SQLiteReturnString(sql);
	return (boost::iequals(retorno, nickname));
}

int ChanServ::Access (string nickname, string channel) {
	string sql = "SELECT ACCESO from ACCESS WHERE USUARIO='" + nickname + "'  AND CANAL='" + channel + "';";
	string retorno = DB::SQLiteReturnString(sql);
	User* user = Mainframe::instance()->getUserByName(nickname);
	if (boost::iequals(retorno, "VOP"))
		return 1;
	else if (boost::iequals(retorno, "HOP"))
		return 2;
	else if (boost::iequals(retorno, "AOP"))
		return 3;
	else if (boost::iequals(retorno, "SOP"))
		return 4;
	else if (ChanServ::IsFounder(nickname, channel) == true)
		return 5;
	else if (user)
		if (user->getMode('o') == true)
			return 5;
	return 0;
}

bool ChanServ::IsAKICK(string mascara, const string &canal) {
	vector <string> akicks;
	boost::algorithm::to_lower(mascara);
	string sql = "SELECT MASCARA from AKICK WHERE CANAL='" + canal + "';";
	akicks = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < akicks.size(); i++) {
		boost::algorithm::to_lower(akicks[i]);
		if (Utils::Match(akicks[i].c_str(), mascara.c_str()) == 1)
			return true;
	}
	return false;
}

bool ChanServ::CheckKEY(const string &canal, string key) {
	string sql = "SELECT CLAVE from CANALES WHERE NOMBRE='" + canal + "';";
	string retorno = DB::SQLiteReturnString(sql);
	if (retorno.length() == 0 || key.length() == 0)
		return true;
	if (retorno == key)
		return true;
	else
		return false;
}

bool ChanServ::IsKEY(const string &canal) {
	string sql = "SELECT CLAVE from CANALES WHERE NOMBRE='" + canal + "';";
	string retorno = DB::SQLiteReturnString(sql);
	return (retorno.length() > 0);
}

int ChanServ::GetChans () {
	string sql = "SELECT COUNT(*) FROM CANALES;";
	return DB::SQLiteReturnInt(sql);
}

bool ChanServ::CanRegister(User *user, string channel) {
	string sql = "SELECT COUNT(*) FROM CANALES WHERE FOUNDER='" + user->nick() + "';";
	int channels = DB::SQLiteReturnInt(sql);
	if (channels >= stoi(config->Getvalue("maxchannels")))
		return false;

	Channel* chan = Mainframe::instance()->getChannelByName(channel);
	if (chan)
		return (chan->hasUser(user) && chan->isOperator(user));

	return false;
}

bool ChanServ::CanGeoIP(User *user, string channel) {
	std::string country = Utils::GetGeoIP(user->host());
	std::string sql = "SELECT COUNTRY FROM CMODES WHERE CANAL = '" + channel + "';";
	std::string dbcountry = DB::SQLiteReturnString(sql);
	if (dbcountry == "")
		return true;
	return (boost::iequals(dbcountry, country));
}
