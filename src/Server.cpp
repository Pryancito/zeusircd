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
		srv->send(message);
	}
}

void Server::send(std::string message)
{
	std::string url("amqp://" + ip + ":" + port + "/zeusircd");
	try {
		std::vector<std::string> requests;
		requests.push_back(message);

		client c(url, requests);
		proton::container(c).run();

		return;
	} catch (const example::bad_option& e) {
		std::cout << e.what() << std::endl;
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

void Server::handleWrite(const boost::system::error_code& error, std::size_t bytes) {
	if (error)
		std::cout << "Error sending to server." << std::endl;
}

void Server::sendBurst (Server *server) {
	server->send("HUB " + config->Getvalue("hub"));
	server->send("SERVER " + config->Getvalue("serverName") + " " + ip + " " + port);
	if (config->Getvalue("cluster") == "false") {
		std::string version = "VERSION ";
		if (DB::GetLastRecord() != "") {
			version.append(DB::GetLastRecord());
		} else {
			version.append("0");
		}
		server->send(version);
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
		if (modos == "+")
			modos.append("x");
		server->send("SNICK " + it->second->mNickName + " " + it->second->mIdent + " " + it->second->mHost + " " + it->second->mCloak + " " + it->second->mvHost + " " + std::to_string(it->second->bLogin) + " " + it->second->mServer + " " + modos);
		if (it->second->bAway == true)
			server->send("AWAY " + it->second->mNickName + " " + it->second->mAway);
	}
	auto channels = Mainframe::instance()->channels();
	auto it2 = channels.begin();
	for (; it2 != channels.end(); ++it2) {
		auto users = it2->second->mLocalUsers;
		auto it4 = users.begin();
		for (; it4 != users.end(); ++it4) {
			std::string mode = "+x";
			if (it2->second->isOperator(*it4) == true)
				mode = "+o";
			else if (it2->second->isHalfOperator(*it4) == true)
				mode = "+h";
			else if (it2->second->isVoice(*it4) == true)
				mode = "+v";
			server->send("SJOIN " + (*it4)->mNickName + " " + it2->second->name() + " " + mode);
		}
		auto bans = it2->second->bans();
		auto it3 = bans.begin();
		for (; it3 != bans.end(); ++it3)
			server->send("SBAN " + it2->second->name() + " " + (*it3)->mask() + " " + (*it3)->whois() + " " + std::to_string((*it3)->time()));
		auto pbans = it2->second->pbans();
		auto it7 = pbans.begin();
		for (; it7 != pbans.end(); ++it7)
			server->send("SPBAN " + it2->second->name() + " " + (*it7)->mask() + " " + (*it7)->whois() + " " + std::to_string((*it7)->time()));
	}
	auto it6 = MemoMsg.begin();
	for (; it6 != MemoMsg.end(); ++it6)
		server->send("MEMO " + (*it6)->sender + " " + (*it6)->receptor + " " + std::to_string((*it6)->time) + " " + (*it6)->mensaje);
}

void Server::sendinitialBurst () {
	Send("HUB " + config->Getvalue("hub"));
	Send("SERVER " + config->Getvalue("serverName") + " " + ip + " " + port);
	if (config->Getvalue("cluster") == "false") {
		std::string version = "VERSION ";
		if (DB::GetLastRecord() != "") {
			version.append(DB::GetLastRecord());
		} else {
			version.append("0");
		}
		Send(version);
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
		if (modos == "+")
			modos.append("x");
		Send("SNICK " + it->second->mNickName + " " + it->second->mIdent + " " + it->second->mHost + " " + it->second->mCloak + " " + it->second->mvHost + " " + std::to_string(it->second->bLogin) + " " + it->second->mServer + " " + modos);
		if (it->second->bAway == true)
			Send("AWAY " + it->second->mNickName + " " + it->second->mAway);
	}
	auto channels = Mainframe::instance()->channels();
	auto it2 = channels.begin();
	for (; it2 != channels.end(); ++it2) {
		auto users = it2->second->mLocalUsers;
		auto it4 = users.begin();
		for (; it4 != users.end(); ++it4) {
			std::string mode = "+x";
			if (it2->second->isOperator(*it4) == true)
				mode = "+o";
			else if (it2->second->isHalfOperator(*it4) == true)
				mode = "+h";
			else if (it2->second->isVoice(*it4) == true)
				mode = "+v";
			Send("SJOIN " + (*it4)->mNickName + " " + it2->second->name() + " " + mode);
		}
		auto bans = it2->second->bans();
		auto it3 = bans.begin();
		for (; it3 != bans.end(); ++it3)
			Send("SBAN " + it2->second->name() + " " + (*it3)->mask() + " " + (*it3)->whois() + " " + std::to_string((*it3)->time()));
		auto pbans = it2->second->pbans();
		auto it7 = pbans.begin();
		for (; it7 != pbans.end(); ++it7)
			Send("SPBAN " + it2->second->name() + " " + (*it7)->mask() + " " + (*it7)->whois() + " " + std::to_string((*it7)->time()));
	}
	auto it6 = MemoMsg.begin();
	for (; it6 != MemoMsg.end(); ++it6)
		Send("MEMO " + (*it6)->sender + " " + (*it6)->receptor + " " + std::to_string((*it6)->time) + " " + (*it6)->mensaje);
}

bool Server::IsAServer (const std::string &ip) {
	for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++)
		if (config->Getvalue("link["+std::to_string(i)+"]ip") == ip)
				return true;
	return false;
}

bool Server::IsConected (const std::string &ip) {
	for (Server *server : Servers) {
		if (server->ip == ip)
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

void Server::SQUIT(std::string nombre)
{
	auto usermap = Mainframe::instance()->RemoteUsers();
	auto it = usermap.begin();
	for (; it != usermap.end(); ++it) {
		if (it->second)
			if (it->second->mServer == nombre)
				it->second->QUIT();
	}
	for (Server *server : Servers) {
		if (server->name == nombre) {
			Servers.erase(server);
			break;
		}
	}
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
			SQUIT(ip);
		} else {
			this->send("PING");
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(30)); 
			deadline.async_wait(boost::bind(&Server::CheckDead, this, boost::asio::placeholders::error));
		}
	}
}
