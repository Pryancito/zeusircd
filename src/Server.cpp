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
#include "Server.h"

std::map<std::string, unsigned int> mThrottle;

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
