#include "session.h"
#include "parser.h"

Session::Session(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx)
:   mUser(this, config->Getvalue("serverName")), mSocket(io_service), mSSL(io_service, ctx), deadline(io_service) {
}

Servidor::Servidor(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx)
:   mSocket(io_service), mSSL(io_service, ctx) {}

Session::pointer Session::create(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx) {
    return Session::pointer(new Session(io_service, ctx));
}

Servidor::pointer Servidor::servidor(boost::asio::io_service& io_service, boost::asio::ssl::context &ctx) {
    return Servidor::pointer(new Servidor(io_service, ctx));
}

void Session::start() {
	read();
	send("PING :" + config->Getvalue("serverName") + config->EOFMessage);
	deadline.expires_from_now(boost::posix_time::seconds(10));
	deadline.async_wait(boost::bind(&Session::check_deadline, this));
}

void Session::close() {
	if (ssl == true) {
		mSSL.lowest_layer().close();
	} else {
		mSocket.close();
	}
}

void Session::check_deadline()
{
	if (mUser.connclose() == true)
		close();
	deadline.cancel();
}

void Session::read() {
	if (ssl == true && mSSL.lowest_layer().is_open()) {
		boost::asio::async_read_until(mSSL, mBuffer, '\n',
                                  boost::bind(&Session::handleRead, shared_from_this(),
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
	} else if (ssl == false && mSocket.is_open()) {
		boost::asio::async_read_until(mSocket, mBuffer, '\n',
                                  boost::bind(&Session::handleRead, shared_from_this(),
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
	}
}

void Session::handleRead(const boost::system::error_code& error, std::size_t bytes) {
	if (error == boost::asio::error::eof)
		close();
	else if (bytes == 0)
		read();
	else {
        std::string message;
        std::istream istream(&mBuffer);
        std::getline(istream, message);
        
        if (message.find('\n') != std::string::npos) {
			size_t tam = message.length();
			message.erase(tam-1);
		}
        if (message.find('\r') != std::string::npos) {
			size_t tam = message.length();
			message.erase(tam-1);
		}

        Parser::parse(message, &mUser);
		read();
    }
}

void Session::send(const std::string& message) {
    if (message.length() > 0 && mUser.server() == config->Getvalue("serverName")) {
		boost::system::error_code ignored_error;
		if (ssl == true && mSSL.lowest_layer().is_open()) {
			boost::asio::write(mSSL, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
		} else if (ssl == false && mSocket.is_open()) {
			boost::asio::write(mSocket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
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

std::string Session::ip() const {
	if (ssl == true && mSSL.lowest_layer().is_open())
		return mSSL.lowest_layer().remote_endpoint().address().to_string();
	else if (mSocket.is_open())
		return mSocket.remote_endpoint().address().to_string();
	else return "127.0.0.0";
}
