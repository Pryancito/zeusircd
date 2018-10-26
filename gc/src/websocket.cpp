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
#include "websocket.h"
#include "utils.h"
#include "services.h"

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
namespace ssl = boost::asio::ssl;
extern boost::asio::io_context channel_user_context;

void
fail(boost::system::error_code ec, const std::string &what)
{
    std::cout << "ERROR WebSockets: " << what << ": " << ec.message() << std::endl;
}

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
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		ctx.set_options(
		boost::asio::ssl::context::default_workarounds
		| boost::asio::ssl::context::no_sslv2
		| boost::asio::ssl::context::single_dh_use);
		ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
		ctx.use_certificate_chain_file("server.pem");
		ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
		ctx.use_tmp_dh_file("dh.pem");
		Session::pointer newclient = Session::create(acceptor_.get_io_context(), ctx);
		acceptor_.async_accept(
			newclient->socket_wss().lowest_layer(),
			std::bind(
				&listener::on_accept,
				shared_from_this(),
				std::placeholders::_1,
				newclient));
    }
	void
	handle_handshake(Session::pointer newclient, const boost::system::error_code& error) {
		deadline.cancel();
		if (error){
			newclient->close();
		} else {
			Server::ThrottleUP(newclient->ip());
			newclient->websocket = true;
			newclient->start();
		}
	}
	void check_deadline(Session::pointer newclient, const boost::system::error_code &e)
	{
		if (!e) {
			newclient->socket_wss().lowest_layer().close();
		}
	}
    void
    on_accept(boost::system::error_code ec, Session::pointer newclient)
    {
        if(ec)
        {
            newclient->sendAsServer("465 :" + Utils::make_string("", "An error happens.") + config->EOFMessage);
            newclient->close();
        } else if (Server::CheckClone(newclient->ip()) == true) {
			newclient->sendAsServer("465 :" + Utils::make_string("", "You have reached the maximum number of clones.") + config->EOFMessage);
			newclient->close();
		} else if (Server::CheckDNSBL(newclient->ip()) == true) {
			newclient->sendAsServer("465 :" + Utils::make_string("", "Your IP is in our DNSBL lists.") + config->EOFMessage);
			newclient->close();
		} else if (Server::CheckThrottle(newclient->ip()) == true) {
			newclient->sendAsServer("465 :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again.") + config->EOFMessage);
			newclient->close();
		} else if (OperServ::IsGlined(newclient->ip()) == true) {
			newclient->sendAsServer("465 :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()) + config->EOFMessage);
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
