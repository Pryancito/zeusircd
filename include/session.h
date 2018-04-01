#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include <string>
#include <boost/asio/ssl.hpp>
#include <iostream>

#include "defines.h"
#include "user.h"

class Servidor;

class Servidores
{
	public:
		Servidor *server;
		std::string nombre;
		std::string ipaddress;
		std::string hub;
		std::vector <std::string> connected;
		
		Servidores (Servidor *servidor, std::string name, std::string ip);
		std::string name();
		std::string ip();
		Servidor *link();
};

class Servidor : public boost::enable_shared_from_this<Servidor>
{
	private:
		Session* mSession;
		boost::asio::ip::tcp::socket mSocket;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> mSSL;
		std::string nombre;
		std::string ipaddress;
		std::string hub;
		int maxusers;
		int maxchannels;

	public:
		typedef boost::shared_ptr<Servidor> pointer;
		static pointer  servidor(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx);
        static pointer  servidor_ssl(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx);
		std::vector <std::string> connected;
		Servidor(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx);
		boost::asio::ip::tcp::socket& socket();
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket_ssl();
		bool ssl;
		void Procesar();
		void close();
		void send(const std::string& message);
		static void sendall(const std::string& message);
		static void sendallbutone(Servidor *server, const std::string& message);
		static void Connect(std::string ipaddr, std::string port);
		void SendBurst (Servidor *server);
		static Servidores* addServer(Servidor *servidor, std::string name, std::string ip);
		static bool IsAServer (std::string ip);
		static bool IsConected (std::string ip);
		static bool Exists (std::string name);
		std::string name();
		std::string ip();
		void Message(Servidor *server, std::string message);
		static void SQUIT(std::string nombre);
};

typedef std::set<Servidores*> 	ServerSet;

class Session : public boost::enable_shared_from_this<Session> {
    
public:
		~Session() { };
		typedef boost::shared_ptr<Session> pointer;

        static pointer  create(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx);
        static pointer  create_ssl(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx);
        
		void start();

		void sendAsServer(const std::string& message);
        void sendAsUser(const std::string& message);

        void send(const std::string& message);
		void close();
		boost::asio::ip::tcp::socket& socket();
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket_ssl();
        std::string ip() const;
        bool ssl;

private:

		Session(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx);
		void Server(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx);
		void read();
		void handleRead(const boost::system::error_code& error, std::size_t bytes);
		
        User mUser;
		boost::asio::ip::tcp::socket mSocket;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> mSSL;
        boost::asio::streambuf mBuffer;
};
