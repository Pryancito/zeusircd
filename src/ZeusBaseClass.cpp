#include "ZeusBaseClass.h"
#include "Server.h"
#include "pool.h"

#include <boost/range/algorithm/remove_if.hpp>
#include <boost/algorithm/string/classification.hpp>

std::mutex quit_mtx;
extern bool exiting;

void PublicSock::Listen(std::string ip, std::string port)
{
	boost::asio::io_context ios;
	auto work = boost::make_shared<boost::asio::io_context::work>(ios);
	size_t max = std::thread::hardware_concurrency();
	ClientServer server(max, ios, ip, (int) stoi(port));
	server.run();
	server.plain();
	for(;;) {
		try {
			ios.run();
			break;
		} catch (std::exception& e) {
			std::cout << "IOS plain accept failure: " << e.what() << std::endl;
		}
	}
}

void PublicSock::SSListen(std::string ip, std::string port)
{
	boost::asio::io_context ios;
	auto work = boost::make_shared<boost::asio::io_context::work>(ios);
	size_t max = std::thread::hardware_concurrency();
	ClientServer server(max, ios, ip, (int) stoi(port));
	server.run();
	server.ssl();
	for(;;) {
		try {
			ios.run();
			break;
		} catch (std::exception& e) {
			std::cout << "IOS SSL accept failure: " << e.what() << std::endl;
		}
	}
}

void PublicSock::WebListen(std::string ip, std::string port)
{
	boost::asio::io_context ios;
	auto work = boost::make_shared<boost::asio::io_context::work>(ios);
	size_t max = std::thread::hardware_concurrency();
	ClientServer server(max, ios, ip, (int) stoi(port));
	server.run();
	server.wss();
	for(;;) {
		try {
			ios.run();
			break;
		} catch (std::exception& e) {
			std::cout << "IOS plain accept failure: " << e.what() << std::endl;
		}
	}
}

void PlainUser::Send(std::string message)
{
	message.append("\r\n");
	mtx.lock();
	Queue.append(std::move(message));
	mtx.unlock();
	if (finish == true) {
		finish = false;
		write();
	}
}

void PlainUser::write() {
	if (!Queue.empty()) {
		if (Socket.is_open()) {
				boost::asio::async_write(Socket, boost::asio::buffer(Queue, Queue.length()), boost::asio::bind_executor(strand, boost::bind(&PlainUser::handleWrite, shared_from_this(), _1, _2)));
		}
	} else
		finish = true;
}

void PlainUser::handleWrite(const boost::system::error_code& error, std::size_t bytes) {
	if (error) {
		Queue.clear();
		finish = true;
		return;
	}
	mtx.lock();
	Queue.erase(0, bytes);
	mtx.unlock();
	if (!Queue.empty())
		write();
	else {
		finish = true;
		Queue.clear();
	}
}

void PlainUser::Close()
{
	if(Socket.is_open()) {
		quit_mtx.lock();
		Exit();
		quit_mtx.unlock();
		Socket.cancel();
		Socket.close();
	}
}

void LocalSSLUser::Send(std::string message)
{
	message.append("\r\n");
	mtx.lock();
	Queue.append(std::move(message));
	mtx.unlock();
	if (finish == true) {
		finish = false;
		write();
	}
}

void LocalSSLUser::write() {
	if (!Queue.empty()) {
		if (Socket.lowest_layer().is_open()) {
				boost::asio::async_write(Socket, boost::asio::buffer(Queue, Queue.length()), boost::asio::bind_executor(strand, boost::bind(&LocalSSLUser::handleWrite, shared_from_this(), _1, _2)));
		}
	} else
		finish = true;
}

void LocalSSLUser::handleWrite(const boost::system::error_code& error, std::size_t bytes) {
	if (error) {
		Queue.clear();
		finish = true;
		return;
	}
	mtx.lock();
	Queue.erase(0, bytes);
	mtx.unlock();
	if (!Queue.empty())
		write();
	else {
		finish = true;
		Queue.clear();
	}
}

void LocalSSLUser::Close()
{
	if(Socket.lowest_layer().is_open()) {
		quit_mtx.lock();
		Exit();
		quit_mtx.unlock();
		Socket.lowest_layer().cancel();
		Socket.lowest_layer().close();
	}
}

void LocalWebUser::Send(std::string message)
{
	message.append("\r\n");
	if (get_lowest_layer(Socket).socket().is_open()) {
		Socket.write(boost::asio::buffer(message, message.length()));
	}
}

void LocalWebUser::Close()
{
	quit_mtx.lock();
	Exit();
	quit_mtx.unlock();
	boost::beast::close_socket(get_lowest_layer(Socket));
}

void ClientServer::plain()
{
	auto newclient = std::make_shared<PlainUser>(io_context_pool_.get_io_context().get_executor());
	mAcceptor.async_accept(newclient->Socket,
					   boost::bind(&ClientServer::handleAccept,   this,   newclient,  boost::asio::placeholders::error));
	newclient->deadline.expires_from_now(boost::posix_time::seconds(10));
	newclient->deadline.async_wait(boost::bind(&ClientServer::check_deadline, this, newclient, boost::asio::placeholders::error));
}

void ClientServer::ssl()
{
		boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12);
		ctx.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::no_sslv3
        | boost::asio::ssl::context::no_tlsv1_1
        | boost::asio::ssl::context::single_dh_use);
		ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
		ctx.use_certificate_chain_file("server.pem");
		ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
		ctx.use_tmp_dh_file("dh.pem");
		auto newclient = std::make_shared<LocalSSLUser>(io_context_pool_.get_io_context().get_executor(), ctx);
		mAcceptor.async_accept(newclient->Socket.lowest_layer(),
                           boost::bind(&ClientServer::handleSSLAccept,   this,   newclient,  boost::asio::placeholders::error));
		newclient->deadline.expires_from_now(boost::posix_time::seconds(10));
		newclient->deadline.async_wait(boost::bind(&ClientServer::check_deadline_ssl, this, newclient, boost::asio::placeholders::error));
}

void ClientServer::wss()
{
		boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12);
		ctx.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::no_sslv3
        | boost::asio::ssl::context::no_tlsv1_1
        | boost::asio::ssl::context::single_dh_use);
		ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
		ctx.use_certificate_chain_file("server.pem");
		ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
		ctx.use_tmp_dh_file("dh.pem");
		auto newclient = std::make_shared<LocalWebUser>(io_context_pool_.get_io_context().get_executor(), ctx);
		mAcceptor.async_accept(newclient->Socket.next_layer().next_layer().socket(),
                           boost::bind(&ClientServer::handleWebAccept,   this,   newclient,  boost::asio::placeholders::error));
		newclient->deadline.expires_from_now(boost::posix_time::seconds(10));
		newclient->deadline.async_wait(boost::bind(&ClientServer::check_deadline_web, this, newclient, boost::asio::placeholders::error));
}

void ClientServer::run()
{
	io_context_pool_.run();
}

void ClientServer::handleAccept(const std::shared_ptr<PlainUser> newclient, const boost::system::error_code& error) {
	plain();
	if (!error) {
//		if (stoi(config->Getvalue("maxUsers")) <= Mainframe::instance()->countusers() && ssl == false) {
//			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections.") + config->EOFMessage);
//			newclient->Close();
//		} else if (CheckClone(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones.") + config->EOFMessage);
//			newclient->close();
//		} else if (CheckDNSBL(newclient->ip()) == true) {
//			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists.") + config->EOFMessage);
//			newclient->Close();
//		} else if (CheckThrottle(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again.") + config->EOFMessage);
//			newclient->close();
//		} else if (OperServ::IsGlined(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()) + config->EOFMessage);
//			newclient->close();
//		} else if (OperServ::CanGeoIP(newclient->ip()) == false) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You can not connect from your country.") + config->EOFMessage);
//			newclient->close();
//		} else {
			Server::ThrottleUP(newclient->ip());
			std::thread t([newclient] { newclient->start(); });
			t.detach();
//		}
	}
}

void ClientServer::handleSSLAccept(const std::shared_ptr<LocalSSLUser> newclient, const boost::system::error_code& error) {
	ssl();
	if (!error) {
//		if (stoi(config->Getvalue("maxUsers")) <= Mainframe::instance()->countusers() && ssl == false) {
//			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections.") + config->EOFMessage);
//			newclient->Close();
//		} else if (CheckClone(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones.") + config->EOFMessage);
//			newclient->close();
//		} else if (CheckDNSBL(newclient->ip()) == true) {
//			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists.") + config->EOFMessage);
//			newclient->Close();
//		} else if (CheckThrottle(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again.") + config->EOFMessage);
//			newclient->close();
//		} else if (OperServ::IsGlined(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()) + config->EOFMessage);
//			newclient->close();
//		} else if (OperServ::CanGeoIP(newclient->ip()) == false) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You can not connect from your country.") + config->EOFMessage);
//			newclient->close();
//		} else {
			newclient->Socket.async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&ClientServer::handle_handshake_ssl,   this,   newclient,  boost::asio::placeholders::error));
//		}
	}
}

void ClientServer::handleWebAccept(const std::shared_ptr<LocalWebUser> newclient, const boost::system::error_code& error) {
	wss();
	if (!error) {
//		if (stoi(config->Getvalue("maxUsers")) <= Mainframe::instance()->countusers() && ssl == false) {
//			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections.") + config->EOFMessage);
//			newclient->Close();
//		} else if (CheckClone(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones.") + config->EOFMessage);
//			newclient->close();
//		} else if (CheckDNSBL(newclient->ip()) == true) {
//			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists.") + config->EOFMessage);
//			newclient->Close();
//		} else if (CheckThrottle(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again.") + config->EOFMessage);
//			newclient->close();
//		} else if (OperServ::IsGlined(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()) + config->EOFMessage);
//			newclient->close();
//		} else if (OperServ::CanGeoIP(newclient->ip()) == false) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You can not connect from your country.") + config->EOFMessage);
//			newclient->close();
//		} else {
			newclient->Socket.next_layer().async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&ClientServer::handle_handshake_web,   this,   newclient,  boost::asio::placeholders::error));
//		}
	}
}

void ClientServer::handle_handshake_ssl(const std::shared_ptr<LocalSSLUser>& newclient, const boost::system::error_code& error) {
	if (!error) {
//		if (stoi(config->Getvalue("maxUsers")) <= Mainframe::instance()->countusers() && ssl == false) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections.") + config->EOFMessage);
//			newclient->close();
//		} else if (CheckClone(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones.") + config->EOFMessage);
//			newclient->close();
//		} else if (CheckDNSBL(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists.") + config->EOFMessage);
//			newclient->close();
//		} else if (CheckThrottle(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again.") + config->EOFMessage);
//			newclient->close();
//		} else if (OperServ::IsGlined(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()) + config->EOFMessage);
//			newclient->close();
//		} else if (OperServ::CanGeoIP(newclient->ip()) == false) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You can not connect from your country.") + config->EOFMessage);
//			newclient->close();
//		} else {
			ThrottleUP(newclient->ip());
			std::thread t([newclient] { newclient->start(); });
			t.detach();
//		}
	}
}

void ClientServer::handle_handshake_web(const std::shared_ptr<LocalWebUser>& newclient, const boost::system::error_code& error) {
	if (!error) {
//		if (stoi(config->Getvalue("maxUsers")) <= Mainframe::instance()->countusers() && ssl == false) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections.") + config->EOFMessage);
//			newclient->close();
//		} else if (CheckClone(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones.") + config->EOFMessage);
//			newclient->close();
//		} else if (CheckDNSBL(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists.") + config->EOFMessage);
//			newclient->close();
//		} else if (CheckThrottle(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again.") + config->EOFMessage);
//			newclient->close();
//		} else if (OperServ::IsGlined(newclient->ip()) == true) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()) + config->EOFMessage);
//			newclient->close();
//		} else if (OperServ::CanGeoIP(newclient->ip()) == false) {
//			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You can not connect from your country.") + config->EOFMessage);
//			newclient->close();
//		} else {
			newclient->Socket.async_accept(boost::bind(&ClientServer::on_accept, this, newclient, boost::asio::placeholders::error));
//		}
	}
}

void ClientServer::on_accept(const std::shared_ptr<LocalWebUser>& newclient, boost::system::error_code ec)
{
	if (!ec) {
		ThrottleUP(newclient->ip());
		std::thread t([newclient] { newclient->start(); });
		t.detach();
	} else
		newclient->Close();
}

std::string PlainUser::ip()
{
	if (Socket.is_open())
		return Socket.remote_endpoint().address().to_string();
	else
		return "127.0.0.0";
}

std::string LocalSSLUser::ip()
{
	if (Socket.lowest_layer().is_open())
			return Socket.lowest_layer().remote_endpoint().address().to_string();
	else
		return "127.0.0.0";
}

std::string LocalWebUser::ip()
{
	if (get_lowest_layer(Socket).socket().is_open())
			return Socket.next_layer().next_layer().socket().remote_endpoint().address().to_string();
	else
		return "127.0.0.0";
}

void ClientServer::check_deadline(const std::shared_ptr<PlainUser> newclient, const boost::system::error_code &e)
{
	if (!e && newclient->bSentNick == false)
		newclient->Close();
}

void ClientServer::check_deadline_ssl(const std::shared_ptr<LocalSSLUser> newclient, const boost::system::error_code &e)
{
	if (!e && newclient->bSentNick == false)
		newclient->Close();
}

void ClientServer::check_deadline_web(const std::shared_ptr<LocalWebUser> newclient, const boost::system::error_code &e)
{
	if (!e && newclient->bSentNick == false)
		newclient->Close();
}

void PlainUser::start()
{
	read();
	deadline.cancel();
	deadline.expires_from_now(boost::posix_time::seconds(60));
	deadline.async_wait(boost::bind(&PlainUser::check_ping, this, boost::asio::placeholders::error));
}

void PlainUser::check_ping(const boost::system::error_code &e) {
	if (!e) {
		if (bPing + 200 < time(0)) {
			deadline.cancel();
			Close();
		} else {
			Send("PING :" + config->Getvalue("serverName"));
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(60));
			deadline.async_wait(boost::bind(&PlainUser::check_ping, this, boost::asio::placeholders::error));
		}
	}
}

void PlainUser::read() {
	
	if (Socket.is_open()) {
		boost::asio::async_read_until(Socket, mBuffer, '\n', boost::asio::bind_executor(strand,
			boost::bind(&PlainUser::handleRead, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)));
	}
}

void PlainUser::handleRead(const boost::system::error_code& error, std::size_t bytes) {
	if (!error) {
		std::string message;
		std::istream istream(&mBuffer);
		std::getline(istream, message);

		message.erase(boost::remove_if(message, boost::is_any_of("\r\n")), message.end());
		
		if (message.length() > 1024)
			message.substr(0, 1024);

		if (!bIsQuit)
			boost::asio::post(strand, boost::bind(&LocalUser::Parse, shared_from_this(), message));

		if (message.substr(0, 4) == "QUIT")
			bIsQuit = true;

		read();
	} else
		Close();
}

void LocalSSLUser::start()
{
	read();
	deadline.cancel();
	deadline.expires_from_now(boost::posix_time::seconds(60));
	deadline.async_wait(boost::bind(&LocalSSLUser::check_ping, this, boost::asio::placeholders::error));
}

void LocalSSLUser::check_ping(const boost::system::error_code &e) {
	if (!e) {
		if (bPing + 200 < time(0)) {
			deadline.cancel();
			Close();
		} else {
			Send("PING :" + config->Getvalue("serverName"));
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(60));
			deadline.async_wait(boost::bind(&LocalSSLUser::check_ping, this, boost::asio::placeholders::error));
		}
	}
}

void LocalSSLUser::read() {
	
	if (Socket.lowest_layer().is_open()) {
		boost::asio::async_read_until(Socket, mBuffer, '\n', boost::asio::bind_executor(strand,
			boost::bind(&LocalSSLUser::handleRead, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)));
	}
}

void LocalSSLUser::handleRead(const boost::system::error_code& error, std::size_t bytes) {
	if (!error) {
		std::string message;
		std::istream istream(&mBuffer);
		std::getline(istream, message);

		message.erase(boost::remove_if(message, boost::is_any_of("\r\n")), message.end());
		
		if (message.length() > 1024)
			message.substr(0, 1024);

		if (!bIsQuit)
			boost::asio::post(strand, boost::bind(&LocalSSLUser::Parse, shared_from_this(), message));

		if (message.substr(0, 4) == "QUIT")
			bIsQuit = true;

		read();
	} else
		Close();
}

void LocalWebUser::start()
{
	read();
	deadline.cancel();
	deadline.expires_from_now(boost::posix_time::seconds(60));
	deadline.async_wait(boost::bind(&LocalWebUser::check_ping, this, boost::asio::placeholders::error));
}

void LocalWebUser::check_ping(const boost::system::error_code &e) {
	if (!e) {
		if (bPing + 200 < time(0)) {
			deadline.cancel();
			Close();
		} else {
			Send("PING :" + config->Getvalue("serverName"));
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(60));
			deadline.async_wait(boost::bind(&LocalWebUser::check_ping, this, boost::asio::placeholders::error));
		}
	}
}

void LocalWebUser::read() {
	
	if (get_lowest_layer(Socket).socket().is_open()) {
		boost::asio::async_read_until(Socket, mBuffer, '\n', boost::asio::bind_executor(strand,
			boost::bind(&LocalWebUser::handleRead, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)));
	}
}

void LocalWebUser::handleRead(const boost::system::error_code& error, std::size_t bytes) {
	if (!error) {
		std::string message;
		std::istream istream(&mBuffer);
		std::getline(istream, message);

		message.erase(boost::remove_if(message, boost::is_any_of("\r\n")), message.end());
		
		if (message.length() > 1024)
			message.substr(0, 1024);

		if (!bIsQuit)
			boost::asio::post(strand, boost::bind(&LocalWebUser::Parse, shared_from_this(), message));

		if (message.substr(0, 4) == "QUIT")
			bIsQuit = true;

		read();
	} else
		Close();
}
