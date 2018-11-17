#pragma once

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

#include <string>

#include <boost/asio.hpp>

using namespace std;

class WebSocket : public gc_cleanup {
	public:
		WebSocket(boost::asio::io_context& io_context, std::string ip, int port, bool ssl, bool ipv6);
};

