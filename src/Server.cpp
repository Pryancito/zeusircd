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

std::map<std::string, unsigned int> mThrottle;
extern Memos MemoMsg;

bool Server::CanConnect(const std::string ip)
{
	/*
	if (stoi(config->Getvalue("maxUsers")) <= (LocalUsers.size() + GlobalUsers.size())) {
		return false;
	} else if (CheckClone(ip) == true) {
		return false;
	} else if (CheckDNSBL(ip) == true) {
		return false;
	} else if (CheckThrottle(ip) == true) {
		return false;
	} else if (OperServ::IsGlined(newclient->ip()) == true) {
		return false;
	} else if (OperServ::CanGeoIP(newclient->ip()) == false) {
		return false;
	}*/
	return true;
}
/*
bool Server::CheckClone(const std::string &ip) {
	unsigned int i = 0;
	UserMap user = Mainframe::instance()->users();
	UserMap::iterator it = user.begin();
	for (; it != user.end(); ++it) {
		if (it->second)
			if (it->second->host() == ip)
				i++;
	}
	unsigned int clones = OperServ::IsException(ip, "clon");
	if (clones != 0 && i <= clones)
		return false;
	return (i >= (unsigned int )stoi(config->Getvalue("clones")));
}
*/
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

bool Server::HUBExiste()
{
	return true;
}

void Server::sendall(const std::string message)
{
	return;
}

void Server::on_container_start(proton::container &c) {
	listener = c.listen(url, listen_handler);
	burst = true;
	for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++)
	{
		std::string server = config->Getvalue("link["+std::to_string(i)+"]ip") + ":" + config->Getvalue("link["+std::to_string(i)+"]port");
		servers.push_back(server);
		proton::sender sender = c.open_sender(server);
		senders[server] = std::move(sender);
	}
	Server::sendBurst();
	burst = false;
}

void Server::on_message(proton::delivery &d, proton::message &m) {
	    //Parser(m.body());
	    std::cout << m.body() << std::endl;
}
    
void Server::on_tracker_accept(proton::tracker &t) {
	confirmed++;

	if (confirmed == total) {
		std::cout << "all messages confirmed" << std::endl;
		t.connection().close();
	}
}

void Server::on_transport_close(proton::transport &) {
	sent = confirmed;
}

void Server::Send(std::string message)
{
	for (auto mp = senders.begin(); mp != senders.end(); mp++)
	{
		proton::sender sender = mp->second;
        proton::message reply;
        reply.body(message);
        reply.id(++sent);
        sender.send(reply);
    }
}

void Server::on_sender_open(proton::sender &sender) {
	for (unsigned int i = 0; i < servers.size(); i++)
	{
		if (sender.source().address() == servers[i])
		{
			std::string addr = sender.source().address();
			sender.open(proton::sender_options().source(proton::source_options().address(addr)));
			senders[addr] = sender;
			return;
		}
	}
	std::cout << "Servidor " << sender.source().address() << " intentando conectar" << std::endl;
}

void Server::sendBurst () {
	Send("HUB " + config->Getvalue("hub"));
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
		Send("SNICK " + it->second->mNickName + " " + it->second->mIdent + " " + it->second->mHost + " " + it->second->mCloak + " " + std::to_string(it->second->bLogin) + " " + it->second->mServer + " " + modos);
		if (it->second->bAway == true)
			Send("AWAY " + it->second->mNickName + " " + it->second->mAway);
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
			Send("SJOIN " + (*it4)->mNickName + " " + it2->second->name() + " " + mode);
		}
		auto bans = it2->second->bans();
		auto it3 = bans.begin();
		for (; it3 != bans.end(); ++it3)
			Send("SBAN " + it2->second->name() + " " + (*it3)->mask() + " " + (*it3)->whois() + " " + std::to_string((*it3)->time()));
	}
	auto it6 = MemoMsg.begin();
	for (; it6 != MemoMsg.end(); ++it6)
		Send("MEMO " + (*it6)->sender + " " + (*it6)->receptor + " " + std::to_string((*it6)->time) + " " + (*it6)->mensaje);
}
