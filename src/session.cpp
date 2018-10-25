#include "session.h"
#include "parser.h"
#include "websocket.h"
#include "utils.h"

#include <boost/range/algorithm/remove_if.hpp>
#include <boost/algorithm/string/classification.hpp>

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

Session::pointer Session::create(boost::asio::io_context& io_context, boost::asio::ssl::context &ctx) {
    return Session::pointer(new (GC) Session(io_context, ctx));
}

Servidor::pointer Servidor::servidor(boost::asio::io_context& io_context, boost::asio::ssl::context &ctx) {
    return Servidor::pointer(new (GC) Servidor(io_context, ctx));
}

void Session::start() {
	read();
	if (websocket == false)
		send("PING :" + config->Getvalue("serverName") + config->EOFMessage);
	deadline.expires_from_now(boost::posix_time::seconds(10));
	deadline.async_wait(boost::bind(&Session::check_deadline, this, boost::asio::placeholders::error));
}

void Session::close() {
	if (websocket == true && wss_.lowest_layer().is_open()) {
		wss_.next_layer().next_layer().close();
	} else if (ssl == true && mSSL.lowest_layer().is_open()) {
		mSSL.lowest_layer().close();
	} else if (mSocket.is_open()) {
		mSocket.close();
	} else return;
}

void Session::check_deadline(const boost::system::error_code &e)
{
	if (!e && mUser.connclose() == true) {
		send(config->Getvalue("serverName") + " " + Utils::make_string("", "The connection has expired.") + config->EOFMessage);
		close();
	}
}

void Session::read() {
	mBuffer.prepare(256*1024);
	if (websocket == true && wss_.lowest_layer().is_open()) {
		wss_.async_read(mBuffer, boost::bind(
										&Session::handleWS, shared_from_this(),
												boost::asio::placeholders::error,
												boost::asio::placeholders::bytes_transferred));
	} else if (ssl == true && mSSL.lowest_layer().is_open()) {
		boost::asio::async_read_until(mSSL, mBuffer, '\n',
                                  boost::bind(&Session::handleRead, shared_from_this(),
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
	} else if (ssl == false && mSocket.is_open()) {
		boost::asio::async_read_until(mSocket, mBuffer, '\n',
                                  boost::bind(&Session::handleRead, shared_from_this(),
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
	} else
		close();
}

void Session::handleRead(const boost::system::error_code& error, std::size_t bytes) {
	if (error)
		close();
	else if (bytes == 0)
		read();
	else {
        std::string message;
        std::istream istream(&mBuffer);
        std::getline(istream, message);
        
        message.erase(boost::remove_if(message, boost::is_any_of("\r\n\t")), message.end());

        Parser::parse(message, &mUser);
		read();
    }
}

void Session::handleWS(const boost::system::error_code& error, std::size_t bytes) {
	if (ws_ready == false) {
		wss_.accept();
		ws_ready = true;
		read();
	} else if (error)
		close();
	else if (bytes == 0)
		read();
	else {
		std::string message;
        std::istream istream(&mBuffer);
        std::getline(istream, message);
        
        message.erase(boost::remove_if(message, boost::is_any_of("\r\n\t")), message.end());

		Parser::parse(message, &mUser);
		read();
    }
}

void Session::send(const std::string& message) {
    if (message.length() > 0 && mUser.server() == config->Getvalue("serverName")) {
		boost::system::error_code ignored_error;
		if (websocket == true && wss_.lowest_layer().is_open()) {
			wss_.write(boost::asio::buffer(std::string(message)), ignored_error);
		} else if (ssl == true && mSSL.lowest_layer().is_open()) {
			boost::asio::write(mSSL, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
		} else if (ssl == false && mSocket.is_open()) {
			boost::asio::write(mSocket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
		}
	}
}

void handler_send(const boost::system::error_code& error,std::size_t bytes_transferred)
{}

void Session::async_send(const std::string& message) {
    if (message.length() > 0 && mUser.server() == config->Getvalue("serverName")) {
		if (websocket == true && wss_.lowest_layer().is_open()) {
			wss_.async_write(boost::asio::buffer(message), handler_send);
		} else if (ssl == true && mSSL.lowest_layer().is_open()) {
			boost::asio::async_write(mSSL, boost::asio::buffer(message), handler_send);
		} else if (ssl == false && mSocket.is_open()) {
			boost::asio::async_write(mSocket, boost::asio::buffer(message), handler_send);
		}
	}
}

void Session::sendAsUser(const std::string& message) {
    send(mUser.messageHeader() + message);
}

void Session::sendAsServer(const std::string& message) {
    send(":"+config->Getvalue("serverName")+" " + message);
}

boost::asio::ip::tcp::socket& Session::socket() { return mSocket; }

boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& Session::socket_ssl() { return mSSL; }

boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>>& Session::socket_wss() { return wss_; }

std::string Session::ip() const {
	if (ssl == true && mSSL.lowest_layer().is_open())
		return mSSL.lowest_layer().remote_endpoint().address().to_string();
	else if (mSocket.is_open())
		return mSocket.remote_endpoint().address().to_string();
	else return "127.0.0.0";
}
