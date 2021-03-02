#include "Config.h"
#include "module.h"
#include <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;
using namespace std;

class WEB_UP : public Module
{
  public:
	std::thread *th;
	WEB_UP() : Module("", 50, false) {
		th = new std::thread(&WEB_UP::init, this);
		th->detach();
	};
	~WEB_UP() { close(); };
	void init () {
		for(;;)
		{
			sleep(300);
			boost::asio::io_service io_service;
			string ipAddress = "servers.zeusircd.net";
			string portNum = "80";
			string hostAddress;
			if (portNum.compare("80") != 0)
			{
				 hostAddress = ipAddress + ":" + portNum;
			}
			else 
			{ 
				hostAddress = ipAddress;
			}
			string queryStr = "/upload.php?network=" + config["network"].as<std::string>() +
				"&server=" + config["serverName"].as<std::string>() + "&hub=" + config["hub"].as<std::string>() +
				"&users=" + std::to_string(Users.size()) + "&language=" + config["language"].as<std::string>() +
				"&channels=" + std::to_string(Channels.size()) + "&time=" + std::to_string(time(0));

			// Get a list of endpoints corresponding to the server name.
			boost::system::error_code ec;
			tcp::resolver resolver(io_service);
			tcp::resolver::query query(ipAddress, portNum);
			tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
			if (ec)
				continue;
			// Try each endpoint until we successfully establish a connection.
			tcp::socket socket(io_service);
			boost::asio::connect(socket, endpoint_iterator);

			// Form the request. We specify the "Connection: close" header so that the
			// server will close the socket after transmitting the response. This will
			// allow us to treat all data up until the EOF as the content.
			boost::asio::streambuf request;
			std::ostream request_stream(&request);
			request_stream << "GET " << queryStr << " HTTP/1.0\r\n";  // note that you can change it if you wish to HTTP/1.0
			request_stream << "Host: " << hostAddress << "\r\n";
			request_stream << "Accept: */*\r\n";
			request_stream << "Connection: close\r\n\r\n";

			// Send the request.
			boost::asio::write(socket, request);
		}
		return;
	}
	void close() {
		delete th;
	}
	virtual void command(User *user, std::string message) override {}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new WEB_UP);
}
