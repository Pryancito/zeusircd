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
#include "amqp.h"

std::map<std::string, unsigned int> mThrottle;
std::set <Server*> Servers;
extern Memos MemoMsg;

bool Server::CheckClone(const std::string &ip) {
	unsigned int i = 0;
	auto user = Mainframe::instance()->LocalUsers();
	auto it = user.begin();
	for (; it != user.end(); ++it) {
		if (it->second)
			if (it->second->mHost == ip)
				i++;
	}
	auto ruser = Mainframe::instance()->RemoteUsers();
	auto rit = ruser.begin();
	for (; rit != ruser.end(); ++rit) {
		if (it->second)
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
	if (config->Getvalue("cluster") == "true")
		return true;
	else if (config->Getvalue("serverName") == config->Getvalue("hub"))
		return true;
	for (Server *server : Servers) {
		if (server->name == config->Getvalue("hub"))
			return true;
	}
	return false;
}

void Server::Send(std::string message)
{
	for (Server *srv : Servers) {
		if (Server::IsConected(srv->ip) == true)
			srv->send(message);
	}
}

void Server::send(std::string message)
{
	try {
		std::string url("//" + ip + ":" + port);
		client c(url, ip, message);
		proton::default_container(c).run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

void Server::sendBurst (Server *server) {
	server->send("HUB " + config->Getvalue("hub"));
	server->send("SERVER " + config->Getvalue("serverName"));
	if (config->Getvalue("cluster") == "false") {
		std::string version = "VERSION ";
		if (DB::GetLastRecord() != "") {
			version.append(DB::GetLastRecord());
		} else {
			version.append("0");
		}
		server->send(version);
	}

	for (const auto& user : Mainframe::instance()->LocalUsers()) {
		std::string modos = "+";
		if (user.second == nullptr)
			continue;
		if (user.second->getMode('r') == true)
			modos.append("r");
		if (user.second->getMode('z') == true)
			modos.append("z");
		if (user.second->getMode('w') == true)
			modos.append("w");
		if (user.second->getMode('o') == true)
			modos.append("o");
		if (modos == "+")
			modos.append("x");
		server->send("SNICK " + user.second->mNickName + " " + user.second->mIdent + " " + user.second->mHost + " " + user.second->mCloak + " " + user.second->mvHost + " " + std::to_string(user.second->bLogin) + " " + user.second->mServer + " " + modos);
		if (user.second->bAway == true)
			server->send("AWAY " + user.second->mNickName + " " + user.second->mAway);
	}

	for (const auto& chan : Mainframe::instance()->channels()) {
		for (const auto& users : chan.second->mLocalUsers) {
			std::string mode = "+x";
			if (chan.second->isOperator(users) == true)
				mode = "+o";
			else if (chan.second->isHalfOperator(users) == true)
				mode = "+h";
			else if (chan.second->isVoice(users) == true)
				mode = "+v";
			server->send("SJOIN " + users->mNickName + " " + chan.second->name() + " " + mode);
		}
		for (const auto& bans : chan.second->bans())
			server->send("SBAN " + chan.second->name() + " " + bans->mask() + " " + bans->whois() + " " + std::to_string(bans->time()));
		for (const auto& pbans : chan.second->pbans())
			server->send("SPBAN " + chan.second->name() + " " + pbans->mask() + " " + pbans->whois() + " " + std::to_string(pbans->time()));
	}
	for (const auto& memos : MemoMsg)
		server->send("MEMO " + memos->sender + " " + memos->receptor + " " + std::to_string(memos->time) + " " + memos->mensaje);
}

bool Server::IsAServer (const std::string &ip) {
	for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++)
		if (config->Getvalue("link["+std::to_string(i)+"]ip") == ip)
				return true;
	return false;
}

bool Server::IsConected (const std::string &ip) {
	for (Server *server : Servers) {
		if (server->ip == ip && !server->name.empty())
			return true;
	}
	return false;
}

bool Server::Exists (const std::string name) {
	for (Server *server : Servers) {
		if (server->name == name)
			return true;
	}
	return false;
}

size_t Server::count()
{
	return Servers.size() + 1;
}

void Server::SQUIT(std::string nombre, bool del, bool send)
{
	auto usermap = Mainframe::instance()->RemoteUsers();
	auto it = usermap.begin();
	for (; it != usermap.end(); ++it) {
		if (it->second)
			if (it->second->mServer == nombre)
				it->second->QUIT();
	}
	if (del == true)
		for (Server *server : Servers) {
			if (server->name == nombre) {
				server->name = "";
				server = nullptr;
				Servers.erase(server);
				break;
			}
		}
	if (send == true)
		Server::Send("SQUIT " + nombre);
	Oper oper;
	oper.GlobOPs(Utils::make_string("", "The server %s was splitted from network.", nombre.c_str()));
}

void Server::Ping()
{
	bPing = time(0);
}

void Server::CheckDead(const boost::system::error_code &e)
{
	if (!e)
	{
		if (bPing + 120 < time(0))
		{
			SQUIT(name, true, false);
		} else {
			this->send("PING");
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(30)); 
			deadline.async_wait(boost::bind(&Server::CheckDead, this, boost::asio::placeholders::error));
		}
	}
}

void Server::ConnectCloud() {
	for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++) {
		if (Server::IsConected(config->Getvalue("link["+std::to_string(i)+"]ip")) == false) {
			Server *srv = new Server(config->Getvalue("link["+std::to_string(i)+"]ip"), config->Getvalue("link["+std::to_string(i)+"]port"));
			Servers.insert(srv);
			srv->send("BURST");
		}
	}
}
