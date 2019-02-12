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
#include "server.h"
#include "mainframe.h"
#include "oper.h"
#include "session.h"
#include "db.h"
#include "services.h"
#include "utils.h"

#include <boost/thread.hpp>
#include <boost/system/error_code.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/algorithm/string/classification.hpp>

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

CloneMap mThrottle;
ServerSet Servers;
extern Memos MemoMsg;
boost::mutex server_mtx;
extern boost::asio::io_context channel_user_context;

Server::Server(boost::asio::io_context& io_context, const std::string &s_ip, int s_port, bool s_ssl, bool s_ipv6)
:   mAcceptor(io_context, tcp::endpoint(boost::asio::ip::address::from_string(s_ip), s_port)), ip(s_ip), port(s_port), ssl(s_ssl), ipv6(s_ipv6), deadline(channel_user_context)
{
    mAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    mAcceptor.listen(boost::asio::socket_base::max_listen_connections);
}

void Server::start() { 
	startAccept();
}

void Server::startAccept() {
	boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
	if (ssl == true) {
		ctx.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2);
		ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
		ctx.use_certificate_chain_file("server.pem");
		ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
		ctx.use_tmp_dh_file("dh.pem");
		std::shared_ptr<Session> newclient(new (GC) Session(mAcceptor.get_executor().context(), ctx));
		newclient->ssl = true;
		newclient->websocket = false;
		mAcceptor.async_accept(newclient->socket_ssl().lowest_layer(),
                           boost::bind(&Server::handleAccept,   this,   newclient,  boost::asio::placeholders::error));
	} else {
		std::shared_ptr<Session> newclient(new (GC) Session(mAcceptor.get_executor().context(), ctx));
		newclient->ssl = false;
		newclient->websocket = false;
		mAcceptor.async_accept(newclient->socket(),
                           boost::bind(&Server::handleAccept,   this,   newclient,  boost::asio::placeholders::error));
	}
}


void Server::handle_handshake(const std::shared_ptr<Session>& newclient, const boost::system::error_code& error) {
	deadline.cancel();
	if (error) {
		newclient->socket_ssl().lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	} else if (stoi(config->Getvalue("maxUsers")) <= Mainframe::instance()->countusers() && ssl == false) {
		newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections.") + config->EOFMessage);
		newclient->close();
	} else if (CheckClone(newclient->ip()) == true) {
		newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones.") + config->EOFMessage);
		newclient->close();
	} else if (CheckDNSBL(newclient->ip()) == true) {
		newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists.") + config->EOFMessage);
		newclient->close();
	} else if (CheckThrottle(newclient->ip()) == true) {
		newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again.") + config->EOFMessage);
		newclient->close();
	} else if (OperServ::IsGlined(newclient->ip()) == true) {
		newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()) + config->EOFMessage);
		newclient->close();
	} else {
		ThrottleUP(newclient->ip());
		newclient->start();
	}
}

void Server::check_deadline(const std::shared_ptr<Session>& newclient, const boost::system::error_code &e)
{
	if (!e)
		newclient->socket_ssl().lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
}

void Server::handleAccept(const std::shared_ptr<Session>& newclient, const boost::system::error_code& error) {
	startAccept();
	if (ssl == false) {
		if (error) {
			newclient->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		} else if (stoi(config->Getvalue("maxUsers")) <= Mainframe::instance()->countusers() && ssl == false) {
			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections.") + config->EOFMessage);
			newclient->close();
		} else if (CheckClone(newclient->ip()) == true) {
			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones.") + config->EOFMessage);
			newclient->close();
		} else if (CheckDNSBL(newclient->ip()) == true) {
			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists.") + config->EOFMessage);
			newclient->close();
		} else if (CheckThrottle(newclient->ip()) == true) {
			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again.") + config->EOFMessage);
			newclient->close();
		} else if (OperServ::IsGlined(newclient->ip()) == true) {
			newclient->sendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(newclient->ip()).c_str()) + config->EOFMessage);
			newclient->close();
		} else {
			deadline.cancel();
			ThrottleUP(newclient->ip());
			newclient->start();
		}
    } else {
		if (error)
			newclient->socket_ssl().lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		else {
			deadline.expires_from_now(boost::posix_time::seconds(10));
			deadline.async_wait(boost::bind(&Server::check_deadline, this, newclient, boost::asio::placeholders::error));
			newclient->socket_ssl().async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&Server::handle_handshake,   this,   newclient,  boost::asio::placeholders::error));
		}
	}
}

bool Server::CheckClone(const std::string &ip) {
	unsigned int i = 0;
	UserMap user = Mainframe::instance()->users();
	UserMap::iterator it = user.begin();
	for (; it != user.end(); ++it) {
		if (it->second)
			if (it->second->host() == ip)
				i++;
	}
	return (i >= (unsigned int )stoi(config->Getvalue("clones")));
}

bool Server::CheckThrottle(const std::string &ip) {
	if (mThrottle.count(ip)) 
		return (mThrottle[ip] >= 3);
	else
		return false;
}

void Server::ThrottleUP(const std::string &ip) {
    if (mThrottle.count(ip) > 0)
		mThrottle[ip] += 1;
	else
		mThrottle[ip] = 1;
}

std::string invertir(std::string rstr)
{
    std::reverse(rstr.begin(), rstr.end());
    int size = rstr.size();
    int start = 0;
    int end = 0;
    while (end != size + 1) {
        if (rstr[end] == '.' || end == size) {
            std::reverse(rstr.begin() + start, rstr.begin() + end);
            start = end + 1;
        }
        ++end;
    }
    return rstr;
}

std::string BinToHex(const void* raw, size_t l)
{
	static const char hextable[] = "0123456789abcdef";
	const char* data = static_cast<const char*>(raw);
	std::string rv;
	rv.reserve(l * 2);
	for (size_t i = 0; i < l; i++)
	{
		unsigned char c = data[i];
		rv.push_back(hextable[c >> 4]);
		rv.push_back(hextable[c & 0xF]);
	}
	return rv;
}

std::string invertirv6 (std::string str) {
	struct in6_addr addr;
    inet_pton(AF_INET6,str.c_str(),&addr);
	const unsigned char* ip = addr.s6_addr;
	std::string reversedip;

	std::string buf = BinToHex(ip, 16);
	for (std::string::const_reverse_iterator it = buf.rbegin(); it != buf.rend(); ++it)
	{
		reversedip.push_back(*it);
		reversedip.push_back('.');
	}
	return reversedip;
}

bool Server::CheckDNSBL(const std::string &ip) {
	std::string ipcliente;
	Oper oper;
	if (ip.find(":") == std::string::npos) {
		for (unsigned int i = 0; config->Getvalue("dnsbl["+std::to_string(i)+"]suffix").length() > 0; i++) {
			if (config->Getvalue("dnsbl["+std::to_string(i)+"]reverse") == "true") {
				ipcliente = invertir(ip);
			} else {
				ipcliente = ip;
			}
			std::string hostname = ipcliente + config->Getvalue("dnsbl["+std::to_string(i)+"]suffix");
			hostent *record = gethostbyname(hostname.c_str());
			if(record != NULL)
			{
				oper.GlobOPs("Alerta DNSBL. " + config->Getvalue("dnsbl["+std::to_string(i)+"]suffix") + " IP: " + ip);
				return true;
			}
		}
	} else {
		for (unsigned int i = 0; config->Getvalue("dnsbl6["+std::to_string(i)+"]suffix").length() > 0; i++) {
			if (config->Getvalue("dnsbl6["+std::to_string(i)+"]reverse") == "true") {
				ipcliente = invertirv6(ip);
			} else {
				ipcliente = ip;
			}
			std::string hostname = ipcliente + config->Getvalue("dnsbl6["+std::to_string(i)+"]suffix");
			hostent *record = gethostbyname(hostname.c_str());
			if(record != NULL)
			{
				oper.GlobOPs("Alerta DNSBL6. " + config->Getvalue("dnsbl6["+std::to_string(i)+"]suffix") + " IP: " + ip);
				return true;
			}
		}
	}
	return false;
}

bool Server::HUBExiste() {
	if (config->Getvalue("serverName") == config->Getvalue("hub"))
		return true;
	ServerSet::iterator it = Servers.begin();
    for (; it != Servers.end(); ++it)
		if ((*it)->name() == config->Getvalue("hub"))
			return true;
	return false;
}

void Servidor::SQUIT(std::string nombre) {
	StrVec servers;
	server_mtx.lock();
	ServerSet::iterator it = Servers.begin();
	for (; it != Servers.end(); ++it) {
		if (boost::iequals((*it)->name(), nombre)) {
			servers.push_back((*it)->name());
			for (unsigned int i = 0; i < (*it)->connected.size(); i++)
				servers.push_back((*it)->connected[i]);
		}
	}

	for (unsigned int i = 0; i < servers.size(); i++) {
		UserMap usermap = Mainframe::instance()->users();
		UserMap::iterator it3 = usermap.begin();
		for (; it3 != usermap.end(); ++it3) {
			if (!it3->second)
				continue;
			else if (boost::iequals(it3->second->server(), servers[i]))
				it3->second->NETSPLIT();
		}
		ServerSet::iterator it2 = Servers.begin();
		while(it2 != Servers.end()) {
			std::vector<std::string>::iterator result = find((*it2)->connected.begin(), (*it2)->connected.end(), servers[i]);
			if (result != (*it2)->connected.end()) {
				(*it2)->connected.erase(result);
				it2++;
				continue;
			} if (boost::iequals((*it2)->name(), servers[i])) {
				it2 = Servers.erase(it2);
				continue;
			}
			it2++;
		}
	}
	server_mtx.unlock();
	Oper oper;
	oper.GlobOPs(Utils::make_string("", "The server %s was splitted from network.", nombre.c_str()));
}

void Servidor::Connect(std::string ipaddr, std::string port) {
	bool ssl = false;
	int puerto;
	Oper oper;
	if (Servidor::IsAServer(ipaddr) == false) {
		oper.GlobOPs(Utils::make_string("", "The server %s is not present into config file.", ipaddr.c_str()));
		return;
	}
	if (port[0] == '+') {
		puerto = (int ) stoi(port.substr(1));
		ssl = true;
	} else
		puerto = (int ) stoi(port);
		
	boost::system::error_code error;
	boost::asio::ip::tcp::endpoint Endpoint(
	boost::asio::ip::address::from_string(ipaddr), puerto);
	boost::asio::io_context io_context;
	boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
	if (ssl == true) {
		ctx.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::single_dh_use);
		ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
		ctx.use_certificate_chain_file("server.pem");
		ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
		ctx.use_tmp_dh_file("dh.pem");
		Servidor *newserver = new (GC) Servidor(io_context, ctx);
		newserver->ssl = true;
		newserver->socket_ssl().lowest_layer().connect(Endpoint, error);
		if (error)
			oper.GlobOPs(Utils::make_string("", "Cannot connect to server: %s Port: %s", ipaddr.c_str(), port.c_str()));
		else {
			boost::system::error_code ec;
			newserver->socket_ssl().handshake(boost::asio::ssl::stream_base::client, ec);
			if (!ec) {
				std::thread t([newserver] { newserver->Procesar(); });
				t.detach();
			} else {
				newserver->close();
			}
		}
	} else {
		Servidor *newserver = new (GC) Servidor(io_context, ctx);
		newserver->ssl = false;
		newserver->socket().connect(Endpoint, error);
		if (error)
			oper.GlobOPs(Utils::make_string("", "Cannot connect to server: %s Port: %s", ipaddr.c_str(), port.c_str()));
		else {
			std::thread t([newserver] { newserver->Procesar(); });
			t.detach();
		}
	}
}

void Server::servidor() {
	while (true) {
		Oper oper;
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		if (ssl == true) {
			ctx.set_options(
			boost::asio::ssl::context::default_workarounds
			| boost::asio::ssl::context::no_sslv2
			| boost::asio::ssl::context::single_dh_use);
			ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
			ctx.use_certificate_chain_file("server.pem");
			ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
			ctx.use_tmp_dh_file("dh.pem");
			Servidor *newserver = new (GC) Servidor(mAcceptor.get_executor().context(), ctx);
			newserver->ssl = true;
			mAcceptor.accept(newserver->socket_ssl().lowest_layer());
			if (Servidor::IsAServer(newserver->socket_ssl().lowest_layer().remote_endpoint().address().to_string()) == false) {
				oper.GlobOPs(Utils::make_string("", "Connection attempt from: %s - Not found in config.", newserver->socket_ssl().lowest_layer().remote_endpoint().address().to_string().c_str()));
				newserver->close();
			} else if (Servidor::IsConected(newserver->socket_ssl().lowest_layer().remote_endpoint().address().to_string()) == true) {
				oper.GlobOPs(Utils::make_string("", "The server %s exists, the connection attempt was ignored.", newserver->socket_ssl().lowest_layer().remote_endpoint().address().to_string().c_str()));
				newserver->close();
			} else {
				std::thread t([newserver] { newserver->Procesar(); });
				t.detach();
			}
		} else {
			Servidor *newserver = new (GC) Servidor(mAcceptor.get_executor().context(), ctx);
			newserver->ssl = false;
			mAcceptor.accept(newserver->socket());
			if (Servidor::IsAServer(newserver->socket().remote_endpoint().address().to_string()) == false) {
				oper.GlobOPs(Utils::make_string("", "Connection attempt from: %s - Not found in config.", newserver->socket().remote_endpoint().address().to_string().c_str()));
				newserver->close();
			} else if (Servidor::IsConected(newserver->socket().remote_endpoint().address().to_string()) == true) {
				oper.GlobOPs(Utils::make_string("", "The server %s exists, the connection attempt was ignored.", newserver->socket().remote_endpoint().address().to_string().c_str()));
				newserver->close();
			} else {
				std::thread t([newserver] { newserver->Procesar(); });
				t.detach();
			}
		}
	}
}

void Servidor::Procesar() {
    GC_stack_base sb;
    GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);
	boost::asio::streambuf buffer;
	boost::system::error_code error;
	if (this->ssl == true) {
		boost::system::error_code ec;
		this->socket_ssl().handshake(boost::asio::ssl::stream_base::server, ec);		
		if (ec) {
			this->close();
			std::cout << "SSL ERROR: " << ec << std::endl;
			return;
		}
		ipaddress = this->socket_ssl().lowest_layer().remote_endpoint().address().to_string();
	} else {
		ipaddress = this->socket().remote_endpoint().address().to_string();
	}
	Oper oper;
	oper.GlobOPs(Utils::make_string("", "Connection with %s right. Syncronizing ...", ipaddress.c_str()));
	SendBurst(this);
	oper.GlobOPs(Utils::make_string("", "End of syncronization with %s", ipaddress.c_str()));

	do {
		if (this->ssl == false)
			boost::asio::read_until(this->socket(), buffer, '\n', error);
		else
			boost::asio::read_until(this->socket_ssl(), buffer, '\n', error);
        
    	std::istream str(&buffer);
		std::string data; 
		std::getline(str, data);

        data.erase(boost::remove_if(data, boost::is_any_of("\r\n\t")), data.end());

		if (data.length() > 2048)
			data.substr(0, 2048);

		Message(this, data);

	} while (this->socket().is_open() || this->socket_ssl().lowest_layer().is_open());
	sendallbutone(this, "SQUIT " + this->name());
	SQUIT(this->name());
	GC_unregister_my_thread();
}

boost::asio::ip::tcp::socket& Servidor::socket() { return mSocket; }

boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& Servidor::socket_ssl() { return mSSL; }

void Servidor::close() {
	if (ssl == true && mSSL.lowest_layer().is_open()) {
		mSSL.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	} else if (ssl == false && mSocket.is_open()) {
		mSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	}
}

std::string Servidor::name() {
	return nombre;
}

std::string Servidor::ip() {
	return ipaddress;
}

std::string Servidores::name() {
	return nombre;
}

std::string Servidores::ip() {
	return ipaddress;
}

Servidor *Servidores::link() {
	return server;
}

void Servidores::UpdatePing() {
	sPing = time(0);
}

time_t Servidores::GetPing() {
	return sPing;
}

void Servidores::uPing(const std::string &servidor) {
	ServerSet::iterator it = Servers.begin();
    for(; it != Servers.end(); ++it)
		if ((*it)->name() == servidor)
			(*it)->UpdatePing();
}

int Servidor::count() {
	if (Servers.size() == 0)
		return 1;
	else
		return Servers.size();
}

bool Servidor::isQuit() {
	return quit;
}

bool Servidor::IsAServer (const std::string &ip) {
	for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++)
		if (config->Getvalue("link["+std::to_string(i)+"]ip") == ip)
				return true;
	return false;
}

bool Servidor::IsConected (const std::string &ip) {
	ServerSet::iterator it = Servers.begin();
    for(; it != Servers.end(); ++it)
		if ((*it)->ip() == ip)
			return true;
	return false;
}

bool Servidor::Exists (std::string name) {
	ServerSet::iterator it = Servers.begin();
    for(; it != Servers.end(); ++it)
		if (boost::iequals((*it)->name(), name))
			return true;
	return false;
}

void Servidor::send(const std::string& message) {
	boost::system::error_code ignored_error;
	if (ssl == true && mSSL.lowest_layer().is_open())
		boost::asio::write(mSSL, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
	else if (ssl == false && mSocket.is_open())
		boost::asio::write(mSocket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
}	

void Servidor::sendall(const std::string& message) {
	server_mtx.lock();
	ServerSet::iterator it = Servers.begin();
    for (; it != Servers.end(); ++it) {
		if ((*it)->link() != nullptr && (*it)->name() != config->Getvalue("serverName"))
			(*it)->link()->send(message + config->EOFServer);
	}
	server_mtx.unlock();
}

void Servidor::sendallbutone(Servidor *server, const std::string& message) {
	server_mtx.lock();
	ServerSet::iterator it = Servers.begin();
    for (; it != Servers.end(); ++it) {
		if ((*it)->link() != nullptr && (*it)->link() != server && (*it)->name() != config->Getvalue("serverName"))
			(*it)->link()->send(message + config->EOFServer);
	}
	server_mtx.unlock();
}

Servidores::Servidores(Servidor *servidor, const std::string &name, const std::string &ip) : server(servidor), nombre(name), ipaddress(ip), sPing(0) { sPing = time(0); }

void Servidor::addServer(Servidor *servidor, std::string name, std::string ip, const std::vector <std::string> &conexiones) {
	Servidores *server = new (GC) Servidores(servidor, name, ip);
	server->connected = conexiones;
	Servers.insert(server);
}

void Servidor::setname(const std::string &name) {
	nombre = name;
}

void Servidor::addLink(const std::string &hub, std::string link) {
	ServerSet::iterator it = Servers.begin();
    for(; it != Servers.end(); ++it)
		if ((*it)->name() == hub && std::find((*it)->connected.begin(), (*it)->connected.end(), link) == (*it)->connected.end())
			(*it)->connected.push_back(link);
}

void Servidor::SendBurst (Servidor *server) {
	server->send("HUB " + config->Getvalue("hub") + config->EOFServer);
	server_mtx.lock();
	std::string version = "VERSION ";
	if (DB::GetLastRecord() != "") {
		version.append(DB::GetLastRecord() + config->EOFServer);
	} else {
		version.append("0" + config->EOFServer);
	}
	server->send(version);

	ServerSet::iterator it5 = Servers.begin();
    for(; it5 != Servers.end(); ++it5) {
		std::string servidor = "SERVER " + (*it5)->name() + " " + (*it5)->ip();
		for (unsigned int i = 0; i < (*it5)->connected.size(); i++)
			servidor.append(" " + (*it5)->connected[i]);
		servidor.append(config->EOFServer);
		server->send(servidor);
	}

	UserMap usermap = Mainframe::instance()->users();
	UserMap::iterator it = usermap.begin();
	for (; it != usermap.end(); ++it) {
		std::string modos = "+";
		if (it->second == nullptr)
			continue;
		if (it->second->getMode('r') == true)
			modos.append("r");
		if (it->second->getMode('z') == true)
			modos.append("z");
		if (it->second->getMode('w') == true)
			modos.append("w");
		if (it->second->getMode('o') == true)
			modos.append("o");
		server->send("SNICK " + it->second->nick() + " " + it->second->ident() + " " + it->second->host() + " " + it->second->cloak() + " " + std::to_string(it->second->GetLogin()) + " " + it->second->server() + " " + modos + config->EOFServer);
		if (it->second->is_away() == true)
			server->send("AWAY " + it->second->nick() + " " + it->second->away_reason() + config->EOFServer);
	}
	ChannelMap channels = Mainframe::instance()->channels();
	ChannelMap::iterator it2 = channels.begin();
	for (; it2 != channels.end(); ++it2) {
		UserSet users = it2->second->users();
		UserSet::iterator it4 = users.begin();
		for (; it4 != users.end(); ++it4) {
			std::string mode;
			if (it2->second->isOperator(*it4) == true)
				mode.append("+o");
			else if (it2->second->isHalfOperator(*it4) == true)
				mode.append("+h");
			else if (it2->second->isVoice(*it4) == true)
				mode.append("+v");
			else
				mode.append("+x");
			server->send("SJOIN " + (*it4)->nick() + " " + it2->second->name() + " " + mode + config->EOFServer);
		}
		BanSet bans = it2->second->bans();
		BanSet::iterator it3 = bans.begin();
		for (; it3 != bans.end(); ++it3)
			server->send("SBAN " + it2->second->name() + " " + (*it3)->mask() + " " + (*it3)->whois() + " " + std::to_string((*it3)->time()) + config->EOFServer);
	}
	Memos::iterator it6 = MemoMsg.begin();
	for (; it6 != MemoMsg.end(); ++it6)
		server->send("MEMO " + (*it6)->sender + " " + (*it6)->receptor + " " + std::to_string((*it6)->time) + " " + (*it6)->mensaje + config->EOFServer);
	server_mtx.unlock();
}
