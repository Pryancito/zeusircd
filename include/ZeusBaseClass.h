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
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/array.hpp>

#include "Config.h"
#include "pool.h" 
#include "Server.h"
#include "amqp.h"

#include <string>
#include <set>
#include <thread> 
#include <iostream>
#include <mutex>
#include <vector>

class Channel;

class PublicSock 
{
	public:
		static void Listen(std::string ip, std::string port, bool ipv6);
		static void SSListen(std::string ip, std::string port);
		static void WebListen(std::string ip, std::string port);
		static void API();
		static void ServerListen(std::string ip, std::string port, bool ssl);
};

class User {
	public: 
		User(const std::string server) : mNickName("ZeusiRCd"), mIdent("ZeusiRCd"), mHost("undefined"), mCloak("undefined"), mvHost("undefined"), mServer(server) {};
		User() : mNickName("ZeusiRCd"), mIdent("ZeusiRCd"), mHost("undefined"), mCloak("undefined"), mvHost("undefined"), mServer(config["serverName"].as<std::string>()) {};
		~User() {}; 
		static bool FindUser(std::string nick);
		bool getMode(char mode);
		void setMode(char mode, bool option);
		void log(const std::string &message);
				
		std::string messageHeader();
		std::string mNickName;
		std::string mIdent;
		std::string mHost;
		std::string mCloak;
		std::string mvHost;
		std::string mServer;
		std::string mAway;
		
		time_t bLogin;
		
		bool bAway = false; 
		bool mode_r = false;
		bool mode_z = false;
		bool mode_o = false;
		bool mode_w = false;

		std::set<Channel*> mChannels;
};

class LocalUser : public User {
	public:
		LocalUser() : User(config["serverName"].as<std::string>())
		, bSentPass(false)
		, bSentUser(false)
		, bSentNick(false) 
		, bSentMotd(false)
		, bSentQuit(false)
		, quit(false)
		, away_notify(false)
		, userhost_in_names(false)
		, extended_join(false)
		, negotiating(false)
		, mLang(config["language"].as<std::string>())
		{ };
		
		virtual ~LocalUser() { }; 
		void Parse(std::string message);
		void CheckPing();
		static bool checkstring(const std::string &str);
		static bool checknick(const std::string &nick);
		static bool checkchan(const std::string &chan);
		bool canchangenick();
		bool addUser(LocalUser* user, std::string nick);
		bool changeNickname(std::string old, std::string recent);
		void removeUser(std::string nick);
		void cmdNick(const std::string& newnick);
		void cmdPart(Channel* channel);
		void cmdJoin(Channel* channel);
		void cmdAway(const std::string &away, bool on);
		void cmdKick(std::string kicker, std::string victim, const std::string& reason, Channel* channel);
		void SKICK(Channel* channel);
		void Cycle();
		int Channs();
		void SendAsServer(const std::string message);
		void sendCAP(const std::string &cmd);
		void Request(std::string request);
		void recvEND();
		std::string sts();
		virtual void Close() = 0;
		virtual void Send(std::string message) = 0;
		void Exit(bool close);
		void MakeQuit();

		void do_cmd_nick(std::string message);
		void do_cmd_user(std::string message);
		void do_cmd_pass(std::string message);
		void do_cmd_quit();
		void do_cmd_release(std::string message);
		void do_cmd_join(std::string message);
		void do_cmd_part(std::string message);
		void do_cmd_topic(std::string message);
		void do_cmd_list(std::string message);
		void do_cmd_msg(std::string message, bool privmsg);
		void do_cmd_kick(std::string message);
		void do_cmd_names(std::string message);
		void do_cmd_who(std::string message);
		void do_cmd_away(std::string message);
		void do_cmd_oper(std::string message);
		void do_cmd_ping(std::string message);
		void do_cmd_pong();
		void do_cmd_userhost();
		void do_cmd_cap(std::string message);
		void do_cmd_webirc(std::string message);
		void do_cmd_stats();
		void do_cmd_opers();
		void do_cmd_uptime();
		void do_cmd_version();
		void do_cmd_rehash();
		void do_cmd_mode(std::string message);
		void do_cmd_whois(std::string message);
		void do_cmd_servers();
		void do_cmd_connect(std::string message);
		void do_cmd_squit(std::string message);
		void do_cmd_reload();
		void do_cmd_nickserv(std::string message, bool abreviated);
		void do_cmd_chanserv(std::string message, bool abreviated);
		void do_cmd_hostserv(std::string message, bool abreviated);
		void do_cmd_operserv(std::string message, bool abreviated);
		
		time_t bPing;
		time_t bSendQ;
		int SendQ = 0;
		
		bool bSentPass = false;
		bool bSentUser = false;
		bool bSentNick = false; 
		bool bSentMotd = false;
		bool bSentQuit = false;
		bool quit = false;
		bool away_notify = false;
		bool userhost_in_names = false;
		bool extended_join = false;
		bool negotiating = false;
		
		std::string PassWord;
		std::string mLang;
		std::mutex mtx;
};

class PlainUser : public LocalUser, public std::enable_shared_from_this<PlainUser> {
	public:
		PlainUser(const boost::asio::executor& ex) : LocalUser(), Socket(ex), mBuffer(2048), deadline(boost::asio::system_executor()) {};
		 ~PlainUser() { deadline.cancel(); };

		void Send(std::string message) override;
		void Close() override;
		void start();
		std::string ip();
		void read();
		void write();
		void handleWrite(const boost::system::error_code& error, std::size_t bytes);
		void handleRead(const boost::system::error_code& error, std::size_t bytes);
		void check_ping(const boost::system::error_code &e);
		
		boost::asio::ip::tcp::socket Socket;
		boost::asio::streambuf mBuffer;
		std::string Queue;
		bool finish = true; 
		boost::asio::deadline_timer deadline;
};

class LocalSSLUser : public LocalUser, public std::enable_shared_from_this<LocalSSLUser> {
	public:
		LocalSSLUser(const boost::asio::executor& ex, boost::asio::ssl::context &ctx) : LocalUser(), Socket(ex, ctx), mBuffer(2048), deadline(boost::asio::system_executor()) {}; 
		~LocalSSLUser() { deadline.cancel(); };
		
		void Send(std::string message) override;
		void Close() override;
		void start();
		std::string ip();
		void read();
		void write();
		void handleWrite(const boost::system::error_code& error, std::size_t bytes);
		void handleRead(const boost::system::error_code& error, std::size_t bytes);
		void check_ping(const boost::system::error_code &e);
		
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> Socket; 
		boost::asio::streambuf mBuffer;
		std::string Queue;
		bool finish = true; 
		boost::asio::deadline_timer deadline;
};

class LocalWebUser : public LocalUser, public std::enable_shared_from_this<LocalWebUser> {
	public:
		LocalWebUser(const boost::asio::executor& ex, boost::asio::ssl::context &ctx) : LocalUser(), Socket(ex, ctx), mBuffer(2048), deadline(boost::asio::system_executor()) {}; 
		~LocalWebUser() { deadline.cancel(); };
		
		void Send(std::string message) override;
		void Close() override;
		void start();
		std::string ip();
		void read();
		void on_write(boost::beast::error_code ec, std::size_t);
		void on_send(std::string const ss);
		void handleRead(boost::beast::error_code error, std::size_t bytes);
		void check_ping(const boost::system::error_code &e);
		void on_accept(boost::beast::error_code ec);
		
		boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> Socket;
		boost::beast::flat_buffer mBuffer;
		bool handshake = false;
		boost::asio::deadline_timer deadline;
		std::vector<std::string> queue;
};

class RemoteUser : public User {
	public:
		RemoteUser(const std::string server) : User(server) {};
		~RemoteUser() { };
		void QUIT();
		void SJOIN(Channel* channel);
		void SPART(Channel* channel);
		void NICK(const std::string &nickname);
		void SKICK(std::string kicker, std::string victim, const std::string reason, Channel* channel);
		void Cycle();
		void MakeQuit();
};

class ClientServer {
	public:
		ClientServer(size_t num_threads, boost::asio::io_context& io_context, const std::string s_ip, int s_port)
		: io_context_pool_(num_threads), mAcceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(s_ip), s_port)) { 
			mAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			mAcceptor.listen();
		}  
		void plain();
		void ssl();
		void wss();
		void run();
		void stop();
		void server(std::string ip, std::string port, bool ssl);
		void handleAccept(const std::shared_ptr<PlainUser> newclient, const boost::system::error_code& error);
		void handleSSLAccept(const std::shared_ptr<LocalSSLUser> newclient, const boost::system::error_code& error);
		void handle_handshake_ssl(const std::shared_ptr<LocalSSLUser> newclient, const boost::system::error_code& error);
		void handleWebAccept(const std::shared_ptr<LocalWebUser> newclient, const boost::system::error_code& error);
		void handleServerAccept(const std::shared_ptr<Server> newserver, const boost::system::error_code& error);
		void handle_handshake_server_ssl(const std::shared_ptr<Server> newserver, const boost::system::error_code& error);
		void handle_handshake_web(const std::shared_ptr<LocalWebUser> newclient, const boost::system::error_code& error);
		void check_deadline(const std::shared_ptr<PlainUser> newclient, const boost::system::error_code &e);
		void check_deadline_ssl(const std::shared_ptr<LocalSSLUser> newclient, const boost::system::error_code &e);
		void check_deadline_web(const std::shared_ptr<LocalWebUser> newclient, const boost::system::error_code &e);
		void on_accept(const std::shared_ptr<LocalWebUser> newclient, boost::system::error_code ec);
		io_context_pool io_context_pool_;
		boost::asio::ip::tcp::acceptor mAcceptor;
};
