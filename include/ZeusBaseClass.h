#pragma once
#include <boost/asio.hpp>
#include <boost/bind.hpp> 
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include "Config.h"
#include "pool.h" 
#include "Server.h"

#include <string>
#include <set>
#include <thread> 
#include <iostream>
#include <mutex>

class Channel;

class PublicSock 
{
	public:
		static void Listen(std::string ip, std::string port);
		static void SSListen(std::string ip, std::string port);
		static void WebListen(std::string ip, std::string port);
};

class User {
	public: 
		User(const std::string server) : mNickName("ZeusiRCd"), mIdent("ZeusiRCd"), mServer(server) {};
		User () {};
		virtual ~User() {}; 
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
		
		bool bAway; 
		bool mode_r;
		bool mode_z;
		bool mode_o;
		bool mode_w;
};

class LocalUser : public User {
	public:
		LocalUser() : User(config->Getvalue("serverName")), mLang(config->Getvalue("language")) {};
		virtual ~LocalUser() { }; 
		static LocalUser *FindLocalUser(std::string nick);
		void Parse(std::string message);
		void CheckPing();
		void CheckNick();
		static bool checkstring(const std::string &str);
		static bool checknick(const std::string &nick);
		static bool checkchan(const std::string &chan); 
		bool addUser(LocalUser* user, std::string nick);
		bool changeNickname(std::string old, std::string recent);
		void removeUser(std::string nick);
		void Cycle();
		int Channs();
		void Exit(); 
		void SendAsServer(const std::string message);
		virtual void Send(std::string message) = 0;
		virtual void Close() = 0;
		
		time_t bPing;
		
		bool bSentPass = false;
		bool bSentUser = false;
		bool bSentNick = false; 
		bool bSentMotd = false;
		bool quit = false;
		bool away_notify = false;
		bool userhost_in_names = false;
		
		std::string PassWord;
		std::string mLang;
		std::set<Channel*> mChannels;
		std::mutex mtx;
};

class PlainUser : public LocalUser, public std::enable_shared_from_this<PlainUser> {
	public:
		PlainUser(const boost::asio::executor& ex) : Socket(ex), strand(ex), mBuffer(2048), deadline(ex) {};
		 ~PlainUser() { deadline.cancel(); };  
		void Send(std::string message);
		void Close();
		void start();
		std::string ip();
		void read();
		void write();
		void handleWrite(const boost::system::error_code& error, std::size_t bytes);
		void handleRead(const boost::system::error_code& error, std::size_t bytes); 
		void check_ping(const boost::system::error_code &e);  
		boost::asio::ip::tcp::socket Socket; 
		boost::asio::strand<boost::asio::executor> strand; 
		boost::asio::streambuf mBuffer; std::string Queue; bool finish = true; 
		boost::asio::deadline_timer deadline;
};

class LocalSSLUser : public LocalUser, public std::enable_shared_from_this<LocalSSLUser> {
	public:
		LocalSSLUser(const boost::asio::executor& ex, boost::asio::ssl::context &ctx) : Socket(ex, ctx), strand(ex), mBuffer(2048), deadline(ex) {}; 
		~LocalSSLUser() { deadline.cancel(); };
		void Send(std::string message); 
		void Close();
		void start();
		std::string ip();
		void read();
		void write();
		void handleWrite(const boost::system::error_code& error, std::size_t bytes);
		void handleRead(const boost::system::error_code& error, std::size_t bytes);
		void check_ping(const boost::system::error_code &e);  
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> Socket; 
		boost::asio::strand<boost::asio::executor> strand; 
		boost::asio::streambuf mBuffer; std::string Queue; bool finish = true; 
		boost::asio::deadline_timer deadline;
};

class LocalWebUser : public LocalUser, public std::enable_shared_from_this<LocalWebUser> {
	public:
		LocalWebUser(const boost::asio::executor& ex, boost::asio::ssl::context &ctx) : Socket(ex, ctx), strand(ex), mBuffer(2048), deadline(ex) {}; 
		~LocalWebUser() { deadline.cancel(); };
		void Send(std::string message); 
		void Close();
		void start();
		std::string ip();
		void read();
		void write();
		void handleWrite(const boost::system::error_code& error, std::size_t bytes);
		void handleRead(const boost::system::error_code& error, std::size_t bytes);
		void check_ping(const boost::system::error_code &e);
		void on_accept(boost::system::error_code ec);  
		boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> Socket;
		boost::asio::strand<boost::asio::executor> strand; 
		boost::asio::streambuf mBuffer;
		std::string Queue;
		bool finish = true; 
		bool handshake = false;
		boost::asio::deadline_timer deadline;
};

class RemoteUser : public User {
	public:
		RemoteUser(const std::string server) : User(server) {}; ~RemoteUser() {};
		static RemoteUser *FindRemoteUser(std::string nick);
		bool addUser(RemoteUser* user, std::string nick);
		bool changeNickname(std::string old, std::string recent);
		void removeUser(std::string nick);
};

class ClientServer : public Server {
	public:
		ClientServer(size_t num_threads, boost::asio::io_context& io_context, const std::string s_ip, int s_port)
		: io_context_pool_(num_threads), mAcceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(s_ip), s_port)) { 
			mAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			mAcceptor.listen(boost::asio::socket_base::max_listen_connections);
		}  
		void plain();
		void ssl();
		void wss();
		void run();
		void handleAccept(const std::shared_ptr<PlainUser> newclient, const boost::system::error_code& error);
		void handleSSLAccept(const std::shared_ptr<LocalSSLUser> newclient, const boost::system::error_code& error);
		void handle_handshake_ssl(const std::shared_ptr<LocalSSLUser>& newclient, const boost::system::error_code& error);
		void handleWebAccept(const std::shared_ptr<LocalWebUser> newclient, const boost::system::error_code& error);
		void handle_handshake_web(const std::shared_ptr<LocalWebUser>& newclient, const boost::system::error_code& error);
		void check_deadline(const std::shared_ptr<PlainUser> newclient, const boost::system::error_code &e);
		void check_deadline_ssl(const std::shared_ptr<LocalSSLUser> newclient, const boost::system::error_code &e);
		void check_deadline_web(const std::shared_ptr<LocalWebUser> newclient, const boost::system::error_code &e);
		void on_accept(const std::shared_ptr<LocalWebUser>& newclient, boost::system::error_code ec);
		io_context_pool io_context_pool_;
		boost::asio::ip::tcp::acceptor mAcceptor;
};
