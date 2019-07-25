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
#include <memory>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include <string>
#include <vector>
#include <set>

#include "ircv3.h"

class User;

extern boost::asio::io_context channel_user_context;
extern boost::asio::ssl::context fakectx;
extern boost::asio::io_context fake;
class Session : public std::enable_shared_from_this<Session>
{
    
public:
		Session(const boost::asio::executor& ex, boost::asio::ssl::context &ctx)
			:   ssl(false), websocket(false), LocalUser(true), deadline_s(channel_user_context), mSocket(ex), mSSL(ex, ctx), wss_(ex, ctx),
			mBuffer(2048), ws_ready(false), strand(boost::asio::make_strand(ex)) {
		}
		Session()
		: LocalUser(false)
		, deadline_s(fake)
		, mSocket(fake.get_executor())
		, mSSL(fake.get_executor(), fakectx)
		, wss_(fake.get_executor(), fakectx) {}
		virtual ~Session () {}
        
		void start();
		void sendAsServer(const std::string message);
        void sendAsUser(const std::string message);
		void handleWrite(const boost::system::error_code& error, std::size_t bytes);
		void on_accept(boost::system::error_code ec);
		void handleWS(const boost::system::error_code& error, std::size_t bytes);
        void send(const std::string message);
        void write();
        void Procesar(); 
		void close();
		void on_close(boost::system::error_code ec);
		void on_shutdown(boost::beast::error_code ec);
		boost::asio::ip::tcp::socket& socket();
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket_ssl();
		boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>& socket_wss();
        std::string ip() const;
        void check_deadline(const boost::system::error_code &e);
        bool ssl = false;
        bool websocket = false;
        bool LocalUser = false;
		boost::asio::deadline_timer deadline_s;
		
		virtual void Exit() = 0;
		virtual bool connclose() = 0;
		virtual std::string messageHeader() = 0;
		virtual User *mUser() = 0;
		virtual bool propquit() = 0;
private:
		void read();
		void handleRead(const boost::system::error_code& error, std::size_t bytes);
		boost::asio::ip::tcp::socket mSocket;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> mSSL;
		boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> wss_;
		boost::asio::streambuf mBuffer;
       	bool ws_ready = false;
		std::string Queue;
		bool finish = true;
		std::mutex mtx;
		boost::asio::strand<boost::asio::executor> strand;
};

class Channel;

typedef std::vector<std::string> StrVec;
typedef std::set<Channel*> ChannelSet;

class User : public Session {
   public:

        User(const boost::asio::executor& ex, boost::asio::ssl::context &ctx, std::string server);
        User(std::string server);
        ~User();
        void cmdNick(const std::string& newnick);
        void cmdUser(const std::string& ident);
		void cmdQuit();
        void cmdJoin(Channel* channel);
        void cmdPart(Channel* channel);
        void cmdKick(User* victim, const std::string& reason, Channel* channel);
        void cmdPing(const std::string &response);
        void cmdWebIRC(const std::string& ip);
        void cmdAway(const std::string &away, bool on);
		void UpdatePing();
		void setPass(const std::string& password);
		void Exit();
		User *mUser();
		
		bool ispassword();
		time_t GetPing();
		time_t GetLogin();

        Ircv3 iRCv3() const;
        std::string nick() const;
        std::string ident() const;
        std::string host() const;
        std::string cloak();
        std::string sha() const;
        std::string server() const;
        std::string messageHeader();
        std::string getPass();
        bool connclose();
        bool propquit();
        
        void setNick(const std::string& nick);
        void setHost(const std::string& host);
        void SetLang(const std::string lang);
        std::string GetLang() const;
        void setMode(char mode, bool option);
        bool getMode(char mode);
        void Cycle();
        void SNICK(const std::string &nickname, const std::string &ident, const std::string &host, const std::string &cloak, std::string login, std::string modos);
        void SUSER(const std::string& ident);
        void SJOIN(Channel* channel);
        void SKICK(Channel* channel);
        void QUIT();
        void NETSPLIT();
        void WEBIRC(const std::string& ip);
        void NICK(const std::string& nickname);
        void propagatenick(const std::string &nickname);
        int Channels();
        bool canchangenick();
		void check_ping(const boost::system::error_code &e);
		bool is_away();
		std::string away_reason();
		
private:

		Ircv3 mIRCv3;
		
        std::string mIdent;
        std::string mNickName;
        std::string mHost;
		std::string mCloak;
		std::string mServer;
		std::string PassWord;
        std::string mAway;
        std::string mLang;
        
        bool bSentUser;
        bool bSentNick;
        bool bSentMotd;
        bool bProperlyQuit;
        bool bSentPass;
		time_t bPing;
		time_t bLogin;
		bool bAway;
		bool bSentQuit;

        bool mode_r;
        bool mode_z;
        bool mode_o;
        bool mode_w;

		boost::asio::deadline_timer deadline_u;

        ChannelSet mChannels;
};

