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

#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include "Config.h"
#include "pool.h"
#include "amqp.h"

#include <string>
#include <set>
#include <thread>
#include <iostream>
#include <mutex>
#include <vector>
#include <deque>

class Channel;
class SSLUser;
class WebUser;

class ListenWSS : public std::enable_shared_from_this<ListenWSS>
{
public:
  ListenWSS(std::string ip, int port)
    : io_context_pool_(std::thread::hardware_concurrency()),
	acceptor_(boost::asio::make_strand(io_context_pool_.get_io_context().get_executor()), boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port)),
	context_(boost::asio::ssl::context::sslv23)
  {
	context_.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::single_dh_use);
    context_.use_certificate_chain_file("server.pem");
    context_.use_private_key_file("server.key", boost::asio::ssl::context::pem);
    context_.use_tmp_dh_file("dh.pem");
	io_context_pool_.run();
  }

  void do_accept();
  void handle_accept(const std::shared_ptr<WebUser> new_session, const boost::system::error_code& error);
  void handle_handshake(const std::shared_ptr<WebUser> new_session, const boost::system::error_code& error);
  
  io_context_pool io_context_pool_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ssl::context context_;
};

class ListenSSL : public std::enable_shared_from_this<ListenSSL>
{
public:
  ListenSSL(std::string ip, int port)
    : io_context_pool_(std::thread::hardware_concurrency()),
	acceptor_(boost::asio::make_strand(io_context_pool_.get_io_context().get_executor()), boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port)),
	context_(boost::asio::ssl::context::sslv23)
  {
	context_.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::single_dh_use);
    context_.use_certificate_chain_file("server.pem");
    context_.use_private_key_file("server.key", boost::asio::ssl::context::pem);
    context_.use_tmp_dh_file("dh.pem");
	io_context_pool_.run();
  }

  void start_accept();
  void handle_accept(std::shared_ptr<SSLUser> new_session, const boost::system::error_code& error);
  void handle_handshake(const std::shared_ptr<SSLUser> new_session, const boost::system::error_code& error);
  
  io_context_pool io_context_pool_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ssl::context context_;
};

class Listen : public std::enable_shared_from_this<Listen>
{
public:
  Listen(std::string ip, int port)
    : io_context_pool_(std::thread::hardware_concurrency()),
	acceptor_(boost::asio::make_strand(io_context_pool_.get_io_context().get_executor()), boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port))
  {
	io_context_pool_.run();
  }

  void do_accept();
  
  io_context_pool io_context_pool_;
  boost::asio::ip::tcp::acceptor acceptor_;
};

class PublicSock
{
  public:
	static void Server(std::string ip, std::string port);
	static void SSListen(std::string ip, std::string port);
	static void WebListen(std::string ip, std::string port);
	static void ServerListen(std::string ip, std::string port);
};

#include <cstdio>
#include <cstdlib>
#include <cstring>

class User {
  public:
	User(const std::string server) : mNickName("ZeusiRCd"), mIdent("ZeusiRCd"), mHost("undefined"), mCloak("undefined"), mvHost("undefined"), mServer(server), is_local(false), mLang(config["language"].as<std::string>()) {};
	User() : mNickName("ZeusiRCd"), mIdent("ZeusiRCd"), mHost("undefined"), mCloak("undefined"), mvHost("undefined"), mServer(config["serverName"].as<std::string>()), is_local(true), mLang(config["language"].as<std::string>()) {};
	virtual ~User() {};
	static bool FindUser(std::string nick);
	static User *GetUser(std::string user);
	bool getMode(char mode);
	bool canchangenick();
	void setMode(char mode, bool option);
	std::string messageHeader();
	void Exit(bool close);
	void MakeQuit();
	void QUIT();
	void Cycle();
	virtual void Close() = 0;
	virtual void deliver(const std::string msg) = 0;
	virtual void prior(const std::string msg) = 0;
	void SendAsServer(const std::string message);
	static bool AddUser(User *user, std::string newnick);
	static bool ChangeNickName(std::string oldnick, std::string newnick);
	void SJOIN(Channel* channel);
	void SPART(Channel* channel);
	void NICK(const std::string nickname);
	void kick(std::string kicker, std::string victim, const std::string& reason, Channel* channel);
	void away(const std::string away, bool on);
	
	void Parse(std::string message);
	
	std::string mNickName;
	std::string mIdent;
	std::string mHost;
	std::string mCloak;
	std::string mvHost;
	std::string mServer;
	std::string mAway;
	time_t bLogin;
	time_t bPing;
	time_t bSendQ;
	int SendQ = 0;
	
	bool bAway = false; 
	bool mode_r = false;
	bool mode_z = false;
	bool mode_o = false;
	bool mode_w = false;

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
	bool is_local = true;

	std::string PassWord;
	std::string mLang;

	std::set<Channel*> channels;
	std::deque<std::string> queue;
};

class NewUser : public User
{
	public:
	NewUser (const std::string server) : User(server) { is_local = false; };
	void deliver(const std::string msg) override {};
	void prior(const std::string msg) override {};
	void Close() override {};
};

class Ban
{
  public:
	Ban (std::string &channel, std::string &mask, std::string &whois, time_t tim) : canal(channel), mascara(mask), who(whois), fecha(tim), deadline(boost::asio::system_executor()) {
		time_t expire = config["banexpire"].as<int>() * 60;
		deadline.expires_from_now(boost::posix_time::seconds(expire)); 
		deadline.async_wait(boost::bind(&Ban::ExpireBan, this, boost::asio::placeholders::error));
	};
	~Ban () { };
	std::string mask();
	std::string whois();
	time_t 		time();
	void ExpireBan(const boost::system::error_code &e);
	std::string canal;
	std::string mascara;
	std::string who;
	time_t fecha;
	boost::asio::deadline_timer deadline;
};

class pBan
{
  public:
	pBan (std::string &channel, std::string &mask, std::string &whois, time_t tim) : canal(channel), mascara(mask), who(whois), fecha(tim) {};
	~pBan () { };
	std::string mask();
	std::string whois();
	time_t	time();

	std::string canal;
	std::string mascara;
	std::string who;
	time_t fecha;
};

class Channel
{
  public:
    Channel (std::string chan) : name(chan), mTopic("") {};
	std::string name;
	std::string mTopic;
	
	bool mode_r = false;
	bool is_flood = false;
	int flood = 0;
	
	std::set <User *> users;
	std::set <User *> operators;
	std::set <User *> halfoperators;
	std::set <User *> voices;
	std::set <Ban *> bans;
	std::set <pBan *> pbans;
	
	bool getMode(char mode);
    void setMode(char mode, bool option);
    
	void join(User *user);
	void part(User *user);
	void quit(User *user);
	void broadcast(const std::string message);
	void broadcast_except_me(const std::string nickname, const std::string message);
	void broadcast_away(User *user, std::string away, bool on);
	static bool FindChannel(std::string nombre);
	static Channel *GetChannel(std::string chan);
	void send_userlist(User* user);
	void send_who_list(User* user);

	bool HasUser(User *user);
	void InsertUser(User *user);
	void RemoveUser(User *user);
	void GiveOperator(User *user);
	bool IsOperator(User *user);
	void RemoveOperator(User *user);
	void GiveHalfOperator(User *user);
	bool IsHalfOperator(User *user);
	void RemoveHalfOperator(User *user);
	void GiveVoice(User *user);
	bool IsVoice(User *user);
	void RemoveVoice(User *user);
	
	void UnBan(Ban *ban);
	void UnpBan(pBan *ban);
	void SBAN(std::string mask, std::string whois, std::string time);
	void SPBAN(std::string mask, std::string whois, std::string time);
	bool IsBan(std::string mask);
	void setBan(std::string mask, std::string whois);
	void setpBan(std::string mask, std::string whois);
	void resetflood();
	void increaseflood();
	bool isonflood();
	void ExpireFlood();
};

extern std::map<std::string, User *> Users;
extern std::map<std::string, Channel *> Channels;
