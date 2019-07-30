#include "TCPServer.h"
#include "TCPSSLServer.h"

#include <future>

class PublicSock
{
	public:
		static void Listen(std::string ip, std::string port);
		static void SSListen(std::string ip, std::string port);
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

class RemoteUser : public User
{
	public:
		RemoteUser() {};
};
