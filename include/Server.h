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
#include <vector>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>

class Server {
	public:
		Server() {};
		~Server() {};
		static bool CanConnect(const std::string ip);
		static bool CheckClone(const std::string &ip);
		static bool CheckThrottle(const std::string &ip);
		static void ThrottleUP(const std::string &ip);
		static bool HUBExiste();
		static void sendall(const std::string message);
		
		std::string nombre;
		std::string ip;
        int port;
        std::vector <std::string> connected;
};

class LocalServer : public Server
{
	public:
		LocalServer() {};
		~LocalServer() {};
		time_t sPing;
		virtual void Send(const std::string message) = 0;
		virtual void Close() = 0;
};
/*
class PlainServer : public LocalServer, public CTCPServer
{
	public:
		PlainServer(CTCPServer server) : CTCPServer(server) {};
		~PlainServer() {};
		ASocket::Socket ConnectedClient;
		void Receive();
		void Send(const std::string message);
		void Close();
		void start();
};

class SSLServer : public LocalServer, public CTCPSSLServer
{
	public:
		SSLServer(CTCPSSLServer const &server) : CTCPSSLServer(server) { ssl = true; };
		~SSLServer() {};
		ASecureSocket::SSLSocket ConnectedClient;
		void Receive();
		void Send(const std::string message);
		void Close();
		void start();
};*/

class RemoteServer : public Server
{
	public:
		RemoteServer() {};
		~RemoteServer() {};
};
