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

class Server {
	public:
		std::string name;
		std::string port;
		std::string ip;
		bool ssl = false;
		boost::asio::ip::tcp::socket Socket;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLSocket;
		boost::asio::strand<boost::asio::executor> strand; 
		boost::asio::streambuf mBuffer;
		boost::asio::deadline_timer deadline;
		time_t bPing;

		Server(const boost::asio::executor& ex, boost::asio::ssl::context &ctx, std::string name, std::string ip, std::string port) : name(name), port(port), ip(ip)
				, Socket(ex), SSLSocket(ex, ctx), strand(ex), mBuffer(2048), deadline(ex) {};
		~Server() {};
		static bool CanConnect(const std::string ip);
		static bool CheckClone(const std::string &ip);
		static bool CheckThrottle(const std::string &ip);
		static void ThrottleUP(const std::string &ip);
		static bool CheckDNSBL(const std::string &ip);
		static bool HUBExiste();
		void sendBurst(Server *server);
		static void Send(std::string message);
		void send(const std::string& message);
		void Close();
		void Procesar();
		static bool IsConected (const std::string &ip);
		static bool IsAServer (const std::string &ip);
		void Parse(std::string message);
		void SQUIT(std::string ip);
		static void Connect(std::string ipaddr, std::string port);
		static void ConnectCloud();
		void Ping();
};
