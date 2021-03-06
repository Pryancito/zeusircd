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
#include "module.h"
#include "Config.h"
#include "Server.h"

class CMD_Servers : public Module
{
	public:
	CMD_Servers() : Module("SERVERS", 50, false) {};
	~CMD_Servers() {};
	virtual void command(User *user, std::string message) override {
	if (user->getMode('o') == false) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not have iRCop privileges."));
			return;
		} else {
			for (unsigned int i = 0; i < config["links"].size(); i++) {
				std::string ip = config["links"][i]["ip"].as<std::string>();
				std::string port = config["links"][i]["port"].as<std::string>();
				for (Server *server : Servers) {
					if (server->ip == ip) {
						if (Server::IsConected(server->ip) == true) {
							user->SendAsServer("461 " + user->mNickName + " :" + server->ip + ":" + port + " " + server->name + " ( \0033CONNECTED\003 )");
							break;
						} else {
							user->SendAsServer("461 " + user->mNickName + " :" + ip + " ( \0034DISCONNECTED\003 )");
							break;
						}
					}
				}
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Servers);
}
