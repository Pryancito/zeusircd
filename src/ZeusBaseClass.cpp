/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ZeusBaseClass.h"
#include "api.h"
#include "Server.h"
#include "services.h"

#include <boost/range/algorithm/remove_if.hpp>
#include <boost/algorithm/string/classification.hpp>

extern bool exiting;
extern Server server;

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
			std::cout << "IOS websocket accept failure: " << e.what() << std::endl;
		}
	}
}

void PublicSock::API() {
	try
    {
        auto const address = boost::asio::ip::make_address("127.0.0.1");
        unsigned short port = 8000;

        boost::asio::io_context ioc{1};

        boost::asio::ip::tcp::acceptor acceptor{ioc, {address, port}};
        boost::asio::ip::tcp::socket socket{ioc};

		httpd::http_server(acceptor, socket);

        ioc.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error launching API: " << e.what() << std::endl;
    }
}

void PublicSock::ServerListen(std::string ip, std::string port, bool ssl)
{
	boost::asio::io_context ios;
	auto work = boost::make_shared<boost::asio::io_context::work>(ios);
	ClientServer server(1, ios, ip, (int) stoi(port));
	server.run();
	server.server(ip, port, ssl);
	for(;;) {
		try {
			ios.run();
			break;
		} catch (std::exception& e) {
			std::cout << "IOS plain accept failure: " << e.what() << std::endl;
		}
	}
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
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		ctx.set_options(
        boost::asio::ssl::context::default_workarounds
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
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		ctx.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::single_dh_use);
		ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
		ctx.use_certificate_chain_file("server.pem");
		ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
		ctx.use_tmp_dh_file("dh.pem");
		auto newclient = std::make_shared<LocalWebUser>(io_context_pool_.get_io_context().get_executor(), ctx);
		mAcceptor.async_accept(newclient->Socket.next_layer().next_layer(),
                           boost::bind(&ClientServer::handleWebAccept,   this,   newclient,  boost::asio::placeholders::error));
		newclient->deadline.expires_from_now(boost::posix_time::seconds(10));
		newclient->deadline.async_wait(boost::bind(&ClientServer::check_deadline_web, this, newclient, boost::asio::placeholders::error));
}

void ClientServer::server(std::string ip, std::string port, bool ssl)
{
	while(true)
	{
		Oper oper;
		if (ssl == true) {
			boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
			ctx.set_options(
			boost::asio::ssl::context::default_workarounds
			| boost::asio::ssl::context::single_dh_use);
			ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
			ctx.use_certificate_chain_file("server.pem");
			ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
			ctx.use_tmp_dh_file("dh.pem");
			auto newserver = std::make_shared<Server>(io_context_pool_.get_io_context().get_executor(), ctx, config->Getvalue("serverName"), ip, port);
			newserver->ssl = true;
			mAcceptor.accept(newserver->SSLSocket.lowest_layer());
			if (Server::IsAServer(newserver->SSLSocket.lowest_layer().remote_endpoint().address().to_string()) == false) {
				oper.GlobOPs(Utils::make_string("", "Connection attempt from: %s - Not found in config.", newserver->SSLSocket.lowest_layer().remote_endpoint().address().to_string().c_str()));
				newserver->Close();
			} else if (Server::IsConected(newserver->SSLSocket.lowest_layer().remote_endpoint().address().to_string()) == true) {
				oper.GlobOPs(Utils::make_string("", "The server %s exists, the connection attempt was ignored.", newserver->SSLSocket.lowest_layer().remote_endpoint().address().to_string().c_str()));
				newserver->Close();
			} else {
				std::thread t([newserver] { newserver->Procesar(); });
				t.detach();
			}
		} else {
			boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
			auto newserver = std::make_shared<Server>(io_context_pool_.get_io_context().get_executor(), ctx, config->Getvalue("serverName"), ip, port);
			newserver->ssl = false;
			mAcceptor.accept(newserver->Socket);
			if (Server::IsAServer(newserver->Socket.remote_endpoint().address().to_string()) == false) {
				oper.GlobOPs(Utils::make_string("", "Connection attempt from: %s - Not found in config.", newserver->Socket.remote_endpoint().address().to_string().c_str()));
				newserver->Close();
			} else if (Server::IsConected(newserver->Socket.remote_endpoint().address().to_string()) == true) {
				oper.GlobOPs(Utils::make_string("", "The server %s exists, the connection attempt was ignored.", newserver->Socket.remote_endpoint().address().to_string().c_str()));
				newserver->Close();
			} else {
				std::thread t([newserver] { newserver->Procesar(); });
				t.detach();
			}
		}
	}
}

void ClientServer::run()
{
	io_context_pool_.run();
}

void ClientServer::handleAccept(const std::shared_ptr<PlainUser> newclient, const boost::system::error_code& error) {
	newclient->deadline.cancel();
	plain();
	if (!error) {
		if (stoi(config->Getvalue("maxUsers")) <= Mainframe::instance()->countusers()) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections."));
			newclient->Close();
		} else if (Server::CheckClone(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones."));
			newclient->Close();
		} else if (Server::CheckDNSBL(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists."));
			newclient->Close();
		} else if (Server::CheckThrottle(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again."));
			newclient->Close();
		} else if (OperServ::IsGlined(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()));
			newclient->Close();
		} else if (OperServ::IsTGlined(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are Timed G-Lined. Reason: %s", OperServ::ReasonTGlined(newclient->ip()).c_str()));
			newclient->Close();
		} else if (OperServ::CanGeoIP(newclient->ip()) == false) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You can not connect from your country."));
			newclient->Close();
		} else {
			Server::ThrottleUP(newclient->ip());
			newclient->start();
		}
	}
}

void ClientServer::handleSSLAccept(const std::shared_ptr<LocalSSLUser> newclient, const boost::system::error_code& error) {
	ssl();
	if (!error) {
		newclient->Socket.async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&ClientServer::handle_handshake_ssl,   this,   newclient,  boost::asio::placeholders::error));
	}
}

void ClientServer::handleWebAccept(const std::shared_ptr<LocalWebUser> newclient, const boost::system::error_code& error) {
	wss();
	if (!error) {
		newclient->Socket.next_layer().async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&ClientServer::handle_handshake_web,   this,   newclient,  boost::asio::placeholders::error));
	}
}

void ClientServer::handle_handshake_ssl(const std::shared_ptr<LocalSSLUser>& newclient, const boost::system::error_code& error) {
	newclient->deadline.cancel();
	if (!error) {
		if (stoi(config->Getvalue("maxUsers")) <= Mainframe::instance()->countusers()) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections."));
			newclient->Close();
		} else if (Server::CheckClone(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones."));
			newclient->Close();
		} else if (Server::CheckDNSBL(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists."));
			newclient->Close();
		} else if (Server::CheckThrottle(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again."));
			newclient->Close();
		} else if (OperServ::IsGlined(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()));
			newclient->Close();
		} else if (OperServ::IsTGlined(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are Timed G-Lined. Reason: %s", OperServ::ReasonTGlined(newclient->ip()).c_str()));
			newclient->Close();
		} else if (OperServ::CanGeoIP(newclient->ip()) == false) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You can not connect from your country."));
			newclient->Close();
		} else {
			Server::ThrottleUP(newclient->ip());
			newclient->start();
		}
	}
}

void ClientServer::handle_handshake_web(const std::shared_ptr<LocalWebUser>& newclient, const boost::system::error_code& error) {
	newclient->deadline.cancel();
	if (!error) {
		if (stoi(config->Getvalue("maxUsers")) <= Mainframe::instance()->countusers()) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections."));
			newclient->Close();
		} else if (Server::CheckClone(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones."));
			newclient->Close();
		} else if (Server::CheckDNSBL(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists."));
			newclient->Close();
		} else if (Server::CheckThrottle(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again."));
			newclient->Close();
		} else if (OperServ::IsGlined(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()));
			newclient->Close();
		} else if (OperServ::IsTGlined(newclient->ip()) == true) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are Timed G-Lined. Reason: %s", OperServ::ReasonTGlined(newclient->ip()).c_str()));
			newclient->Close();
		} else if (OperServ::CanGeoIP(newclient->ip()) == false) {
			newclient->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You can not connect from your country."));
			newclient->Close();
		} else {
			Server::ThrottleUP(newclient->ip());
			newclient->start();
		}
	}
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
