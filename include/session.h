#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include <string>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <mutex>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/shared_ptr.hpp>

#include "defines.h"
#include "user.h"

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

extern boost::asio::io_context channel_user_context;

class Servidor;

class Servidores
{
	public:
		Servidor *server;
		std::string nombre;
		std::string ipaddress;
		time_t sPing;
		std::vector <std::string> connected;
		
		Servidores (Servidor *servidor, const std::string &name, const std::string &ip);
		std::string name();
		std::string ip();
		Servidor *link();
		void UpdatePing();
		static void uPing(const std::string &servidor);
		time_t GetPing();
};

class Servidor : public std::enable_shared_from_this<Servidor>, public gc_cleanup
{
	private:
		boost::asio::ip::tcp::socket mSocket;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> mSSL;
		std::string nombre;
		std::string ipaddress;
		bool quit;

	public:
		~Servidor() {};
		Servidor(boost::asio::io_context& io_context, boost::asio::ssl::context &ctx)
		:   mSocket(io_context), mSSL(io_context, ctx), quit(false), ssl(false) {}
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
		static void addServer(Servidor *servidor, std::string name, std::string ip, const std::vector <std::string> &conexiones);
		static void updateServer(std::string name, std::vector <std::string> conexiones);
		void addLink(const std::string &hub, std::string link);
		static bool IsAServer (const std::string &ip);
		static bool IsConected (const std::string &ip);
		static bool Exists (std::string name);
		std::string name();
		std::string ip();
		static int count ();
		void Message(Servidor *server, std::string message);
		static void SQUIT(std::string nombre);
		bool isQuit();
		void setQuit();
		void setname(const std::string &name);
};

typedef std::set<Servidores*> 	ServerSet;
typedef std::map<std::string, Servidores*> 	ServerMap;


class Session : public std::enable_shared_from_this<Session>, public gc_cleanup
{
    
public:
		Session(boost::asio::io_context& io_context, boost::asio::ssl::context &ctx)
			:   ssl(false), websocket(false), deadline(channel_user_context), mUser(this, config->Getvalue("serverName")), mSocket(io_context), mSSL(io_context, ctx), wss_(mSocket, ctx),
			ws_ready(false), strand(wss_.get_executor()) {
		}
		~Session () { };
        
		void start();
		void Server(boost::asio::io_context& io_context, boost::asio::ssl::context &ctx);
		void sendAsServer(const std::string& message);
        void sendAsUser(const std::string& message);
		void on_write(boost::system::error_code ec, std::size_t bytes_transferred);
		void on_accept(boost::system::error_code ec);
		void handleWS(const boost::system::error_code& error, std::size_t bytes);
        void send(const std::string& message);
        void handler_send(const boost::system::error_code& error,std::size_t bytes_transferred);
        void async_send(const std::string& message);
		void close();
		boost::asio::ip::tcp::socket& socket();
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket_ssl();
		boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>>& socket_wss();
        std::string ip() const;
        void check_deadline(const boost::system::error_code &e);
        bool ssl;
        bool websocket;
		boost::asio::deadline_timer deadline;
		
private:
		void read();
		void handleRead(const boost::system::error_code& error, std::size_t bytes);
		
        User mUser;
		boost::asio::ip::tcp::socket mSocket;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> mSSL;
		boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>> wss_;
        boost::asio::streambuf mBuffer;
        bool ws_ready;
        boost::asio::strand<boost::asio::io_context::executor_type> strand;
};

#endif
