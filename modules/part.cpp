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

class CMD_Part : public Module
{
	public:
	CMD_Part() : Module("PART", 50, false) {};
	~CMD_Part() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		else if (results.size() < 2) return;
		Channel* chan = Channel::GetChannel(results[1]);
		if (chan) {
			if (chan->HasUser(user) == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are not into the channel."));
				return;
			}
			chan->increaseflood();
			Server::Send("SPART " + user->mNickName + " " + chan->name);
			chan->part(user);
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Part);
}
