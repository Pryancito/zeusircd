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

class UserSock : public CTCPServer
{
	public:
		UserSock(CTCPServer const &server) : CTCPServer(server) {};
		ASocket::Socket ConnectedClient;
		void Receive();
		void Send(const std::string message);
		void Close();
};

class UserSSLSock : public CTCPSSLServer
{
	public:
		UserSSLSock(CTCPSSLServer const &server) : CTCPSSLServer(server) {};
		ASecureSocket::SSLSocket ConnectedClient;
		void Receive();
		void Send(const std::string message);
		void Close();
};

class User
{
	public:
		User() {};
};

class LocalUser : public User, public UserSock
{
	public:
		LocalUser(CTCPServer server) : UserSock(server) {};
		void start();
};

class LocalSSLUser : public User, public UserSSLSock
{
	public:
		LocalSSLUser(CTCPSSLServer server) : UserSSLSock(server) {};
		void start();
};

class LocalWebUser : public WebSocketServer
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
};
