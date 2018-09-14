#include "session.h"
#include "parser.h"
#include "sha1.h"
#include "base64.h"
#include "websocket.h"

extern boost::asio::io_context channel_user_context;
extern std::string sha1(const std::string &string);
string MagicGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

Session::Session(boost::asio::io_context& io_context, boost::asio::ssl::context &ctx)
:   websocket(false), deadline(channel_user_context), mUser(this, config->Getvalue("serverName")), mSocket(io_context), mSSL(io_context, ctx), wss_(mSocket, ctx), ws_ready(false), strand(wss_.get_executor()) {
}

Servidor::Servidor(boost::asio::io_context& io_context, boost::asio::ssl::context &ctx)
:   mSocket(io_context), mSSL(io_context, ctx) {}

Session::pointer Session::create(boost::asio::io_context& io_context, boost::asio::ssl::context &ctx) {
    return Session::pointer(new Session(io_context, ctx));
}

Servidor::pointer Servidor::servidor(boost::asio::io_context& io_context, boost::asio::ssl::context &ctx) {
    return Servidor::pointer(new Servidor(io_context, ctx));
}

void Session::start() {
	read();
	if (websocket == false)
		send("PING :" + config->Getvalue("serverName") + config->EOFMessage);
	deadline.expires_from_now(boost::posix_time::seconds(10));
	deadline.async_wait(boost::bind(&Session::check_deadline, this, boost::asio::placeholders::error));
}

void Session::close() {
	if (websocket == true) {
		wss_.lowest_layer().close();
	} else if (ssl == true) {
		mSSL.lowest_layer().close();
	} else {
		mSocket.close();
	}
}

void Session::check_deadline(const boost::system::error_code &e)
{
	if (!e && mUser.connclose() == true) {
		send(config->Getvalue("serverName") + " La conexion ha expirado." + config->EOFMessage);
		close();
	}
}

void Session::read() {
	if (websocket == true && wss_.lowest_layer().is_open()) {
		std::cout << "READ" << std::endl;
		boost::asio::async_read_until(wss_, WSbuf, "\r\n\r\n",
                                  boost::bind(&Session::handleWS, shared_from_this(),
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

void Session::handleWS(const boost::system::error_code& error, std::size_t bytes) {
	std::cout << "HANDLE" << std::endl;
	if (error)
		close();
	else {
        std::ostringstream os; os << boost::beast::buffers(WSbuf.data()); std::string message = os.str();
		std::cout << message << std::endl;
		if (ws_ready == false) {
			unsigned first = message.find("Sec-WebSocket-Key:");
			unsigned last = message.find("\r\n\r\n");
			std::string key = message.substr (first,last-first);
			
			key.append(MagicGUID);
			std::string sha = sha1(key);
			std::string text = base64_encode((const unsigned char *) sha.c_str(), sha.length());
			
			send("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + text + "\r\n\r\n");
			
			ws_ready = true;
			read();
		} else {
			unsigned char opcode = (unsigned char)message.c_str()[0];
			opcode &= ~(1 << 7);
			switch (opcode)
			{
				case 0x00:
				case 0x01:
				case 0x02:
				{
					Parser::parse(message.substr(1), &mUser);
					read();
				}

				case 0x09:
				{
					mUser.UpdatePing();
					read();
				}

				case 0x0a:
				{
					mUser.UpdatePing();
					read();
				}

				case 0x08:
				{
					close();
				}

				default:
				{
					close();
				}
			}
		}
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
