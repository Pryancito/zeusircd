#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/system/error_code.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "mainframe.h"
#include "user.h"
#include "parser.h"

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
namespace ssl = boost::asio::ssl;
extern boost::asio::io_context channel_user_context;

void
fail(boost::system::error_code ec, std::string what)
{
    std::cout << "ERROR WebSockets: " << what << ": " << ec.message() << std::endl;
}
/*
class session : public std::enable_shared_from_this<session>
{
    tcp::socket socket_;
    websocket::stream<ssl::stream<tcp::socket&>> wss_;
    boost::asio::strand<
        boost::asio::io_context::executor_type> strand_;
    boost::beast::multi_buffer buffer_;
    bool ssl = false;
    User mUser;

public:
    // Take ownership of the socket
    explicit
    session(tcp::socket socket, ssl::context& ctx, Session *newclient)
        : socket_(std::move(socket))
        , wss_(socket_, ctx)
        , strand_(wss_.get_executor())
        , mUser(newclient, config->Getvalue("serverName"))
    {
    }

    // Start the asynchronous operation
    void
    run()
    {
        wss_.async_accept(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &session::on_accept,
                    shared_from_this(),
                    std::placeholders::_1)));
    }
    void
    on_accept(boost::system::error_code ec)
    {
        if(ec)
            return fail(ec, "accept_session");
        do_read();
    }

    void
    do_read()
    {
		wss_.async_read(
			buffer_,
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&session::on_read,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2)));
    }

    void
    on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if(ec == websocket::error::closed)
            return;

        if(ec)
            fail(ec, "read");

		std::ostringstream os; os << boost::beast::buffers(buffer_.data());
		std::string message = os.str();
        
        if (message.find('\n') != std::string::npos) {
			size_t tam = message.length();
			message.erase(tam-1);
		}
        if (message.find('\r') != std::string::npos) {
			size_t tam = message.length();
			message.erase(tam-1);
		}
		
		unsigned char opcode = (unsigned char)message.c_str()[0];

		switch (opcode)
		{
			case 0x00:
			case 0x01:
			case 0x02:
			{
				Parser::parse(message.substr(1), &mUser);
			}

			case 0x09:
			{
				mUser.UpdatePing();
			}

			case 0x0a:
			{
				mUser.UpdatePing();
			}

			case 0x08:
			{
				wss_.lowest_layer().close();
			}

			default:
			{
				wss_.lowest_layer().close();
			}
		}
	}
	
    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Do another read
        do_read();
    }
};*/

class listener : public std::enable_shared_from_this<listener>
{
    tcp::acceptor acceptor_;
    boost::asio::deadline_timer deadline;
public:
    listener(
        boost::asio::io_context& ioc,
        tcp::endpoint endpoint)
        : acceptor_(ioc)
        , deadline(channel_user_context)
    {
        boost::system::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        if(! acceptor_.is_open())
            return;
        do_accept();
    }
    
    void
    do_accept()
    {
		boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12);
		ctx.set_options(
		boost::asio::ssl::context::default_workarounds
		| boost::asio::ssl::context::no_sslv2
		| boost::asio::ssl::context::single_dh_use);
		ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
		ctx.use_certificate_chain_file("server.pem");
		ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
		ctx.use_tmp_dh_file("dh.pem");
		Session *newclient = new Session(acceptor_.get_io_context(), ctx);
		acceptor_.async_accept(
			newclient->socket_wss().lowest_layer(),
			std::bind(
				&listener::on_accept,
				shared_from_this(),
				std::placeholders::_1,
				newclient));
    }
	void
	handle_handshake(Session *newclient, const boost::system::error_code& error) {
		deadline.cancel();
		if (error){
			newclient->close();
		} else {
			Server::ThrottleUP(newclient->ip());
			newclient->websocket = true;
			newclient->start();
		}
	}
	void check_deadline(Session *newclient, const boost::system::error_code &e)
	{
		if (!e) {
			newclient->socket_wss().lowest_layer().close();
		}
	}
    void
    on_accept(boost::system::error_code ec, Session *newclient)
    {
        if(ec)
        {
            fail(ec, "accept_listener");
            newclient->close();
        } else if (Server::CheckClone(newclient->ip()) == true) {
			newclient->send(config->Getvalue("serverName") + " Has superado el numero maximo de clones." + config->EOFMessage);
			newclient->close();
		} else if (Server::CheckDNSBL(newclient->ip()) == true) {
			newclient->send(config->Getvalue("serverName") + " Tu IP esta en nuestras listas DNSBL." + config->EOFMessage);
			newclient->close();
		} else if (Server::CheckThrottle(newclient->ip()) == true) {
			newclient->send(config->Getvalue("serverName") + " Te conectas demasiado rapido, espera 30 segundos para volver a conectarte." + config->EOFMessage);
			newclient->close();
		} else {
			newclient->socket_wss().next_layer().async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&listener::handle_handshake,   this,   newclient,  boost::asio::placeholders::error));
			deadline.expires_from_now(boost::posix_time::seconds(10));
			deadline.async_wait(boost::bind(&listener::check_deadline, this, newclient, boost::asio::placeholders::error));
		}
        do_accept();
    }
};

WebSocket::WebSocket(boost::asio::io_context& io_context, std::string ip, int port, bool ssl, bool ipv6)
{
	auto const address = boost::asio::ip::make_address(ip);
	std::make_shared<listener>(io_context, tcp::endpoint{address, (unsigned short ) port})->run();
}
