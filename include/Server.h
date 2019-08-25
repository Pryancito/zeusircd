#pragma once

#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>

class Server {
	public:
		Server() {};
		~Server() {};
		static bool CanConnect(const std::string ip);
		static bool CheckClone(const std::string &ip);
		static bool CheckThrottle(const std::string &ip);
		static void ThrottleUP(const std::string &ip);
		static bool HUBExiste();
		static void sendall(const std::string message);
		
		std::string nombre;
		std::string ip;
        int port;
        std::vector <std::string> connected;
};

class LocalServer : public Server
{
	public:
		LocalServer() {};
		~LocalServer() {};
		time_t sPing;
		virtual void Send(const std::string message) = 0;
		virtual void Close() = 0;
};
/*
class PlainServer : public LocalServer, public CTCPServer
{
	public:
		PlainServer(CTCPServer server) : CTCPServer(server) {};
		~PlainServer() {};
		ASocket::Socket ConnectedClient;
		void Receive();
		void Send(const std::string message);
		void Close();
		void start();
};

class SSLServer : public LocalServer, public CTCPSSLServer
{
	public:
		SSLServer(CTCPSSLServer const &server) : CTCPSSLServer(server) { ssl = true; };
		~SSLServer() {};
		ASecureSocket::SSLSocket ConnectedClient;
		void Receive();
		void Send(const std::string message);
		void Close();
		void start();
};*/

class RemoteServer : public Server
{
	public:
		RemoteServer() {};
		~RemoteServer() {};
};
