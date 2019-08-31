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

#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <cctype>

#include "Server.h"
#include "mainframe.h"
#include "db.h"
#include "Config.h"
#include "services.h"
#include "oper.h"

std::map<std::string, unsigned int> mThrottle;
std::map <std::string, Server*> Servers;
extern Memos MemoMsg;

bool Server::CheckClone(const std::string &ip) {
	unsigned int i = 0;
	auto user = Mainframe::instance()->LocalUsers();
	auto it = user.begin();
	for (; it != user.end(); ++it) {
		if (it->second->mHost == ip)
			i++;
	}
	auto ruser = Mainframe::instance()->RemoteUsers();
	auto rit = ruser.begin();
	for (; rit != ruser.end(); ++rit) {
		if (rit->second->mHost == ip)
			i++;
	}
	unsigned int clones = OperServ::IsException(ip, "clon");
	if (clones != 0 && i <= clones)
		return false;
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

bool resolve (std::string ip) {
	struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    return (getaddrinfo(ip.c_str(), NULL, &hints, &res) == 0);
}

bool Server::CheckDNSBL(const std::string &ip) {
	std::string ipcliente = ip;
	Oper oper;
	int valor = OperServ::IsException(ipcliente, "dnsbl");
	if (valor > 0)
		return false;
	else if (ip.find(":") == std::string::npos) {
		for (unsigned int i = 0; config->Getvalue("dnsbl["+std::to_string(i)+"]suffix").length() > 0; i++) {
			if (config->Getvalue("dnsbl["+std::to_string(i)+"]reverse") == "true") {
				ipcliente = invertir(ip);
			} else {
				ipcliente = ip;
			}
			std::string hostname = ipcliente + config->Getvalue("dnsbl["+std::to_string(i)+"]suffix");
			if(resolve(hostname) == true)
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
			if(resolve(hostname) == true)
			{
				oper.GlobOPs("Alerta DNSBL6. " + config->Getvalue("dnsbl6["+std::to_string(i)+"]suffix") + " IP: " + ip);
				return true;
			}
		}
	}
	return false;
}

bool Server::HUBExiste()
{
	if (config->Getvalue("serverName") == config->Getvalue("hub"))
			return true;
	if (Servers.find(config->Getvalue("hub")) != Servers.end())
			return true;
	return false;
}

void Server::send(const std::string& message) {
	try {
		if (ssl == true && SSLSocket.lowest_layer().is_open()) {
			boost::asio::write(SSLSocket, boost::asio::buffer(message));
		} else if (ssl == false && Socket.is_open()) {
			boost::asio::write(Socket, boost::asio::buffer(message));
		}
	} catch (boost::system::system_error &e) {
		std::cout << "Error sending to server." << std::endl;
	}
} 

void Server::Send(std::string message)
{
	auto it = Servers.begin();
	for (; it != Servers.end(); ++it) {
		it->second->send(message + "\n");
	}
}

void Server::sendBurst (Server *server) {
	server->send("HUB " + config->Getvalue("hub") + "\n");
	server->send("SERVER " + config->Getvalue("serverName") + "\n");
	if (config->Getvalue("cluster") == "false") {
		std::string version = "VERSION ";
		if (DB::GetLastRecord() != "") {
			version.append(DB::GetLastRecord());
		} else {
			version.append("0");
		}
		server->send(version + "\n");
	}

	auto usermap = Mainframe::instance()->LocalUsers();
	auto it = usermap.begin();
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
		server->send("SNICK " + it->second->mNickName + " " + it->second->mIdent + " " + it->second->mHost + " " + it->second->mvHost + " " + std::to_string(it->second->bLogin) + " " + it->second->mServer + " " + modos + "\n");
		if (it->second->bAway == true)
			server->send("AWAY " + it->second->mNickName + " " + it->second->mAway + "\n");
	}
	auto channels = Mainframe::instance()->channels();
	auto it2 = channels.begin();
	for (; it2 != channels.end(); ++it2) {
		auto users = it2->second->mLocalUsers;
		auto it4 = users.begin();
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
			server->send("SJOIN " + (*it4)->mNickName + " " + it2->second->name() + " " + mode + "\n");
		}
		auto bans = it2->second->bans();
		auto it3 = bans.begin();
		for (; it3 != bans.end(); ++it3)
			server->send("SBAN " + it2->second->name() + " " + (*it3)->mask() + " " + (*it3)->whois() + " " + std::to_string((*it3)->time()) + "\n");
	}
	auto it6 = MemoMsg.begin();
	for (; it6 != MemoMsg.end(); ++it6)
		server->send("MEMO " + (*it6)->sender + " " + (*it6)->receptor + " " + std::to_string((*it6)->time) + " " + (*it6)->mensaje + "\n");
}

bool Server::IsAServer (const std::string &ip) {
	for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++)
		if (config->Getvalue("link["+std::to_string(i)+"]ip") == ip)
				return true;
	return false;
}

bool Server::IsConected (const std::string &ip) {
	auto it = Servers.begin();
    for(; it != Servers.end(); ++it)
		if (it->second->ip == ip)
			return true;
	return false;
}

void Server::Procesar() {
	boost::asio::streambuf buffer;
	boost::system::error_code error;
	std::string ipaddress;
	if (ssl == true) {
		SSLSocket.handshake(boost::asio::ssl::stream_base::server, error);		
		if (error) {
			Close();
			std::cout << "SSL ERROR: " << error << std::endl;
			return;
		}
		ipaddress = SSLSocket.lowest_layer().remote_endpoint().address().to_string();
	} else {
		ipaddress = Socket.remote_endpoint().address().to_string();
	}
	Oper oper;
	oper.GlobOPs(Utils::make_string("", "Connection with %s right. Syncronizing ...", ipaddress.c_str()));
	sendBurst(this);
	oper.GlobOPs(Utils::make_string("", "End of syncronization with %s", ipaddress.c_str()));

	do {
		if (ssl == false)
			boost::asio::read_until(Socket, buffer, '\n', error);
		else
			boost::asio::read_until(SSLSocket, buffer, '\n', error);
        
    	std::istream str(&buffer);
		std::string data; 
		std::getline(str, data);

		Parse(data);

	} while (Socket.is_open() || SSLSocket.lowest_layer().is_open());
	SQUIT(name);
}

void Server::Close() {
	if (ssl == true) {
		if (SSLSocket.lowest_layer().is_open()) {
			SSLSocket.lowest_layer().close();
		}
	} else {
		if (Socket.is_open()) {
			Socket.close();
		}
	}
}

void Server::SQUIT(std::string nombre)
{
	auto usermap = Mainframe::instance()->RemoteUsers();
	auto it = usermap.begin();
	for (; it != usermap.end(); ++it) {
		if (strcasecmp(it->second->mServer.c_str(), nombre.c_str()) == 0)
			it->second->QUIT();
	}
	Servers.erase(nombre);
	Oper oper;
	oper.GlobOPs(Utils::make_string("", "The server %s was splitted from network.", nombre.c_str()));
}

void Server::Connect(std::string ipaddr, std::string port) {
	bool ssl = false;
	int puerto;
	Oper oper;
	if (Server::IsAServer(ipaddr) == false) {
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
	boost::asio::io_context io;
	if (ssl == true) {
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		ctx.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::single_dh_use);
		ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
		ctx.use_certificate_chain_file("server.pem");
		ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
		ctx.use_tmp_dh_file("dh.pem");
		auto newserver = std::make_shared<Server>(io.get_executor(), ctx, "NoName", ipaddr, port);
		newserver->ssl = true;
		newserver->SSLSocket.lowest_layer().connect(Endpoint, error);
		if (error)
			oper.GlobOPs(Utils::make_string("", "Cannot connect to server: %s Port: %s", ipaddr.c_str(), port.c_str()));
		else {
			boost::system::error_code ec;
			newserver->SSLSocket.handshake(boost::asio::ssl::stream_base::client, ec);
			if (!ec) {
				std::thread t([newserver] { newserver->Procesar(); });
				t.detach();
			} else {
				newserver->Close();
			}
		}
	} else {
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		auto newserver = std::make_shared<Server>(io.get_executor(), ctx, "NoName", ipaddr, port);
		newserver->ssl = false;
		newserver->Socket.connect(Endpoint, error);
		if (error)
			oper.GlobOPs(Utils::make_string("", "Cannot connect to server: %s Port: %s", ipaddr.c_str(), port.c_str()));
		else {
			std::thread t([newserver] { newserver->Procesar(); });
			t.detach();
		}
	}
}

void Server::ConnectCloud()
{
	for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++) {
		std::string ip = config->Getvalue("link["+std::to_string(i)+"]ip");
		std::string port = config->Getvalue("link["+std::to_string(i)+"]port");
		if (!IsConected(ip)) {
			Connect(ip, port);
			sleep(1);
		}
	}
}

void Server::Ping()
{
	bPing = time(0);
}
