#include "TCPServer.h"
#include "TCPSSLServer.h"
#include "WebSocketServer.h"
#include "Timer.h"

#include <string>

extern TimerWheel timers;
typedef std::function<void()> Callback;

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
		User() {};
		~User() {};
        std::string mNickName;
		std::string mIdent;
        std::string mHost;
		std::string mCloak;
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
		LocalUser() : tnick(this), tping(this) {};
		~LocalUser() { };
		void StartTimers(TimerWheel* timers);
		void Parse(std::string message);
		void CheckPing();
		void CheckNick();
		virtual void Send(const std::string message) = 0;
		virtual void Close() = 0;
		time_t bPing;
        
        bool bSentPass = false;
        bool bSentUser = false;
        bool bSentNick = false;
        bool bSentMotd = false;
		bool quit = false;
		
		std::string PassWord;
        std::string mLang;
        
        MemberTimerEvent<LocalUser, &LocalUser::CheckNick> tnick;
        MemberTimerEvent<LocalUser, &LocalUser::CheckPing> tping;
};

class PlainUser : public LocalUser, public CTCPServer
{
	public:
		PlainUser(CTCPServer server) : CTCPServer(server) {};
		~PlainUser() {};
		ASocket::Socket ConnectedClient;
		void Receive();
		void Send(const std::string message);
		void Close();
		void start();
};

class LocalSSLUser : public LocalUser, public CTCPSSLServer
{
	public:
		LocalSSLUser(CTCPSSLServer const &server) : CTCPSSLServer(server) {};
		~LocalSSLUser() {};
		ASecureSocket::SSLSocket ConnectedClient;
		void Receive();
		void Send(const std::string message);
		void Close();
		void start();
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
	private:
		int SocketID;
};

class RemoteUser : public User
{
	public:
		RemoteUser() {};
		~RemoteUser() {};
};
