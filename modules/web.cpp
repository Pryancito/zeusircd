#include "mainframe.h"
#include "Config.h"
#include "module.h"
#include <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;

class WEB_UP : public Module
{
  public:
	std::thread *th;
	boost::asio::io_context ioc;
	boost::asio::deadline_timer deadline;
	WEB_UP() : Module("", 50, false) , deadline(ioc.get_executor()) {
		th = new std::thread(&WEB_UP::init, this);
		th->detach();
	};
	~WEB_UP() { close(); };
	void send () {
		deadline.expires_from_now(boost::posix_time::seconds(300));
		deadline.async_wait(boost::bind(&WEB_UP::send, this));

		// Get a list of endpoints corresponding to the server name.
		tcp::resolver resolver(ioc);
		tcp::resolver::results_type endpoints = resolver.resolve("servers.zeusircd.net", "http");

		// Try each endpoint until we successfully establish a connection.
		tcp::socket socket(ioc);
		boost::asio::connect(socket, endpoints);

		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		request_stream << "GET " << "/upload.php?network=" + config["network"].as<std::string>() +
					"&server=" + config["serverName"].as<std::string>() + "&hub=" + config["hub"].as<std::string>() +
					"&users=" + std::to_string(Mainframe::instance()->countusers()) + "&language=" + config["language"].as<std::string>() +
					"&channels=" + std::to_string(Mainframe::instance()->countchannels()) + "&time=" + std::to_string(time(0)) + "\n" << " HTTP/1.0\r\n";
		request_stream << "Host: " << "servers.zeusircd.net" << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Connection: close\r\n\r\n";

		// Send the request.
		boost::asio::write(socket, request);
	}
	void init ()
	{
		send();
		ioc.run();
	}
	void close() {
		ioc.stop();
		delete th;
	}
	virtual void command(LocalUser *user, std::string message) override {}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new WEB_UP);
}
