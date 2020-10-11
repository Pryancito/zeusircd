
#include "mainframe.h"
#include "Config.h"
#include "module.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>
using namespace std;

class web
{
public:
  web(boost::asio::io_context& io_context,
      const std::string& server, const std::string& path)
  {
        // These objects perform our I/O
        tcp::resolver resolver(io_context);
        beast::tcp_stream stream(io_context);

        // Look up the domain name
        auto const results = resolver.resolve(server, "80");

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, path, 11};
        req.set(http::field::host, server);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
  }
};

class WEB_UP : public Module
{
  public:
	std::thread *th;
	boost::asio::io_context ioc;
	WEB_UP() : Module("", 50, false) {
		th = new std::thread(&WEB_UP::init, this);
		th->detach();
	};
	~WEB_UP() { close(); };
	void init () {

		for(;;)
		{
			web c(ioc, "servers.zeusircd.net", "/upload.php?network=" + config["network"].as<std::string>() +
				"&server=" + config["serverName"].as<std::string>() + "&hub=" + config["hub"].as<std::string>() +
				"&users=" + std::to_string(Mainframe::instance()->countusers()) + "&language=" + config["language"].as<std::string>() +
				"&channels=" + std::to_string(Mainframe::instance()->countchannels()) + "&time=" + std::to_string(time(0)) + "\n"
			);
			sleep(300);
		}
	}
	void close() {
		delete th;
	}
	virtual void command(LocalUser *user, std::string message) override {}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new WEB_UP);
}
