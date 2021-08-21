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

class CMD_Sapart : public Module
{
	public:
	CMD_Sapart() : Module("SAPART", 50, false) {};
	~CMD_Sapart() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (user->getMode('o') == false) {
			user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		}
		else if (results.size() < 3) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		}
		else if (User::FindUser(results[1]) == false) {
			user->SendAsServer("401 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick does not exist."));
			return;
		}
		Channel* chan = Channel::GetChannel(results[2]);
		User *usr = User::GetUser(results[1]);
		if (chan) {
			if (chan->HasUser(usr) == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The user is not into the channel."));
				return;
			}
			Server::Send("SPART " + usr->mNickName + " " + chan->name);
			chan->part(usr);
			usr->SendAsServer("461 " + usr->mNickName + " :" + Utils::make_string(usr->mLang, "You were forced to exit %s.", chan->name.c_str()));
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Sapart);
}
