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
#include "sha256.h"
#include "Server.h"
#include "db.h"
#include "services.h"
#include "Utils.h"
#include "base64.h"
#include "asio.h"

#include <map>
#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <vector>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <fstream>
#include <regex>
#include <thread>

using std::map;
using std::string;
using std::vector;
using boost::property_tree::ptree;
using std::make_pair;
using boost::match_results;
using boost::property_tree::read_json;
using boost::property_tree::write_json;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

using namespace std;

extern ForceMap bForce;

class Command
{
  public:
	static std::string isreg(const vector<string> args)
	{
	if (args.size() < 1) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Error in nick or channel input.").c_str());
		std::ostringstream buf;
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	} else if (args[0][0] == '#') {
		if (Utils::checkchan(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The channel contains no-valid characters.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (ChanServ::IsRegistered(args[0]) == true) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The channel %s is already registered.", args[0].c_str()).c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
			} else {
				ptree pt;
				pt.put ("status", "OK");
				pt.put ("message", Utils::make_string("", "The channel %s is not registered.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			}
		} else {
			if (Utils::checknick(args[0]) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else if (NickServ::IsRegistered(args[0]) == true) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick %s is already registered.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else {
				ptree pt;
				pt.put ("status", "OK");
				pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			}
		}
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Data Error.").c_str());
		std::ostringstream buf;
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	}

	static std::string registro(const vector<string> args)
	{
		if (args.size() < 2) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "Error at data input.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (args[0][0] == '#') {
			if (Utils::checkchan(args[0]) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The channel contains no-valid characters.").c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else if (Utils::checknick(args[1]) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else if (ChanServ::IsRegistered(args[0]) == true) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The channel %s is already registered.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else if (Server::HUBExiste() == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The HUB doesnt exists, DBs are in read-only mode.").c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else if (NickServ::IsRegistered(args[1]) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[1].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else {
				std::string topic = Utils::make_string("", "The channel has been registered.");
				std::string sql = "INSERT INTO CANALES VALUES ('" + args[0] + "', '" + args[1] + "', '+r', '', '" + encode_base64((const unsigned char*) topic.c_str(), topic.length()) + "',  " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The channel %s cannot be registered. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "INSERT INTO CMODES (CANAL) VALUES ('" + args[0] + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The channel %s cannot be registered. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				Channel* chan = Channel::GetChannel(args[0]);
				if (chan) {
					if (chan->getMode('r') == false) {
						chan->setMode('r', true);
						chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name + " +r");
						Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name + " +r");
					}
				}
				ptree pt;
				pt.put ("status", "OK");
				pt.put ("message", Utils::make_string("", "The channel %s has been registered.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			}
		} else {
			if (Utils::checknick(args[0]) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else if (NickServ::IsRegistered(args[0]) == true) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick %s is already registered.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else if (Server::HUBExiste() == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The HUB doesnt exists, DBs are in read-only mode.").c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else if (DB::EscapeChar(args[1]) == true) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The password contains no valid characters (!:;').").c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else {
				std::string sql = "INSERT INTO NICKS VALUES ('" + args[0] + "', '" + sha256(args[1]) + "', '', '', '', " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The nick %s cannot be registered. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "INSERT INTO OPTIONS (NICKNAME, LANG) VALUES ('" + args[0] + "', '" + config["language"].as<std::string>() + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The nick %s cannot be registered. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				ptree pt;
				pt.put ("status", "OK");
				pt.put ("message", Utils::make_string("", "The nick %s has been registered.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				User *target = User::GetUser(args[0]);
				if (target) {
					if (target->getMode('r') == false) {
						target->deliver(":" + config["serverName"].as<std::string>() + " MODE " + target->mNickName + " +r");
						target->setMode('r', true);
						if (target->is_local)
							Server::Send("UMODE " + target->mNickName + " +r");
					}
				}
				return json;
			}
		}
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Data Error.").c_str());
		std::ostringstream buf;
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	}

	static std::string drop(const vector<string> args)
	{
		if (args.size() < 1) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "Error at data input.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (args[0][0] == '#') {
			if (Utils::checkchan(args[0]) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The channel contains no-valid characters.").c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else if (ChanServ::IsRegistered(args[0]) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The channel %s is not registered.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else {
				string sql = "DELETE FROM CANALES WHERE NOMBRE='" + args[0] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The channel %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM ACCESS WHERE CANAL='" + args[0] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The channel %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM AKICK WHERE CANAL='" + args[0] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The channel %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM CMODES WHERE CANAL='" + args[0] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The channel %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				Channel* chan = Channel::GetChannel(args[0]);
				if (chan->getMode('r') == true) {
					chan->setMode('r', false);
					chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name + " -r");
					Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name + " -r");
				}
				ptree pt;
				pt.put ("status", "OK");
				pt.put ("message", Utils::make_string("", "The channel %s has been deleted.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			}
		} else {
			if (Utils::checknick(args[0]) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else if (NickServ::IsRegistered(args[0]) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			} else {
				std::string sql = "DELETE FROM NICKS WHERE NICKNAME='" + args[0] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The nick %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + args[0] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The nick %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM CANALES WHERE OWNER='" + args[0] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The nick %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				sql = "DELETE FROM ACCESS WHERE USUARIO='" + args[0] + "';";
				if (DB::SQLiteNoReturn(sql) == false) {
					ptree pt;
					pt.put ("status", "ERROR");
					pt.put ("message", Utils::make_string("", "The nick %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
					std::ostringstream buf;
					write_json (buf, pt, false);
					std::string json = buf.str();
					return json;
				}
				if (config["database"]["cluster"].as<bool>() == false) {
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Server::Send(sql);
				}
				User *target = User::GetUser(args[0]);
				if (target) {
					if (target->getMode('r') == true) {
						target->deliver(":" + config["serverName"].as<std::string>() + " MODE " + target->mNickName + " -r");
						target->setMode('r', false);
						if (target->is_local)
							Server::Send("UMODE " + target->mNickName + " -r");
					}
				}
				ptree pt;
				pt.put ("status", "OK");
				pt.put ("message", Utils::make_string("", "The nick %s has been deleted.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			}
		}
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Data Error.").c_str());
		std::ostringstream buf;
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	}

	static std::string auth(const vector<string> args)
	{
		if (args.size() < 2) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "Error at data input.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (Utils::checknick(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (DB::EscapeChar(args[1]) == true) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The password contains no valid characters (!:;').").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (NickServ::IsRegistered(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[0].c_str()).c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (bForce[args[0]] >= 7) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "Too much identify attempts for this nick. Try in 1 hour.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (NickServ::Login(args[0], args[1]) == true) {
			std::string nick = args[0];
			bForce[nick] = 0;
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "Logued in !").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else {
			std::string nick = args[0];
			if (bForce.count(nick) > 0)
				bForce[nick] += 1;
			else
				bForce[nick] = 1;
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "Wrong login").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		}
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Data Error.").c_str());
		std::ostringstream buf;
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	}

	static std::string online(const vector<string> args)
	{
		if (Utils::checknick(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (User::FindUser(args[0])) {
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The nick %s is online.", args[0].c_str()).c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick %s is offline.", args[0].c_str()).c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		}
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Data Error.").c_str());
		std::ostringstream buf;
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	}

	static std::string pass(const vector<string> args)
	{
		if (Utils::checknick(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (DB::EscapeChar(args[1]) == true) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The password contains no valid characters (!:;').").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (NickServ::IsRegistered(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[0].c_str()).c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else {
			string sql = "UPDATE NICKS SET PASS='" + sha256(args[1]) + "' WHERE NICKNAME='" + args[0] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The password for nick %s cannot be changed. Contact with an iRCop.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			}
			if (config["database"]["cluster"].as<bool>() == false) {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The password for nick %s has been changed to: %s", args[0].c_str(), args[1].c_str()).c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		}
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Data Error.").c_str());
		std::ostringstream buf;
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	}

	static std::string email(const vector<string> args)
	{
		if (Utils::checknick(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (std::regex_match(args[1], std::regex("^[_a-z0-9-]+(.[_a-z0-9-]+)*@[a-z0-9-]+(.[a-z0-9-]+)*(.[a-z]{2,4})$")) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The email seems to be wrong.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (NickServ::IsRegistered(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[0].c_str()).c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else {
			string sql = "UPDATE NICKS SET EMAIL='" + args[1] + "' WHERE NICKNAME='" + args[0] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The e-mail for nick %s cannot be changed. Contact with an iRCop.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			}
			if (config["database"]["cluster"].as<bool>() == false) {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The e-mail for nick %s has been changed.", args[0].c_str()).c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		}
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Data Error.").c_str());
		std::ostringstream buf;
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	}

	static std::string logs(const vector<string> args)
	{
		std::ifstream fichero("ircd.log");
		std::string linea;
		std::string respuesta;
		do {
			getline(fichero, linea);
			if (!fichero.eof() && Utils::Match(args[0].c_str(), linea.c_str()) == true)
				respuesta.append(linea + "<br />");
		} while (!fichero.eof());
		return respuesta;
	}

	static std::string ungline(const vector<string> args)
	{
		if (OperServ::IsGlined(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "Does not exists GLINE for that IP.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else {
			string sql = "DELETE FROM GLINE WHERE IP='" + args[0] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The GLINE for IP %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
				std::ostringstream buf;
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			}
			if (config["database"]["cluster"].as<bool>() == false) {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Server::Send(sql);
			}
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The GLINE for IP %s has been deleted.", args[0].c_str()).c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		}
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Data Error.").c_str());
		std::ostringstream buf;
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	}

	static std::string users(const std::vector<std::string> args)
	{
		Channel *chan = Channel::GetChannel("#" + args[0]);
		if (!chan) return "0";
		else return std::to_string(chan->users.size());
	}
};

class httpd : public std::enable_shared_from_this<httpd>
{
  public:
	tcp::socket socket_;
	beast::flat_buffer buffer_{8192};
	http::request<http::dynamic_body> request_;
	http::response<http::dynamic_body> response_;
	net::steady_timer deadline_{
		socket_.get_executor(), std::chrono::seconds(60)};
	httpd(tcp::socket socket)
		: socket_(std::move(socket)) {}
	void
	start()
	{
		check_deadline();
		read_request();
	}

	void
	read_request()
	{
		auto self = shared_from_this();
		http::async_read(
			socket_,
			buffer_,
			request_,
			[self](beast::error_code ec,
				std::size_t bytes_transferred)
			{
				boost::ignore_unused(bytes_transferred);
				self->process_request();
			});
	}

	void
	process_request()
	{
		response_.version(request_.version());
		response_.keep_alive(false);
		response_.result(http::status::ok);
		response_.set(http::field::server, "ZeusiRCd");
		create_response();
		write_response();
	}

	std::string parse_request()
	{
		std::vector<std::string> args;
		std::string response = string(request_.target());
		Utils::split(response, args, "?=&");
		
		if (args.size() < 3)
		{
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "Data Error.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (args[1] != "data")
		{
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "Data Error.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		}
		std::string cmd = args[0];
		std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
		args.erase (args.begin(),args.begin()+2);
		if (cmd == "/isreg") {
			return Command::isreg(args);
		} else if (cmd == "/register") {
			return Command::registro(args);
		} else if (cmd == "/drop") {
			return Command::drop(args);
		} else if (cmd == "/auth") {
			return Command::auth(args);
		} else if (cmd == "/online") {
			return Command::online(args);
		} else if (cmd == "/pass") {
			return Command::pass(args);
		} else if (cmd == "/email") {
			return Command::email(args);
		} else if (cmd == "/log") {
			response_.set(http::field::content_type, "text/html");
			return Command::logs(args);
		} else if (cmd == "/users") {
			response_.set(http::field::content_type, "text/html");
			return Command::users(args);
		} else if (cmd == "/ungline") {
			return Command::ungline(args);
		} else {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "Data Error.").c_str());
			std::ostringstream buf;
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		}
	}

	void
	create_response()
	{
		response_.set(http::field::content_type, "application/json");
		beast::ostream(response_.body()) << parse_request();
	}

	void
	write_response()
	{
		auto self = shared_from_this();

		http::async_write(
			socket_,
			response_,
			[self](beast::error_code ec, std::size_t)
			{
				self->socket_.shutdown(tcp::socket::shutdown_send, ec);
				self->deadline_.cancel();
			});
	}

	void
	check_deadline()
	{
		auto self = shared_from_this();

		deadline_.async_wait(
			[self](beast::error_code ec)
			{
				if(!ec)
				{
					self->Close();
				}
			});
	}

	void
	Close()
	{
	  boost::system::error_code ignored_error;
	  deadline_.cancel();
	  if (socket_.lowest_layer().lowest_layer().is_open() == false) return;
	  socket_.lowest_layer().lowest_layer().cancel(ignored_error);
	  socket_.lowest_layer().lowest_layer().close(ignored_error);
	}

	static void
	http_server(boost::asio::ip::tcp::acceptor& acceptor, boost::asio::ip::tcp::socket& socket)
	{
	  acceptor.async_accept(socket,
		  [&](boost::beast::error_code ec)
		  {
			  if(!ec)
				  std::make_shared<httpd>(std::move(socket))->start();
			  http_server(acceptor, socket);
		  });
	}
};

class API : public Module
{
  public:
	std::thread *th;
	boost::asio::io_context ioc;
	boost::asio::ip::tcp::acceptor acceptor;
	bool exited = false;
	API() : Module("", 50, false), acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 8000)) {
		th = new std::thread(&API::init, this);
		th->detach();
	};
	~API() { close(); };
	void init () {
		boost::asio::ip::tcp::socket socket{ioc};
		httpd::http_server(acceptor, socket);
		ioc.run();
		return;
	}
	void close() {
		acceptor.close();
		ioc.stop();
		delete th;
	}
	virtual void command(User *user, std::string message) override {}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new API);
}
