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
#include "Config.h"
#include "services.h"
#include "oper.h"
#include "amqp.h"
#include "db.h"

std::map<std::string, unsigned int> mThrottle;
std::set <Server*> Servers;
extern Memos MemoMsg;
std::mutex mutex_srv;

bool Server::CheckClone(const std::string &ip) {
	if (ip == "127.0.0.1")
		return false;
	unsigned int i = 0;
	auto it = Users.begin();
	for (; it != Users.end(); ++it) {
		if (it->second)
			if (it->second->mHost == ip)
				i++;
	}
	unsigned int clones = OperServ::IsException(ip, "clon");
	if (clones != 0 && i <= clones)
		return false;
	return (i >= config["clones"].as<unsigned int>());
}

bool Server::CheckThrottle(const std::string &ip) {
	if (ip == "127.0.0.1")
		return false;
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
		for (unsigned int i = 0; i < config["dnsbl"].size(); i++) {
			if (config["dnsbl"][i]["reverse"].as<bool>() == true) {
				ipcliente = invertir(ip);
			} else {
				ipcliente = ip;
			}
			std::string hostname = ipcliente + config["dnsbl"][i]["suffix"].as<std::string>();
			if (resolve(hostname) == true)
			{
				oper.GlobOPs("Alerta DNSBL. " + config["dnsbl"][i]["suffix"].as<std::string>() + " IP: " + ip);
				return true;
			}
		}
	} else {
		for (unsigned int i = 0; i < config["dnsbl6"].size(); i++) {
			if (config["dnsbl6"][i]["reverse"].as<bool>() == true) {
				ipcliente = invertirv6(ip);
			} else {
				ipcliente = ip;
			}
			std::string hostname = ipcliente + config["dnsbl6"][i]["suffix"].as<std::string>();
			if (resolve(hostname) == true)
			{
				oper.GlobOPs("Alerta DNSBL6. " + config["dnsbl6"][i]["suffix"].as<std::string>() + " IP: " + ip);
				return true;
			}
		}
	}
	return false;
}

bool Server::HUBExiste()
{
	if (config["database"]["cluster"].as<bool>() == true)
		return true;
	else if (config["hub"].as<std::string>() == config["serverName"].as<std::string>())
		return true;
	for (Server *server : Servers) {
		if (server->name == config["hub"].as<std::string>())
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
	server->send("HUB " + config["hub"].as<std::string>());
	server->send("SERVER " + config["serverName"].as<std::string>());
	if (config["database"]["cluster"].as<bool>() == false) {
		server->send("VERSION " + DB::GetLastRecord());
	}

	auto it = Users.begin();
	for (; it != Users.end(); ++it) {
		std::string modos = "+";
		if (!it->second || it->second == 0 || it->second == nullptr)
			continue;
		if (it->second->getMode('r') == true)
			modos.append("r");
		if (it->second->getMode('z') == true)
			modos.append("z");
		if (it->second->getMode('w') == true)
			modos.append("w");
		if (it->second->getMode('o') == true)
			modos.append("o");
		if (modos == "+")
			modos.append("x");
		server->send("SNICK " + it->second->mNickName + " " + it->second->mIdent + " " + it->second->mHost + " " + it->second->mCloak + " " + it->second->mvHost + " " + std::to_string(it->second->bLogin) + " " + it->second->mServer + " " + modos);
		if (it->second->bAway == true)
			server->send("AWAY " + it->second->mNickName + " " + it->second->mAway);
	}
	auto it2 = Channels.begin();
	for (; it2 != Channels.end(); ++it2) {
		auto users = it2->second->users;
		auto it4 = users.begin();
		for (; it4 != users.end(); ++it4) {
			std::string mode = "+x";
			if (it2->second->IsOperator(*it4) == true)
				mode = "+o";
			else if (it2->second->IsHalfOperator(*it4) == true)
				mode = "+h";
			else if (it2->second->IsVoice(*it4) == true)
				mode = "+v";
			server->send("SJOIN " + (*it4)->mNickName + " " + it2->second->name + " " + mode);
		}
		auto bans = it2->second->bans;
		auto it3 = bans.begin();
		for (; it3 != bans.end(); ++it3)
			server->send("SBAN " + it2->second->name + " " + (*it3)->mask() + " " + (*it3)->whois() + " " + std::to_string((*it3)->time()));
		auto pbans = it2->second->pbans;
		auto it7 = pbans.begin();
		for (; it7 != pbans.end(); ++it7)
			server->send("SPBAN " + it2->second->name + " " + (*it7)->mask() + " " + (*it7)->whois() + " " + std::to_string((*it7)->time()));
	}
	auto it6 = MemoMsg.begin();
	for (; it6 != MemoMsg.end(); ++it6)
		server->send("MEMO " + (*it6)->sender + " " + (*it6)->receptor + " " + std::to_string((*it6)->time) + " " + (*it6)->mensaje);
}

bool Server::IsAServer (const std::string &ip) {
	for (unsigned int i = 0; i < config["links"].size(); i++)
		if (config["links"][i]["ip"].as<std::string>() == ip)
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
	size_t i = 0;
	for (Server *server : Servers) {
		if (Server::IsConected(server->ip) == true)
			i++;
	}
	return i + 1;
}

void Server::SQUIT(std::string nombre, bool del, bool send)
{
	auto it = Users.begin();
	for (; it != Users.end(); ++it) {
		if (it->second)
			if (it->second->mServer == nombre)
				it->second->QUIT();
	}
	if (del)
		for (Server *server : Servers) {
			if (server->name == nombre) {
				server->name = "";
				server = nullptr;
				Servers.erase(server);
				break;
			}
		}
	if (send)
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
	for (Server *server : Servers) {
		if (Server::IsConected(server->ip) == false) {
			server->send("BURST");
			Server::sendBurst(server);
		}
	}
}
