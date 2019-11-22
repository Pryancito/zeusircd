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

#pragma once

#include <string>
#include <iostream>
#include <mutex>
#include <set>

#include <boost/asio.hpp>
#include <boost/bind.hpp> 
#include <boost/asio/ssl.hpp>

#include "Utils.h"
#include "oper.h"
#include "Config.h"
#include "db.h"

class Server : public std::enable_shared_from_this<Server> {
	public:
		std::string name;
		std::string ip;
		std::string port;
		time_t bPing;
		boost::asio::deadline_timer deadline;
		
		Server(std::string ip, std::string puerto) : ip(ip), port(puerto), deadline(boost::asio::system_executor()) {
			deadline.expires_from_now(boost::posix_time::seconds(30)); 
			deadline.async_wait(boost::bind(&Server::CheckDead, this, boost::asio::placeholders::error));
		};
		~Server() { deadline.cancel(); }
		static bool CanConnect(const std::string ip);
		static bool CheckClone(const std::string &ip);
		static bool CheckThrottle(const std::string &ip);
		static void ThrottleUP(const std::string &ip);
		static bool CheckDNSBL(const std::string &ip);
		static bool HUBExiste();
		static void sendBurst(Server *server);
		void sendinitialBurst();
		static void Send(std::string message);
		void send(std::string message);
		void Close();
		void Procesar();
		static bool IsConected (const std::string &ip);
		static bool IsAServer (const std::string &ip);
		static bool Exists (const std::string name);
		static void SQUIT(std::string ip);
		static void Connect(std::string ipaddr, std::string port);
		static void ConnectCloud();
		void Ping();
		std::string remoteip();
		static size_t count();
		void handleWrite(const boost::system::error_code& error, std::size_t bytes);
	    void Parse(std::string message);
	    void CheckDead(const boost::system::error_code &e);
};
