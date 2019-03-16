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
#include "parser.h"

using namespace std;

void HostServ::Message(User *user, string message) {
	StrVec  x;
	boost::split(x, message, boost::is_any_of(" \t"), boost::token_compress_on);
	std::string cmd = x[0];
	boost::to_upper(cmd);
	
	if (cmd == "HELP") {
		user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :[ /hostserv register|drop|transfer|request|accept|off|list ]" + config->EOFMessage);
		return;
	} else if (cmd == "REGISTER") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			string owner;
			if (x.size() == 2)
				owner = user->nick();
			else
				owner = x[2];
			if (Parser::checknick(owner) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick contains no-valid characters.") + config->EOFMessage);
				return;
			} else if (NickServ::IsRegistered(owner) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", owner.c_str()) + config->EOFMessage);
				return;
			} else if (HostServ::CheckPath(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The path %s is not valid.", x[1].c_str()) + config->EOFMessage);
				return;
			} else if (HostServ::Owns(user, x[1]) == false && x[1].find("/") != std::string::npos) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not own the path %s", x[1].c_str()) + config->EOFMessage);
				return;
			} else if (HostServ::HowManyPaths(owner) >= 40) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s can not register more paths.", owner.c_str()) + config->EOFMessage);
				return;
			} else {
				string sql = "SELECT PATH from PATHS WHERE PATH='" + x[1] + "' COLLATE NOCASE;";
				if (boost::iequals(DB::SQLiteReturnString(sql), x[1]) == true) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The path %s is already registered.", x[1].c_str()) + config->EOFMessage);
					return;
				}
				sql = "SELECT VHOST from NICKS WHERE VHOST='" + x[1] + "' COLLATE NOCASE;";
				if (boost::iequals(DB::SQLiteReturnString(sql), x[1]) == true) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The path %s is already registered.", x[1].c_str()) + config->EOFMessage);
					return;
				}
				sql = "INSERT INTO PATHS VALUES ('" + owner + "', '" + x[1] + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Path %s can not be registered.", x[1].c_str()) + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Path %s has been registered by %s.", x[1].c_str(), owner.c_str()) + config->EOFMessage);
				return;
			}
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			if (HostServ::CheckPath(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The path %s is not valid.", x[1].c_str()) + config->EOFMessage);
				return;
			} else if (HostServ::Owns(user, x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not own the path %s", x[1].c_str()) + config->EOFMessage);
				return;
			} else {
				if (HostServ::DeletePath(x[1]) == true)
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Path %s has been deleted.", x[1].c_str()) + config->EOFMessage);
				else
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Path %s can not be deleted.", x[1].c_str()) + config->EOFMessage);
				return;
			}
		}
	} else if (cmd == "TRANSFER") {
		if (x.size() < 3) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else {
			string owner = x[2];
			if (Parser::checknick(owner) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick contains no-valid characters.") + config->EOFMessage);
				return;
			} else if (NickServ::IsRegistered(owner) == 0) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", owner.c_str()) + config->EOFMessage);
				return;
			} else if (user->getMode('r') == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
				return;
			} else if (HostServ::CheckPath(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The path %s is not valid.", x[1].c_str()) + config->EOFMessage);
				return;
			} else if (HostServ::Owns(user, x[1]) == false && x[1].find("/") != std::string::npos) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not own the path %s", x[1].c_str()) + config->EOFMessage);
				return;
			} else if (HostServ::HowManyPaths(owner) >= 40) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s can not register more paths.", owner.c_str()) + config->EOFMessage);
				return;
			} else {
				string sql = "UPDATE PATHS SET OWNER='" + owner + "' WHERE PATH='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The owner of path %s can not be changed.", x[1].c_str()) + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The owner of path %s has changed to: %s.", x[1].c_str(), owner.c_str()) + config->EOFMessage);
				return;
			}
		}
	} else if (cmd == "REQUEST") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else {
			if (HostServ::CheckPath(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The path %s is not valid.", x[1].c_str()) + config->EOFMessage);
				return;
			} else if (HostServ::GotRequest(user->nick()) == true && !boost::iequals(x[1], "OFF")) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You already have a vHost request.") + config->EOFMessage);
				return;
			} else if (HostServ::IsRegistered(x[1]) == false && !boost::iequals(x[1], "OFF")) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The path %s is not valid.", x[1].c_str()) + config->EOFMessage);
				return;
			} else if (HostServ::PathIsInvalid(x[1]) == true && !boost::iequals(x[1], "OFF")) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The path %s is not valid.", x[1].c_str()) + config->EOFMessage);
				return;
			} else if (user->getMode('r') == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
				return;
			} else if (boost::iequals(x[1], "OFF")) {
				if (HostServ::GotRequest(user->nick()) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have a vHost request.") + config->EOFMessage);
					return;
				}
				string sql = "DELETE FROM REQUEST WHERE OWNER='" + user->nick() + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your request can not be deleted.") + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your request has been deleted.") + config->EOFMessage);
				return;
			} else {
				string sql = "SELECT PATH from PATHS WHERE PATH='" + x[1] + "' COLLATE NOCASE;";
				if (boost::iequals(DB::SQLiteReturnString(sql), x[1]) == true) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The path %s is already registered.", x[1].c_str()) + config->EOFMessage);
					return;
				}
				sql = "INSERT INTO REQUEST VALUES ('" + user->nick() + "', '" + x[1] + "', " + std::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your request can not be registered.") + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your request has been registered successfully.") + config->EOFMessage);
				return;
			}
		}
	} else if (cmd == "ACCEPT") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else {
			if (Parser::checknick(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick contains no-valid characters.") + config->EOFMessage);
				return;
			} else if (NickServ::IsRegistered(x[1]) == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", x[1].c_str()) + config->EOFMessage);
				return;
			} else if (user->getMode('r') == false) {
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
				return;
			} else if (HostServ::GotRequest(x[1]) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s does not have a vHost request.", x[1].c_str()) + config->EOFMessage);
				return;
			} else {
				string sql = "SELECT PATH from REQUEST WHERE OWNER='" + x[1] + "' COLLATE NOCASE;";
				string path = DB::SQLiteReturnString(sql);
				sql = "DELETE FROM REQUEST WHERE OWNER='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your request can not be finished.") + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				sql = "UPDATE NICKS SET VHOST='" + path + "' WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your request can not be finished.") + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your request has finished successfully.") + config->EOFMessage);
				User* target = Mainframe::instance()->getUserByName(x[1]);
				if (target) {
					target->session()->sendAsServer("396 " + target->nick() + " " + target->cloak() + " :is now your hidden host" + config->EOFMessage);
					target->Cycle();
				}
				return;
			}
		}
	} else if (cmd == "OFF") {
		if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else if (NickServ::GetvHost(user->nick()) == "" && x.size() != 2) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your nick does not have a vHost set.") + config->EOFMessage);
			return;
		} else {
			string sql;
			if (user->getMode('o') == true && x.size() == 2) {
				if (NickServ::IsRegistered(x[1]) == true)
					sql = "UPDATE NICKS SET VHOST='' WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
				else {
					user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s is not registered.", x[1].c_str()) + config->EOFMessage);
					return;
				}
			} else
				sql = "UPDATE NICKS SET VHOST='' WHERE NICKNAME='" + user->nick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your deletion of vHost cannot be ended.") + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Your deletion of vHost finished successfully.") + config->EOFMessage);
			if (user->getMode('o') == true && x.size() == 2) {
				User* target = Mainframe::instance()->getUserByName(x[1]);
				if (target) {
					target->session()->sendAsServer("396 " + target->nick() + " " + target->cloak() + " :is now your hidden host" + config->EOFMessage);
					target->Cycle();
				}
			} else {
				user->session()->sendAsServer("396 " + user->nick() + " " + user->cloak() + " :is now your hidden host" + config->EOFMessage);
				user->Cycle();
			}
			return;
		}
	} else if (cmd == "LIST") {
		if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identify first.") + config->EOFMessage);
			return;
		} else {
			if (x.size() == 1) {
				vector <string> subpaths;
				vector <string> owner;
				string sql = "SELECT PATH from PATHS WHERE OWNER='" + user->nick() + "' COLLATE NOCASE;";
				vector <string> paths = DB::SQLiteReturnVector(sql);
				for (unsigned int i = 0; i < paths.size(); i++) {
					vector <string> temp1;
					vector <string> temp2;
					sql = "SELECT OWNER from REQUEST WHERE PATH LIKE '" + paths[i] + "%' COLLATE NOCASE;";
					temp1 = DB::SQLiteReturnVector(sql);
					sql = "SELECT PATH from REQUEST WHERE PATH LIKE '" + paths[i] + "%' COLLATE NOCASE;";
					temp2 = DB::SQLiteReturnVector(sql);
					for (unsigned int j = 0; j < temp1.size(); j++) {
						subpaths.push_back(temp2[j]);
						owner.push_back(temp1[j]);
					}
				}
				string temp1;
				for (unsigned int i = 0; i < subpaths.size(); i++) {
					if (temp1.find(owner[i]) == std::string::npos) {
						user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :<" + owner[i] + "> PATH: " + subpaths[i] + config->EOFMessage);
						temp1.append(owner[i] + ' ');
					}
				}
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "End of LIST.") + config->EOFMessage);
				return;
			} else if (x.size() == 2) {
				string search = x[1];
				boost::algorithm::to_lower(search);
				vector<vector<string> > result;
				string sql = "SELECT PATH, OWNER FROM PATHS ORDER BY OWNER;";
				result = DB::SQLiteReturnVectorVector(sql);
				for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
				{
					vector<string> row = *it;
					string path = row.at(0);
					boost::algorithm::to_lower(path);
					if (Utils::Match(search.c_str(), path.c_str()) == 1)
						user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :<" + row.at(1) + "> PATH: " + row.at(0) + config->EOFMessage);
				}
				user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "End of LIST.") + config->EOFMessage);
				return;
			}
		}
	}
}

int HostServ::HowManyPaths(const string &nickname) {
	string sql = "SELECT COUNT(*) from PATHS WHERE OWNER='" + nickname + "' COLLATE NOCASE;";
	return DB::SQLiteReturnInt(sql);
}

bool HostServ::CheckPath(string path) {
	StrVec subpaths;
	boost::split(subpaths,path,boost::is_any_of("/"));
	if (subpaths.size() < 1 || subpaths.size() > 10)
		return false;
	for (unsigned int i = 0; i < subpaths.size(); i++) {
		if (subpaths[i].length() == 0 || subpaths[i].length() > 64)
			return false;
		else if (Parser::checknick(subpaths[i]) == false)
			return false;
	}
	return true;
}

bool HostServ::IsRegistered(string path) {
	StrVec subpaths;
	boost::split(subpaths,path,boost::is_any_of("/"));
	string pp = subpaths[0];
	for (unsigned int i = 1; i < subpaths.size(); i++) {
		string sql = "SELECT PATH from PATHS WHERE PATH='" + pp + "' COLLATE NOCASE;";
		string retorno = DB::SQLiteReturnString(sql);
		if (retorno.empty())
			return false;
		else if (subpaths.size() >= i)
			pp.append("/" + subpaths[i]);
	}
	return true;
}

bool HostServ::Owns(User *user, string path) {
	StrVec subpaths;
	boost::split(subpaths,path,boost::is_any_of("/"));
	string pp = subpaths[0];
	for (unsigned int i = 1; i <= subpaths.size(); i++) {
		string sql = "SELECT OWNER from PATHS WHERE PATH='" + pp + "' COLLATE NOCASE;";
		string retorno = DB::SQLiteReturnString(sql);
		if (boost::iequals(retorno, user->nick()))
			return true;
		else if (user->getMode('o') == true)
			return true;
		else if (subpaths.size() >= i)
			pp.append("/" + subpaths[i]);
	}
	return false;
}

bool HostServ::DeletePath(const string &path) {
	string sql = "SELECT PATH from PATHS WHERE PATH LIKE '" + path + "%' COLLATE NOCASE;";
	StrVec retorno = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < retorno.size(); i++) {
		string sql = "DELETE FROM PATHS WHERE PATH='" + retorno[i] + "' COLLATE NOCASE;";
		if (DB::SQLiteNoReturn(sql) == false) {
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Servidor::sendall(sql);
	}
	sql = "SELECT NICKNAME from NICKS WHERE VHOST LIKE '" + path + "%' COLLATE NOCASE;";
	retorno = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < retorno.size(); i++) {
		string sql = "UPDATE NICKS SET VHOST='' WHERE NICKNAME='" + retorno[i] + "' COLLATE NOCASE;";
		if (DB::SQLiteNoReturn(sql) == false) {
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Servidor::sendall(sql);
	}
	return true;
}

bool HostServ::GotRequest (string user) {
	string sql = "SELECT OWNER from REQUEST WHERE OWNER='" + user + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	return (boost::iequals(retorno, user));
}

bool HostServ::PathIsInvalid (string path) {
	std::string sql = "SELECT VHOST from NICKS WHERE VHOST='" + path + "' COLLATE NOCASE;";
	std::string retorno = DB::SQLiteReturnString(sql);
	if (boost::iequals(retorno, path))
		return true;
		
	return false;
}
