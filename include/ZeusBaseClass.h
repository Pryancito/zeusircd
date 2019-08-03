#include "TCPServer.h"
#include "TCPSSLServer.h"
#include "WebSocketServer.h"

#include <future>
#include <string>

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
};

class LocalUser : public User
{
	public:
		LocalUser() {};
		~LocalUser() {};
		void Parse(std::string message);
		virtual void Send(const std::string message) = 0;
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
		~LocalWebUser( );
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
