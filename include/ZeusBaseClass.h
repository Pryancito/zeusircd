#pragma once

#include "TCPServer.h"
#include "TCPSSLServer.h"
#include "WebSocketServer.h"
#include "Timer.h"
#include "Config.h"
#include "system.h"

#include <string>
#include <set>
#include <thread>

extern TimerWheel timers;
typedef std::function<void()> Callback;

class Channel;

class PublicSock
{
	public:
		static void Listen(std::string ip, std::string port);
		static void SSListen(std::string ip, std::string port);
		static void WebListen(std::string ip, std::string port);
};

class User
{
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

class LocalUser : public User
{
	public:
		LocalUser() : User(config->Getvalue("serverName")), mLang(config->Getvalue("language")), tnick(this), tping(this) {};
		virtual ~LocalUser() { };
		static LocalUser *FindLocalUser(std::string nick);
		void StartTimers(TimerWheel* timers);
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
		void SendAsServer(const std::string message);
		virtual void Send(const std::string message) = 0;
		virtual void Close() = 0;
		time_t bPing;
        
        bool bSentPass = false;
        bool bSentUser = false;
        bool bSentNick = false;
        bool bSentMotd = false;
		bool quit = false;
		bool ssl = false;
		bool websocket = false;
		bool away_notify = false;
		bool userhost_in_names = false;
		
		std::string PassWord;
        std::string mLang;
        std::set<Channel*> mChannels;

        MemberTimerEvent<LocalUser, &LocalUser::CheckNick> tnick;
        MemberTimerEvent<LocalUser, &LocalUser::CheckPing> tping;
};

class PlainUser : public LocalUser, public Poller::Client
{
	public:
		PlainUser(CTCPServer const &server) : Server(std::move(server)), cli(this) {};
		~PlainUser() { };
		ASocket::Socket ConnectedClient;
		CTCPServer Server;
		void Receive();
		void Send(const std::string message);
		void Close();
		void start();
		void Exit();
		int notifyPollEvent(Poller::PollEvent* e);
		void fpool();
		Poller::Client *cli;
};

class LocalSSLUser : public LocalUser, public Poller::Client
{
	public:
		LocalSSLUser(CTCPSSLServer const &server) : Server(std::move(server)), cli(this) { ssl = true; };
		~LocalSSLUser() {};
		ASecureSocket::SSLSocket ConnectedClient;
		CTCPSSLServer Server;
		void Receive();
		void Send(const std::string message);
		void Close();
		void start();
		void Exit();
		int notifyPollEvent(Poller::PollEvent* e);
		void fpool();
		Poller::Client *cli;
};

class LocalWebUser : public LocalUser, public WebSocketServer
{
	public:
		LocalWebUser( std::string ip, std::string port );
		~LocalWebUser();
		virtual void onConnect(    int socketID                        );
		virtual void onMessage(    int socketID, const string& data    );
		virtual void onDisconnect( int socketID                        );
		virtual void onError(	   int socketID, const string& message );
		void Send(const std::string message);
		void Close();
		void Exit();
	private:
		int SocketID;
};

class RemoteUser : public User
{
	public:
		RemoteUser(const std::string server) : User(server) {};
		~RemoteUser() {};
		static RemoteUser *FindRemoteUser(std::string nick);
		bool addUser(RemoteUser* user, std::string nick);
		bool changeNickname(std::string old, std::string recent);
		void removeUser(std::string nick);
};
