#include <string>

#include <boost/asio.hpp>

using namespace std;

class WebSocket {
	public:
		WebSocket(boost::asio::io_context& io_context, std::string ip, int port, bool ssl, bool ipv6);
};

