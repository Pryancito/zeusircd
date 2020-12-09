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
#include "ZeusiRCd.h"
#include "oper.h"
#include "sha256.h"
#include "Config.h"
#include "Utils.h"
#include "Server.h"

#include <string>

OperSet miRCOps;

bool Oper::Login (User *u, const std::string &nickname, const std::string &pass) {
	for (unsigned int i = 0; i < config["opers"].size(); i++)
		if (config["opers"][i]["nick"].as<std::string>() == nickname)
			if (config["opers"][i]["pass"].as<std::string>() == sha256(pass)) {
				miRCOps.insert(u->mNickName);
				u->SendAsServer("MODE " + u->mNickName + " +o");
				u->setMode('o', true);
				Server::Send("UMODE " + u->mNickName + " +o");
				return true;
			}
	GlobOPs(Utils::make_string("", "Failure /oper auth from nick: %s", nickname.c_str()));
	return false;
}

void Oper::GlobOPs(const std::string &message) {
	for (auto nick : miRCOps) {
		User *u = User::GetUser(nick);
		if (u->is_local == true)
			u->SendAsServer("NOTICE " + u->mNickName + " :" + message);
		else {
			Server::Send("NOTICE " + config["serverName"].as<std::string>() + " " + u->mNickName + " " + message);
		}
    }
}

int Oper::Count () {
	return miRCOps.size();
}
