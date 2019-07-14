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

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include <string>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/strand.hpp>

#include "defines.h"
#include "user.h"
#include "channel.h"
#include "mainframe.h"

extern boost::asio::io_context channel_user_context;
extern boost::asio::io_context io_queue;
extern std::mutex quit_mtx;

class Servidor;

class Servidores
{
	public:
		Servidor *server;
		std::string nombre;
		std::string ipaddress;
		time_t sPing;
		std::vector <std::string> connected;
		
		Servidores (Servidor *servidor, const std::string &name, const std::string &ip);
		std::string name();
		std::string ip();
		Servidor *link();
		void UpdatePing();
		static void uPing(const std::string &servidor);
		time_t GetPing();
};

class Servidor : public std::enable_shared_from_this<Servidor>
{
	private:
		boost::asio::ip::tcp::socket mSocket;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> mSSL;
		std::string nombre;
		std::string ipaddress;
		bool quit;
		std::mutex mtx;

	public:
		~Servidor() {};
		Servidor(const boost::asio::executor& ex, boost::asio::ssl::context &ctx)
		:   mSocket(ex), mSSL(ex, ctx), quit(false), ssl(false) {}
		boost::asio::ip::tcp::socket& socket();
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket_ssl();
		bool ssl;
		void Procesar();
		void close();
		void send(const std::string& message);
		static void sendall(const std::string& message);
		static void sendallbutone(Servidor *server, const std::string& message);
		static void Connect(std::string ipaddr, std::string port);
		void SendBurst (Servidor *server);
		static void addServer(Servidor *servidor, std::string name, std::string ip, const std::vector <std::string> &conexiones);
		static void updateServer(std::string name, std::vector <std::string> conexiones);
		void addLink(const std::string &hub, std::string link);
		static bool IsAServer (const std::string &ip);
		static bool IsConected (const std::string &ip);
		static bool Exists (std::string name);
		std::string name();
		std::string ip();
		static int count ();
		void Message(Servidor *server, std::string message);
		static void SQUIT(std::string nombre);
		bool isQuit();
		void setQuit();
		void setname(const std::string &name);
};

typedef std::set<Servidores*> 	ServerSet;
typedef std::map<std::string, Servidores*> 	ServerMap;

class Session : public std::enable_shared_from_this<Session>
{
    
public:
		Session(const boost::asio::executor& ex, boost::asio::ssl::context &ctx)
			:   ssl(false), websocket(false), deadline(channel_user_context), mSocket(ex), mSSL(ex, ctx), wss_(ex, ctx),
			mBuffer(2048), ws_ready(false), strand(boost::asio::make_strand(ex)), mUser(this, config->Getvalue("serverName")) {
		}
		~Session () {}
        
		void start();
		void sendAsServer(const std::string& message);
        void sendAsUser(const std::string& message);
		void handleWrite(const boost::system::error_code& error, std::size_t bytes);
		void on_accept(boost::system::error_code ec);
		void handleWS(const boost::system::error_code& error, std::size_t bytes);
        void send(const std::string message);
        void write();
        void Procesar(); 
		void close();
		void on_close(boost::system::error_code ec);
		void on_shutdown(boost::beast::error_code ec);
		void socket_timer(int seconds);
		void start_timer(int seconds);
		void stop_timer();
		void call_timer();
		boost::asio::ip::tcp::socket& socket();
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket_ssl();
		boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>& socket_wss();
        std::string ip() const;
        void check_deadline(const boost::system::error_code &e);
        bool ssl = false;
        bool websocket = false;
		boost::asio::deadline_timer deadline;
		
private:
		void read();
		void handleRead(const boost::system::error_code& error, std::size_t bytes);
		boost::asio::ip::tcp::socket mSocket;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> mSSL;
		boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> wss_;
        boost::asio::streambuf mBuffer;
        bool ws_ready = false;
        boost::asio::strand<boost::asio::executor> strand;
        User mUser;
		std::string Queue;
		bool finish = true;
};

