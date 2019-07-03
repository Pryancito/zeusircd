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

#include "api.h"
#include "parser.h"
#include "sha256.h"
#include "server.h"
#include "db.h"
#include "services.h"
#include "mainframe.h"
#include "utils.h"
#include "base64.h"

#include <iostream>                 
#include <map>                      
#include <string>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/system/error_code.hpp>
#include <fstream>
#include <regex>

using std::map;
using std::string;
using std::vector;
using boost::property_tree::ptree;
using std::make_pair;
using boost::lexical_cast;
using boost::bad_lexical_cast;
using boost::format;
using boost::regex_search;
using boost::match_default;
using boost::match_results;
using boost::regex;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

using namespace std;

extern ForceMap bForce;

void
httpd::start()
{
	read_request();
	check_deadline();
}

void
httpd::read_request()
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
			if(!ec)
				self->process_request();
		});
}

// Determine what needs to be done with the request message.
void
httpd::process_request()
{
	response_.version(request_.version());
	response_.keep_alive(false);

	switch(request_.method())
	{
	case http::verb::get:
		response_.result(http::status::ok);
		response_.set(http::field::server, "ZeusiRCd");
		create_response();
		break;

	default:
		// We return responses indicating an error if
		// we do not recognize the request method.
		response_.result(http::status::bad_request);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body())
			<< "Invalid request-method '"
			<< std::string(request_.method_string())
			<< "'";
		break;
	}

	write_response();
}

std::string httpd::parse_request()
{
	StrVec args;
	std::allocator<char> req;
	std::string response = string(request_.target());
	boost::split(args, response, boost::is_any_of("?=,"), boost::token_compress_on);
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
	boost::to_lower(cmd);
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
		return Command::logs(args);
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

// Construct a response message based on the program state.
void
httpd::create_response()
{
	response_.set(http::field::content_type, "application/json");
	beast::ostream(response_.body()) << parse_request();
}

// Asynchronously transmit the response message.
void
httpd::write_response()
{
	auto self = shared_from_this();

	response_.set(http::field::content_length, response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
		});
}

// Check whether we have spent enough time on this connection.
void
httpd::check_deadline()
{
	auto self = shared_from_this();

	deadline_.async_wait(
		[self](beast::error_code ec)
		{
			if(!ec)
			{
				// Close socket to cancel any outstanding operation.
				self->socket_.close(ec);
			}
		});
}
void
httpd::http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
  acceptor.async_accept(socket,
	  [&](beast::error_code ec)
	  {
		  if(!ec)
			  std::make_shared<httpd>(std::move(socket))->start();
		  http_server(acceptor, socket);
	  });
}

std::string Command::isreg(const vector<string> args)                                               
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
		if (Parser::checkchan(args[0]) == false) {
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
		if (Parser::checknick(args[0]) == false) {
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
 
std::string Command::registro(const vector<string> args)                                               
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
		if (Parser::checkchan(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The channel contains no-valid characters.").c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			return json;
		} else if (Parser::checknick(args[1]) == false) {
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
			std::string sql = "INSERT INTO CANALES VALUES ('" + args[0] + "', '" + args[1] + "', '+r', '', '" + Base64::Encode(Utils::make_string("", "The channel has been registered.")) + "',  " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The channel %s cannot be registered. Please contact with an iRCop.", args[0].c_str()).c_str());
				std::ostringstream buf; 
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			Channel* chan = Mainframe::instance()->getChannelByName(args[0]);
			User* target = Mainframe::instance()->getUserByName(args[1]);
			if (chan) {
				if (chan->getMode('r') == false) {
					chan->setMode('r', true);
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +r" + config->EOFMessage);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +r");
				}
				if (target) {
					if (chan->hasUser(target)) {
						if (!chan->isOperator(target)) {
							chan->giveOperator(target);
							chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
							Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + target->nick());
						}
					}
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
		if (Parser::checknick(args[0]) == false) {
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
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			sql = "INSERT INTO OPTIONS (NICKNAME, LANG) VALUES ('" + args[0] + "', '" + config->Getvalue("language") + "');";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick %s cannot be registered. Please contact with an iRCop.", args[0].c_str()).c_str());
				std::ostringstream buf; 
				write_json (buf, pt, false);
				std::string json = buf.str();
				return json;
			}
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The nick %s has been registered.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			User *user = Mainframe::instance()->getUserByName(args[0]);
			if (user) {
				if (user->getMode('r') == false) {
					if (user->server() == config->Getvalue("serverName"))
						user->session()->send(":" + config->Getvalue("serverName") + " MODE " + user->nick() + " +r" + config->EOFMessage);
					user->setMode('r', true);
					Servidor::sendall("UMODE " + user->nick() + " +r");
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
 
std::string Command::drop(const vector<string> args)                                              
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
		if (Parser::checkchan(args[0]) == false) {
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
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			Channel* chan = Mainframe::instance()->getChannelByName(args[0]);
			if (chan->getMode('r') == true) {
				chan->setMode('r', false);
				chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -r" + config->EOFMessage);
				Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -r");
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
		if (Parser::checknick(args[0]) == false) {
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
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
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
			if (config->Getvalue("cluster") == "false") {
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
			}
			User *target = Mainframe::instance()->getUserByName(args[0]);
			if (target) {
				if (target->getMode('r') == true) {
					if (target->server() == config->Getvalue("serverName"))
						target->session()->send(":" + config->Getvalue("serverName") + " MODE " + target->nick() + " -r" + config->EOFMessage);
					target->setMode('r', false);
					Servidor::sendall("UMODE " + target->nick() + " -r");
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

std::string Command::auth(const vector<string> args)                                               
{
	if (args.size() < 2) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Error at data input.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	} else if (Parser::checknick(args[0]) == false) {
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

std::string Command::online(const vector<string> args)                                               
{
	if (Parser::checknick(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		return json;
	} else if (Mainframe::instance()->getUserByName(args[0])) {
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

std::string Command::pass(const vector<string> args)
{
	if (Parser::checknick(args[0]) == false) {
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
		if (config->Getvalue("cluster") == "false") {
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
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

std::string Command::email(const vector<string> args)                                               
{
	if (Parser::checknick(args[0]) == false) {
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
		if (config->Getvalue("cluster") == "false") {
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
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

std::string Command::logs(const vector<string> args)                                               
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

std::string Command::ungline(const vector<string> args)                                               
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
		if (config->Getvalue("cluster") == "false") {
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
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
